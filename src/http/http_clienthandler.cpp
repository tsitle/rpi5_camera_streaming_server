#include <chrono>
#include <iostream>
#include <sstream>
#include <sys/socket.h>  // ::setsockopt()
#include <unistd.h>  // ::write()
#include <opencv2/opencv.hpp>

#include "../settings.hpp"
#include "../httpparser/httprequestparser.hpp"
#include "../httpparser/request.hpp"
#include "../httpparser/urlparser.hpp"
#include "../json/json.hpp"
#include "http_clienthandler.hpp"

using namespace std::chrono_literals;
using json = nlohmann::json;

namespace http {

	const uint32_t BUFFER_SIZE = 16 * 1024;

	const std::string URL_PSEUDO_HOST = "http://pseudohost";

	// -----------------------------------------------------------------------------
	// -----------------------------------------------------------------------------

	ClientHandler::ClientHandler(
			const uint32_t thrIx,
			const int32_t socket,
			CbAddRunningHandler cbAddRunningHandler,
			CbRemoveRunningHandler cbRemoveRunningHandler,
			CbIncStreamingClientCount cbIncStreamingClientCount,
			CbDecStreamingClientCount cbDecStreamingClientCount,
			CbGetFrameFromQueue cbGetFrameFromQueue) :
				gClientSocket(socket),
				gCbDecStreamingClientCount(cbDecStreamingClientCount),
				gCbGetFrameFromQueue(cbGetFrameFromQueue) {
		//
		gHndCltData.thrIx = thrIx;
		gHndCltData.cbIncStreamingClientCount = cbIncStreamingClientCount;
		gHndCltData.staticOptionsStc = fcapcfgfile::CfgFile::getStaticOptions();
		gHndCltData.rtOptsCur = fcapshared::Shared::getRuntimeOptions();
		gHndCltData.rtOptsNew = gHndCltData.rtOptsCur;
		//
		gPHandleRouteGet = new HandleRouteGet(&gHndCltData);

		//
		char buffer[BUFFER_SIZE] = {0};
		struct timeval tv;

		//
		cbAddRunningHandler(thrIx);

		//
		std::ostringstream ss;
		ss << "\r\n";
		ss << fcapconstants::HTTP_BOUNDARY_SEPARATOR << "\r\n";
		ss << "Content-Type: " << fcapconstants::HTTP_CONTENT_TYPE_JPEG << "\r\n";
		ss << "Content-Length: ";
		gRespMultipartPrefix = ss.str();

		// set timeouts for send/receive
		tv.tv_sec = 10;
		tv.tv_usec = 0;
		//
		/**log(gHndCltData.thrIx, "read");**/
		::setsockopt(socket, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof(tv));
		::setsockopt(socket, SOL_SOCKET, SO_SNDTIMEO, (const char*)&tv, sizeof(tv));
		int32_t bytesReceived = ::read(socket, buffer, BUFFER_SIZE);
		if (bytesReceived < 0) {
			log(gHndCltData.thrIx, "Failed to read bytes from client socket connection");
		} else {
			/**log(gHndCltData.thrIx, "------ Received Request from client ------");**/
			handleRequest(buffer, (uint32_t)bytesReceived);
		}

		//
		cbRemoveRunningHandler(thrIx);
	}

	ClientHandler::~ClientHandler() {
		/*log(gHndCltData.thrIx, "close socket");**/
		::close(gClientSocket);
	}

	std::thread ClientHandler::startThread(
			const uint32_t thrIx,
			const int32_t socket,
			CbAddRunningHandler cbAddRunningHandler,
			CbRemoveRunningHandler cbRemoveRunningHandler,
			CbIncStreamingClientCount cbIncStreamingClientCount,
			CbDecStreamingClientCount cbDecStreamingClientCount,
			CbGetFrameFromQueue cbGetFrameFromQueue) {
		std::thread threadClientObj(
				_startThread_internal,
				thrIx,
				socket,
				cbAddRunningHandler,
				cbRemoveRunningHandler,
				cbIncStreamingClientCount,
				cbDecStreamingClientCount,
				cbGetFrameFromQueue
			);
		threadClientObj.detach();
		return threadClientObj;
	}

	// -----------------------------------------------------------------------------
	// -----------------------------------------------------------------------------

