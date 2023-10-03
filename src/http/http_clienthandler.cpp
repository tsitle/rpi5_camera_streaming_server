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
#include "http_handleroute_get.hpp"
#include "http_handleroute_post.hpp"

using namespace std::chrono_literals;
using json = nlohmann::json;

namespace http {

	const uint32_t BUFFER_SIZE = 16 * 1024;

	const std::string URL_PSEUDO_HOST = "http://pseudohost";

	cputemp::CpuTemp ClientHandler::gCpuTemp;

	// -----------------------------------------------------------------------------
	// -----------------------------------------------------------------------------

	ClientHandler::ClientHandler(
			const uint32_t thrIx,
			const int32_t socket,
			CbAddRunningHandler cbAddRunningHandler,
			CbRemoveRunningHandler cbRemoveRunningHandler,
			CbIncStreamingClientCount cbIncStreamingClientCount,
			CbDecStreamingClientCount cbDecStreamingClientCount,
			CbGetFrameFromQueue cbGetFrameFromQueue,
			CbSetFramerateInfo cbSetFramerateInfo,
			CbGetFramerateInfo cbGetFramerateInfo) :
				gClientSocket(socket),
				gCbDecStreamingClientCount(cbDecStreamingClientCount),
				gCbGetFrameFromQueue(cbGetFrameFromQueue),
				gCbSetFramerateInfo(cbSetFramerateInfo),
				gCbGetFramerateInfo(cbGetFramerateInfo) {
		//
		gHndCltData.thrIx = thrIx;
		gHndCltData.cbIncStreamingClientCount = cbIncStreamingClientCount;
		gHndCltData.staticOptionsStc = fcapcfgfile::CfgFile::getStaticOptions();
		gHndCltData.rtOptsCur = fcapshared::Shared::getRuntimeOptions();
		gHndCltData.rtOptsNew = gHndCltData.rtOptsCur;

		//
		char buffer[BUFFER_SIZE] = {0};
		struct timeval tv;

		//
		bool clientAccepted = cbAddRunningHandler(thrIx);

		//
		if (clientAccepted) {
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
		}

		//
		cbRemoveRunningHandler(thrIx);
	}

	ClientHandler::~ClientHandler() {
		char buffer[BUFFER_SIZE] = {0};
		int32_t bytesReceived;

		::shutdown(gClientSocket, SHUT_RDWR);
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
		while ((bytesReceived = ::read(gClientSocket, buffer, BUFFER_SIZE)) > 0) {
			// read until no more data arrives
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
		}
		::close(gClientSocket);
	}

	std::thread ClientHandler::startThread(
			const uint32_t thrIx,
			const int32_t socket,
			CbAddRunningHandler cbAddRunningHandler,
			CbRemoveRunningHandler cbRemoveRunningHandler,
			CbIncStreamingClientCount cbIncStreamingClientCount,
			CbDecStreamingClientCount cbDecStreamingClientCount,
			CbGetFrameFromQueue cbGetFrameFromQueue,
			CbSetFramerateInfo cbSetFramerateInfo,
			CbGetFramerateInfo cbGetFramerateInfo) {
		std::thread threadClientObj(
				_startThread_internal,
				thrIx,
				socket,
				cbAddRunningHandler,
				cbRemoveRunningHandler,
				cbIncStreamingClientCount,
				cbDecStreamingClientCount,
				cbGetFrameFromQueue,
				cbSetFramerateInfo,
				cbGetFramerateInfo
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
			CbGetFrameFromQueue cbGetFrameFromQueue,
			CbSetFramerateInfo cbSetFramerateInfo,
			CbGetFramerateInfo cbGetFramerateInfo) {
		ClientHandler chnd = ClientHandler(
				thrIx,
				socket,
				cbAddRunningHandler,
				cbRemoveRunningHandler,
				cbIncStreamingClientCount,
				cbDecStreamingClientCount,
				cbGetFrameFromQueue,
				cbSetFramerateInfo,
				cbGetFramerateInfo);
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
		bool apiKeyOk = false;
		bool methOk;
		bool methIsGet;
		bool methIsPost;
		bool methIsOptions;
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
		methIsPost = (! methIsGet && request.method.compare("POST") == 0);
		methIsOptions = (! (methIsGet || methIsPost) && request.method.compare("OPTIONS") == 0);
		methOk = (methIsGet || methIsPost || methIsOptions);
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
			if (gRequUriPath.empty()) {
				gRequUriPath = "/";
			}
			//
			apiKeyOk = (
						(methIsGet && gRequUriPath.compare(fcapconstants::HTTP_URL_PATH_STREAM) == 0) ||
							methIsOptions || checkApiKey((void*)&request.headers)
					);
			if (! apiKeyOk) {
				log(gHndCltData.thrIx, "401 Unauthorized");
				gHndCltData.respHttpStat = 401;
			} else {
				if (methIsGet) {
					gRequUriQuery = urlparser.query();
				} else if (methIsPost) {
					std::vector<char> *tmpData = &request.content;
					if (! tmpData->empty()) {
						gRequUriQuery = std::string(tmpData->begin(), tmpData->end());
					}
				}
			}
		}

		if ((methIsGet || methIsPost) && apiKeyOk) {
			HandleRoute *pHndRoute;

			if (methIsGet) {
				pHndRoute = new HandleRouteGet(&gHndCltData);
			} else {
				pHndRoute = new HandleRoutePost(&gHndCltData);
			}

			success = pHndRoute->handleRequest(gRequUriPath, gRequUriQuery);
		} else if (methIsOptions) {
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
			if (methIsGet || methIsPost) {
				gHndCltData.respHttpMsgString = buildJsonResult(success);
			} else {
				gHndCltData.respHttpMsgString = "";
			}
			sendResponse(&fcapconstants::HTTP_CONTENT_TYPE_JSON, &gHndCltData.respHttpMsgString);
		} else {
			if (! success) {
				gHndCltData.respHttpMsgString = gHndCltData.respErrMsg;
			}
			sendResponse(&fcapconstants::HTTP_CONTENT_TYPE_HTML, &gHndCltData.respHttpMsgString);
		}
	}