	void ClientHandler::_startThread_internal(
			const uint32_t thrIx,
			const int32_t socket,
			CbAddRunningHandler cbAddRunningHandler,
			CbRemoveRunningHandler cbRemoveRunningHandler,
			CbIncStreamingClientCount cbIncStreamingClientCount,
			CbDecStreamingClientCount cbDecStreamingClientCount,
			CbGetFrameFromQueue cbGetFrameFromQueue) {
		ClientHandler chnd = ClientHandler(
				thrIx,
				socket,
				cbAddRunningHandler,
				cbRemoveRunningHandler,
				cbIncStreamingClientCount,
				cbDecStreamingClientCount,
				cbGetFrameFromQueue);
		/**
		// Debugging shutdown procedure:
		std::this_thread::sleep_for(std::chrono::milliseconds(3000 * (thrIx + 1)));
		**/

		/**log(thrIx, "thread end");**/
	}

	void ClientHandler::log(const uint32_t thrIx, const std::string &message) {
		std::cout << "CLIENT#" << std::to_string(thrIx) << ": " << message << std::endl;
	}

	// -----------------------------------------------------------------------------

	void ClientHandler::handleRequest(const char *buffer, const uint32_t bufSz) {
		bool methOk;
		bool methIsGet;
		bool methIsOptions;
		bool requUriOk = false;
		bool success = false;
		std::string requFullUri;
		httpparser::Request request;
		httpparser::HttpRequestParser requparser;
		httpparser::UrlParser urlparser;

		httpparser::HttpRequestParser::ParseResult requParseRes = requparser.parse(request, buffer, buffer + bufSz);

		if (requParseRes != httpparser::HttpRequestParser::ParsingCompleted) {
			log(gHndCltData.thrIx, "Parsing failed");
			return;
		}

		/**log(gHndCltData.thrIx, request.inspect());**/
		methIsGet = (request.method.compare("GET") == 0);
		methIsOptions = (request.method.compare("OPTIONS") == 0);
		methOk = (methIsGet || methIsOptions);
		if (! methOk) {
			log(gHndCltData.thrIx, "405 Method Not Allowed");
			gHndCltData.respHttpStat = 405;
		}

		requFullUri = URL_PSEUDO_HOST + request.uri;

		if (methOk && ! urlparser.parse(requFullUri)) {
			/**log(gHndCltData.thrIx, "404 invalid path '" + request.uri + "'");**/
			log(gHndCltData.thrIx, "404 invalid path");
			gHndCltData.respHttpStat = 404;
		} else if (methOk) {
			gRequUriPath = urlparser.path();
			requUriOk = (! gRequUriPath.empty());
			if (requUriOk) {
				gRequUriQuery = urlparser.query();
			}
		}

		if (methIsGet && requUriOk) {
			success = gPHandleRouteGet->handleRequest(gRequUriPath, gRequUriQuery);
		} else if (methIsOptions && requUriOk) {
			// the browser is doing a "preflight" check of this API endpoint
			/**log(gHndCltData.thrIx, "200 OPTIONS");**/
			success = true;
			gHndCltData.respReturnJson = true;
		}

		if (success || gHndCltData.respReturnJson) {
			gHndCltData.respHttpStat = 200;
		}

		//
		if (success && gHndCltData.isNewStreamingClientAccepted) {
			startStreaming();
		} else if (gHndCltData.respReturnJson) {
			if (! success) {
				log(gHndCltData.thrIx, "__ERR " + gHndCltData.respErrMsg);
			}
			//
			if (methIsGet) {
				gHndCltData.respHttpMsgString = buildJsonResult(success);
			} else {
				gHndCltData.respHttpMsgString = "";
			}
			sendResponse(&fcapconstants::HTTP_CONTENT_TYPE_JSON, &gHndCltData.respHttpMsgString);
		} else {
			sendResponse(&fcapconstants::HTTP_CONTENT_TYPE_HTML, &gHndCltData.respHttpMsgString);
		}
	}

	// -----------------------------------------------------------------------------

	std::string ClientHandler::buildResponse(const std::string *pHttpContentType, const std::string *pContent) {
		std::string httpStatus = "HTTP/1.1 ";
		switch (gHndCltData.respHttpStat) {
			case 200:
				httpStatus += "200 OK";
				break;
			case 404:
				httpStatus += "404 Not Found";
				break;
			case 405:
				httpStatus += "405 Method Not Allowed";
				break;
			default:
				httpStatus += "500 Internal Server Error";
		}

		uint32_t cntSz = (pContent != NULL ? pContent->size() : 0);
		if (cntSz != 0) {
			cntSz += 2;
		}
		std::ostringstream ss;
		ss << httpStatus << "\r\n";
		ss << "Server: " << fcapconstants::HTTP_SERVER_NAME << "/" << fcapconstants::HTTP_SERVER_VERSION << "\r\n";
		ss << "Connection: close" << "\r\n";
		ss << "Max-Age: 0" << "\r\n";
		ss << "Expires: 0" << "\r\n";
		ss << "Cache-Control: no-cache, private" << "\r\n";
		ss << "Pragma: no-cache" << "\r\n";
		ss << "Access-Control-Allow-Origin: *" << "\r\n";
		ss << "Access-Control-Allow-Methods: GET" << "\r\n";
		ss << "Access-Control-Allow-Headers: API-Key,Content-Type,If-Modified-Since,Cache-Control" << "\r\n";
		ss << "Access-Control-Max-Age: 0" << "\r\n";
		ss << "Content-Type: " << *pHttpContentType;
		if (pHttpContentType->compare(fcapconstants::HTTP_CONTENT_TYPE_MULTIPART) == 0) {
			ss << "; boundary=" << fcapconstants::HTTP_BOUNDARY_SEPARATOR;
		}
		ss << "\r\n";
		if (cntSz != 0) {
			ss << "Content-Length: " << cntSz << "\r\n";
			ss << "\r\n";
			ss << *pContent;
			ss << "\r\n";
		}

		return ss.str();
	}

	bool ClientHandler::sendResponse(const std::string *pHttpContentType, const std::string *pContent) {
		std::string respMsg = buildResponse(pHttpContentType, pContent);

		/**log(gHndCltData.thrIx, "__>> " + respMsg);**/
		long bytesSent = ::write(gClientSocket, respMsg.c_str(), respMsg.size());
		if (bytesSent != (long)respMsg.size()) {
			log(gHndCltData.thrIx, "__Error sending response to client");
			return false;
		}
		return true;
	}

	void ClientHandler::startStreaming() {
		const bool _MEASURE_TIME_COPY = false;
		const bool _MEASURE_TIME_SEND = false;
		bool haveFrame = false;
		bool resB;
		bool needToStop = false;
		uint32_t bufSz = 0;
		uint32_t rsvdBufSz = 64 * 1024;
		uint8_t* pData = (uint8_t*)::malloc(rsvdBufSz);
		uint32_t toNeedToStop = 100;
		auto timeFpsStart = std::chrono::steady_clock::now();
		auto timeFpsCur = std::chrono::steady_clock::now();
		bool timeFpsRun = false;
		uint32_t timeFpsDiffMs;
		uint32_t timeFpsFrames = 0;
		auto timeStart = std::chrono::steady_clock::now();
		auto timeEnd = std::chrono::steady_clock::now();

		if (pData == NULL) {
			gCbDecStreamingClientCount();
			return;
		}
		//
		resB = sendResponse(&fcapconstants::HTTP_CONTENT_TYPE_MULTIPART, NULL);
		if (! resB) {
			gCbDecStreamingClientCount();
			return;
		}
		while (true) {
			// check if we need to stop
			if (--toNeedToStop == 0) {
				needToStop = fcapshared::Shared::getFlagNeedToStop();
				if (needToStop) {
					break;
				}
				toNeedToStop = 100;
			}

			// copy frame from queue
			if (_MEASURE_TIME_COPY) {
				timeStart = std::chrono::steady_clock::now();
			}
			haveFrame = gCbGetFrameFromQueue(gHndCltData.thrIx, &pData, rsvdBufSz, bufSz);
			if (_MEASURE_TIME_COPY) {
				timeEnd = std::chrono::steady_clock::now();
				if (haveFrame) {
					log(gHndCltData.thrIx, "__copy frame took " + std::to_string(std::chrono::duration_cast<std::chrono::microseconds>(timeEnd - timeStart).count()) + " us");
				}
			}

			// send frame to client
			if (haveFrame) {
				/**log(gHndCltData.thrIx, "__send frame");**/
				//
				++timeFpsFrames;
				if (! timeFpsRun) {
					timeFpsStart = std::chrono::steady_clock::now();
					timeFpsRun = true;
				} else {
					timeFpsCur = std::chrono::steady_clock::now();
					timeFpsDiffMs = std::chrono::duration_cast<std::chrono::milliseconds>(timeFpsCur - timeFpsStart).count();
					if (timeFpsDiffMs >= 10000) {
						log(gHndCltData.thrIx, "__FPS=" + std::to_string(((float)timeFpsFrames / (float)timeFpsDiffMs) * 1000.0));
						//
						timeFpsStart = std::chrono::steady_clock::now();
						timeFpsFrames = 0;
					}
				}
				//
				if (_MEASURE_TIME_SEND) {
					timeStart = std::chrono::steady_clock::now();
				}
				resB = sendFrame(pData, bufSz);
				if (! resB) {
					break;
				}
				if (_MEASURE_TIME_SEND) {
					timeEnd = std::chrono::steady_clock::now();
					log(gHndCltData.thrIx, "__send frame took " + std::to_string(std::chrono::duration_cast<std::chrono::microseconds>(timeEnd - timeStart).count()) + " us");
				}
				//
				haveFrame = false;
			} else {
				std::this_thread::sleep_for(std::chrono::milliseconds(10));
			}
		}
		if (pData != NULL) {
			::free(pData);
		}
		//
		gCbDecStreamingClientCount();
	}