	// -----------------------------------------------------------------------------

	bool ClientHandler::checkApiKey(void *pHeaders) {
		bool resB = false;
		std::vector<httpparser::Request::HeaderItem> *pHeadersVec = static_cast<std::vector<httpparser::Request::HeaderItem>*>(pHeaders);

		for (uint32_t x = 0; x < pHeadersVec->size(); x++) {
			httpparser::Request::HeaderItem hi = (*pHeadersVec)[x];
			std::string hiKey = fcapshared::Shared::toUpper(hi.name);
			/**log(gHndCltData.thrIx, "hi " + hiKey + "=" + hi.value);**/
			if (hiKey.compare("APIKEY") != 0) {
				continue;
			}
			std::string hiVal = fcapshared::Shared::toLower(hi.value);
			for (uint32_t akIx = 0; akIx < gHndCltData.staticOptionsStc.apiKeys.size(); akIx++) {
				std::string sosAk = gHndCltData.staticOptionsStc.apiKeys[akIx];
				/**log(gHndCltData.thrIx, "sosAk[" + std::to_string(akIx) + "]='" + sosAk + "'");**/
				if (hiVal.compare(sosAk) == 0) {
					resB = true;
					break;
				}
			}
			break;
		}
		return resB;
	}

	// -----------------------------------------------------------------------------

	std::string ClientHandler::buildResponse(const std::string *pHttpContentType, const std::string *pContent) {
		std::string httpStatus = "HTTP/1.1 ";
		switch (gHndCltData.respHttpStat) {
			case 200:
				httpStatus += "200 OK";
				break;
			case 401:
				httpStatus += "401 Unauthorized";
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
		ss << "Access-Control-Allow-Methods: GET,POST" << "\r\n";
		ss << "Access-Control-Allow-Headers: Apikey,Content-Type,If-Modified-Since,Cache-Control" << "\r\n";
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

	void ClientHandler::buildJsonResult_procTrDeltaX(void *pJsonObj, const std::string key, std::map<fcapconstants::CamIdEn, cv::Point> &val) {
		cv::Point tmpPntL = val[gHndCltData.staticOptionsStc.camL];
		cv::Point tmpPntR = val[gHndCltData.staticOptionsStc.camR];

		(*((json*)pJsonObj))["procTr"][key] = {
				{"L", {
						{"x", tmpPntL.x},
						{"y", tmpPntL.y}
					}},
				{"R", {
						{"x", tmpPntR.x},
						{"y", tmpPntR.y}
					}}
			};
	}

	std::string ClientHandler::buildJsonResult(const bool success) {
		json jsonObj;

		jsonObj["result"] = (success ? "success" : "error");
		if (success) {
			fcapconstants::OutputCamsEn outputCams = gHndCltData.rtOptsNew.outputCams;
			bool tmpProcCalRunning;
			bool tmpProcCalDone;
			bool tmpProcCalShowChess;
			bool tmpProcPtDone;

			//
			if (gHndCltData.curCamId() == NULL) {
				tmpProcCalRunning = (gHndCltData.rtOptsNew.procCalDoStart[fcapconstants::CamIdEn::CAM_0] ||
						gHndCltData.rtOptsNew.procCalDoStart[fcapconstants::CamIdEn::CAM_1]);
				tmpProcCalDone = (gHndCltData.rtOptsNew.procCalDone[fcapconstants::CamIdEn::CAM_0] &&
						gHndCltData.rtOptsNew.procCalDone[fcapconstants::CamIdEn::CAM_1]);
				tmpProcPtDone = (gHndCltData.rtOptsNew.procPtDone[fcapconstants::CamIdEn::CAM_0] &&
						gHndCltData.rtOptsNew.procPtDone[fcapconstants::CamIdEn::CAM_1]);
				tmpProcCalShowChess = (gHndCltData.rtOptsNew.procCalShowCalibChessboardPoints[fcapconstants::CamIdEn::CAM_0] ||
						gHndCltData.rtOptsNew.procCalShowCalibChessboardPoints[fcapconstants::CamIdEn::CAM_1]);
			} else {
				tmpProcCalRunning = gHndCltData.rtOptsNew.procCalDoStart[*gHndCltData.curCamId()];
				tmpProcCalDone = gHndCltData.rtOptsNew.procCalDone[*gHndCltData.curCamId()];
				tmpProcPtDone = gHndCltData.rtOptsNew.procPtDone[*gHndCltData.curCamId()];
				tmpProcCalShowChess = gHndCltData.rtOptsNew.procCalShowCalibChessboardPoints[*gHndCltData.curCamId()];
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
			jsonObj["cpuTemperature"] = gCpuTemp.getTemperature();

			//
			jsonObj["framerate"] = (gHndCltData.streamingClientId > 0 ? gCbGetFramerateInfo(gHndCltData.streamingClientId) : 0);

			//
			jsonObj["enabledProc"] = {
					{"bnc", gHndCltData.staticOptionsStc.procEnabled.bnc},
					{"cal", gHndCltData.staticOptionsStc.procEnabled.cal},
					{"flip", gHndCltData.staticOptionsStc.procEnabled.flip},
					{"pt", gHndCltData.staticOptionsStc.procEnabled.pt},
					{"roi", gHndCltData.staticOptionsStc.procEnabled.roi},
					{"tr", gHndCltData.staticOptionsStc.procEnabled.tr},
					{"overlCal", gHndCltData.staticOptionsStc.procEnabled.overlCal},
					{"overlCam", gHndCltData.staticOptionsStc.procEnabled.overlCam}
				};

			//
			const std::string *pTmpIsst;
			switch (gHndCltData.staticOptionsStc.camSourceType) {
				case fcapconstants::CamSourceEn::GSTREAMER:
					pTmpIsst = &fcapconstants::CONFFILE_CAMSRC_GSTR;
					break;
				case fcapconstants::CamSourceEn::MJPEG:
					pTmpIsst = &fcapconstants::CONFFILE_CAMSRC_MJPEG;
					break;
				default:
					pTmpIsst = &fcapconstants::CONFFILE_CAMSRC_UNSPEC;
			}
			jsonObj["inputStreamSourceType"] = *pTmpIsst;
			jsonObj["resolutionInputStream"] = {
					{"w", gHndCltData.staticOptionsStc.resolutionInputStream.width},
					{"h", gHndCltData.staticOptionsStc.resolutionInputStream.height}
				};
			jsonObj["resolutionOutput"] = {
					{"w", gHndCltData.rtOptsNew.resolutionOutput.width},
					{"h", gHndCltData.rtOptsNew.resolutionOutput.height}
				};
			//
			jsonObj["procBnc"] = {
					{"brightness", {
							{"val", gHndCltData.rtOptsNew.procBncAdjBrightness},
							{"min", -100},
							{"max", 100},
							{"supported", true}
						}},
					{"contrast", {
							{"val", gHndCltData.rtOptsNew.procBncAdjContrast},
							{"min", -100},
							{"max", 100},
							{"supported", true}
						}},
					{"gamma", {
							{"val", gHndCltData.rtOptsNew.procBncAdjGamma},
							{"min", -100},
							{"max", 100},
							{"supported", fcapsettings::PROC_BNC_USE_ALGO == fcapconstants::ProcBncAlgoEn::TYPE1}
						}}
				};
			//
			jsonObj["procCal"] = {
					{"running", tmpProcCalRunning},
					{"done", tmpProcCalDone},
					{"showCalibChessboardPoints", tmpProcCalShowChess}
				};
			//
			jsonObj["procGrid"] = {
					{"show", gHndCltData.rtOptsNew.procGridShow}
				};
			//
			///
			jsonObj["procPt"] = {
					{"done", tmpProcPtDone}
				};
			///
			if (gHndCltData.curCamId() != NULL &&
					! (gHndCltData.staticOptionsStc.procEnabled.cal || tmpProcPtDone)) {
				jsonObj["procPt"]["rectCorners"] = {};
				uint8_t tmpSz = gHndCltData.rtOptsNew.procPtRectCorners[*gHndCltData.curCamId()].size();
				char tmpBuf[3] = {0, 0, 0};
				for (uint8_t x = 1; x <= fcapconstants::PROC_PT_RECTCORNERS_MAX; x++) {
					tmpBuf[0] = 'a' + (x - 1);
					tmpBuf[1] = 'x';
					jsonObj["procPt"]["rectCorners"][tmpBuf] = (tmpSz >= x ?
							std::to_string(gHndCltData.rtOptsNew.procPtRectCorners[*gHndCltData.curCamId()][x - 1].x) :
							"-");
					tmpBuf[1] = 'y';
					jsonObj["procPt"]["rectCorners"][tmpBuf] = (tmpSz >= x ?
							std::to_string(gHndCltData.rtOptsNew.procPtRectCorners[*gHndCltData.curCamId()][x - 1].y) :
							"-");
				}
			}
			//
			jsonObj["procRoi"] = {
					{"sizePerc", gHndCltData.rtOptsNew.procRoiSizePerc}
				};
			//
			jsonObj["procTr"] = {};
			buildJsonResult_procTrDeltaX(&jsonObj, "fixDelta", gHndCltData.rtOptsNew.procTrFixDelta);
			buildJsonResult_procTrDeltaX(&jsonObj, "dynDelta", gHndCltData.rtOptsNew.procTrDynDelta);
		} else {
			jsonObj["message"] = gHndCltData.respErrMsg;
		}
		return jsonObj.dump();
	}

	// -----------------------------------------------------------------------------

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
		float framerate = 0.0;
		char framerateBuf[32];
		uint32_t noFrameCnt = 0;

		if (pData == NULL) {
			gCbDecStreamingClientCount(gHndCltData.thrIx);
			return;
		}
		//
		resB = sendResponse(&fcapconstants::HTTP_CONTENT_TYPE_MULTIPART, NULL);
		if (! resB) {
			gCbDecStreamingClientCount(gHndCltData.thrIx);
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
			if (! haveFrame) {
				if (++noFrameCnt > 10000 / 5) {
					log(gHndCltData.thrIx, "__no frames for client - closing connection");
					break;
				}
				std::this_thread::sleep_for(std::chrono::milliseconds(5));
				continue;
			}
			if (_MEASURE_TIME_COPY) {
				timeEnd = std::chrono::steady_clock::now();
				log(gHndCltData.thrIx, "__copy frame took " +
						std::to_string(std::chrono::duration_cast<std::chrono::microseconds>(timeEnd - timeStart).count()) + " us");
			}
			noFrameCnt = 0;

			// send frame to client
			///
			/**log(gHndCltData.thrIx, "__send frame");**/
			++timeFpsFrames;
			if (! timeFpsRun) {
				timeFpsStart = std::chrono::steady_clock::now();
				timeFpsRun = true;
			} else {
				timeFpsCur = std::chrono::steady_clock::now();
				timeFpsDiffMs = std::chrono::duration_cast<std::chrono::milliseconds>(timeFpsCur - timeFpsStart).count();
				if (timeFpsDiffMs >= 10000) {
					framerate = ((float)timeFpsFrames / (float)timeFpsDiffMs) * 1000.0;
					if (gHndCltData.streamingClientId != 0) {
						gCbSetFramerateInfo(gHndCltData.streamingClientId, (uint32_t)std::round(framerate));
					}
					snprintf(framerateBuf, sizeof(framerateBuf), "%5.2f", framerate);
					log(gHndCltData.thrIx, std::string("__FPS=") + std::string(framerateBuf));
					//
					timeFpsStart = std::chrono::steady_clock::now();
					timeFpsFrames = 0;
				}
			}
			///
			if (_MEASURE_TIME_SEND) {
				timeStart = std::chrono::steady_clock::now();
			}
			resB = sendFrame(pData, bufSz);
			if (! resB) {
				break;
			}
			if (_MEASURE_TIME_SEND) {
				timeEnd = std::chrono::steady_clock::now();
				log(gHndCltData.thrIx, "__send frame took " +
						std::to_string(std::chrono::duration_cast<std::chrono::microseconds>(timeEnd - timeStart).count()) + " us");
			}
		}
		if (pData != NULL) {
			::free(pData);
		}
		//
		gCbDecStreamingClientCount(gHndCltData.thrIx);
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

}  // namespace http