	bool ClientHandler::sendFrame(uint8_t *pData, const uint32_t bufferSz) {
		long bytesSent;

		std::string respMsg = gRespMultipartPrefix + std::to_string(bufferSz) + "\r\n\r\n";
		/**log(gHndCltData.thrIx, ">> " + respMsg);**/
		bytesSent = ::write(gClientSocket, respMsg.c_str(), respMsg.size());
		if (bytesSent != (long)respMsg.size()) {
			if (bytesSent == -1) {
				log(gHndCltData.thrIx, "__Client connection closed");
			} else {
				log(gHndCltData.thrIx, "__Error sending response to client #HE (sent=" + std::to_string(bytesSent) + ")");
			}
			return false;
		}
		//
		uint32_t sentTot = 0;
		uint32_t remBufSz = bufferSz;
		uint32_t curBufSz = BUFFER_SIZE;
		uint8_t* pStartBuf = pData;
		/**char strBuf[1024];
		snprintf(strBuf, sizeof(strBuf), "__b 0x%02X%02X 0x%02X%02X", pData[0], pData[1], pData[bufferSz - 2], pData[bufferSz - 1]);
		log(gHndCltData.thrIx, strBuf);**/
		while (remBufSz != 0) {
			if (curBufSz > remBufSz) {
				curBufSz = remBufSz;
			}
			bytesSent = ::write(gClientSocket, pStartBuf, curBufSz);
			if (bytesSent != (long)curBufSz) {
				if (bytesSent == -1) {
					log(gHndCltData.thrIx, "__Client connection closed");
				} else {
					log(gHndCltData.thrIx, "__Error sending response to client #DA (sent=" +
							std::to_string(bytesSent) + ", exp=" + std::to_string(curBufSz) + ", ts=" + std::to_string(sentTot) +
							")");
				}
				return false;
			}
			sentTot += curBufSz;
			remBufSz -= curBufSz;
			pStartBuf += curBufSz;
		}
		/**log(gHndCltData.thrIx, "__sent " + std::to_string(sentTot) + " total");**/
		return true;
	}

	std::string ClientHandler::buildJsonResult(const bool success) {
		json jsonObj;

		jsonObj["result"] = (success ? "success" : "error");
		if (success) {
			fcapconstants::OutputCamsEn outputCams = gHndCltData.rtOptsNew.outputCams;
			bool tmpProcCalRunning;
			bool tmpProcCalDone;
			bool tmpProcPtDone;

			//
			if (gHndCltData.curCamId() == NULL) {
				tmpProcCalRunning = (gHndCltData.rtOptsNew.procCalDoStart[fcapconstants::CamIdEn::CAM_0] &&
						gHndCltData.rtOptsNew.procCalDoStart[fcapconstants::CamIdEn::CAM_1]);
				tmpProcCalDone = (gHndCltData.rtOptsNew.procCalDone[fcapconstants::CamIdEn::CAM_0] &&
						gHndCltData.rtOptsNew.procCalDone[fcapconstants::CamIdEn::CAM_1]);
				tmpProcPtDone = (gHndCltData.rtOptsNew.procPtDone[fcapconstants::CamIdEn::CAM_0] &&
						gHndCltData.rtOptsNew.procPtDone[fcapconstants::CamIdEn::CAM_1]);
			} else {
				tmpProcCalRunning = gHndCltData.rtOptsNew.procCalDoStart[*gHndCltData.curCamId()];
				tmpProcCalDone = gHndCltData.rtOptsNew.procCalDone[*gHndCltData.curCamId()];
				tmpProcPtDone = gHndCltData.rtOptsNew.procPtDone[*gHndCltData.curCamId()];
			}

			//
			std::string tmpAvail;

			if (gHndCltData.isCameraAvailabelL() && gHndCltData.isCameraAvailabelR()) {
				tmpAvail = "BOTH";
			} else if (gHndCltData.isCameraAvailabelL()) {
				tmpAvail = "L";
			} else if (gHndCltData.isCameraAvailabelR()) {
				tmpAvail = "R";
			}
			jsonObj["availOutputCams"] = tmpAvail;

			//
			std::string tmpOc;

			switch (outputCams) {
				case fcapconstants::OutputCamsEn::CAM_L:
					tmpOc = "L";
					break;
				case fcapconstants::OutputCamsEn::CAM_R:
					tmpOc = "R";
					break;
				default:
					tmpOc = "BOTH";
			}
			jsonObj["outputCams"] = tmpOc;

			//
			jsonObj["cameraReady"] = {
					{"L", gHndCltData.rtOptsNew.cameraReady[gHndCltData.staticOptionsStc.camL]},
					{"R", gHndCltData.rtOptsNew.cameraReady[gHndCltData.staticOptionsStc.camR]}
				};

			//
			jsonObj["enabledProc"] = {
					{"bnc", gHndCltData.staticOptionsStc.procEnabled.bnc},
					{"cal", gHndCltData.staticOptionsStc.procEnabled.cal},
					{"pt", gHndCltData.staticOptionsStc.procEnabled.pt},
					{"tr", gHndCltData.staticOptionsStc.procEnabled.tr},
					{"overlCal", gHndCltData.staticOptionsStc.procEnabled.overlCal},
					{"overlCam", gHndCltData.staticOptionsStc.procEnabled.overlCam}
				};

			//
			jsonObj["resolutionOutput_w"] = gHndCltData.staticOptionsStc.resolutionOutput.width;
			jsonObj["resolutionOutput_h"] = gHndCltData.staticOptionsStc.resolutionOutput.height;
			//
			jsonObj["procBncAdjBrightness"] = gHndCltData.rtOptsNew.procBncAdjBrightness;
			jsonObj["procBncAdjContrast"] = gHndCltData.rtOptsNew.procBncAdjContrast;
			//
			jsonObj["procCalRunning"] = tmpProcCalRunning;
			//
			jsonObj["procCalDone"] = tmpProcCalDone;
			//
			jsonObj["procCalShowCalibChessboardPoints"] = gHndCltData.rtOptsNew.procCalShowCalibChessboardPoints;
			//
			jsonObj["procPtDone"] = tmpProcPtDone;
			//
			if (gHndCltData.curCamId() != NULL) {
				uint8_t tmpSz = gHndCltData.rtOptsNew.procPtRectCorners[*gHndCltData.curCamId()].size();
				for (uint8_t x = 1; x <= fcapconstants::PROC_PT_RECTCORNERS_MAX; x++) {
					jsonObj["procPtRectCorners_" + std::to_string(x) + "x"] = (tmpSz >= x ?
							std::to_string(gHndCltData.rtOptsNew.procPtRectCorners[*gHndCltData.curCamId()][x - 1].x) :
							"-");
					jsonObj["procPtRectCorners_" + std::to_string(x) + "y"] = (tmpSz >= x ?
							std::to_string(gHndCltData.rtOptsNew.procPtRectCorners[*gHndCltData.curCamId()][x - 1].y) :
							"-");
				}
			}
			//
			cv::Point tmpPntL = gHndCltData.rtOptsNew.procTrDelta[gHndCltData.staticOptionsStc.camL];
			cv::Point tmpPntR = gHndCltData.rtOptsNew.procTrDelta[gHndCltData.staticOptionsStc.camR];
			jsonObj["procTrDelta"] = {
					{"L", {
							{"x", tmpPntL.x},
							{"y", tmpPntL.y}
						}},
					{"R", {
							{"x", tmpPntR.x},
							{"y", tmpPntR.y}
						}}
				};
		} else {
			jsonObj["message"] = gHndCltData.respErrMsg;
		}
		return jsonObj.dump();
	}

}  // namespace http
