#include <chrono>
#include <iostream>
#include <sstream>
#include <string>  // for stoi()
#include <sys/socket.h>  // ::setsockopt()
#include <unistd.h>  // ::write()
#include <opencv2/opencv.hpp>

#include "../shared.hpp"
#include "../constants.hpp"
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

	const std::string SERVER_NAME = "HttpCamServer";
	const std::string SERVER_VERSION = "0.1";

	const std::string URL_PATH_ROOT = "/";
	const std::string URL_PATH_STREAM = "/stream.mjpeg";
	const std::string URL_PATH_FAVICON = "/favicon.ico";
	const std::string URL_PATH_OUTPUT_CAMS_ENABLE = "/output/cams/enable";
	const std::string URL_PATH_OUTPUT_CAMS_DISABLE = "/output/cams/disable";
	const std::string URL_PATH_OUTPUT_CAMS_SWAP = "/output/cams/swap";
	const std::string URL_PATH_PROC_BNC_BRIGHTN = "/proc/bnc/brightness";
	const std::string URL_PATH_PROC_BNC_CONTRAST = "/proc/bnc/contrast";
	const std::string URL_PATH_PROC_CAL_SHOWCHESSCORNERS = "/proc/cal/showchesscorners";
	const std::string URL_PATH_PROC_CAL_RESET = "/proc/cal/reset";
	const std::string URL_PATH_PROC_PT_RECTCORNER = "/proc/pt/rect_corner";
	const std::string URL_PATH_PROC_PT_RESET = "/proc/pt/reset";
	const std::string URL_PATH_STATUS = "/status";

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
				gThrIx(thrIx),
				gClientSocket(socket),
				gCbIncStreamingClientCount(cbIncStreamingClientCount),
				gCbDecStreamingClientCount(cbDecStreamingClientCount),
				gCbGetFrameFromQueue(cbGetFrameFromQueue) {
		gStaticOptionsStc = fcapcfgfile::CfgFile::getStaticOptions();

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
		/**log(gThrIx, "read");**/
		::setsockopt(socket, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof(tv));
		::setsockopt(socket, SOL_SOCKET, SO_SNDTIMEO, (const char*)&tv, sizeof(tv));
		int32_t bytesReceived = ::read(socket, buffer, BUFFER_SIZE);
		if (bytesReceived < 0) {
			log(gThrIx, "Failed to read bytes from client socket connection");
		} else {
			/**log(gThrIx, "------ Received Request from client ------");**/
			handleRequest(buffer, (uint32_t)bytesReceived);
		}

		//
		cbRemoveRunningHandler(thrIx);
	}

	ClientHandler::~ClientHandler() {
		/*log(gThrIx, "close socket");**/
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

	void ClientHandler::handleRequest(const char *buffer, const uint32_t bufSz) {
		bool methOk;
		bool methIsGet;
		bool methIsOptions;
		bool requUriOk = false;
		bool success = false;
		bool startStream = false;
		bool returnJson = false;
		uint32_t resHttpStat = 500;
		std::ostringstream resHttpMsgStream;
		std::string resHttpMsgString;
		std::string requFullUri;
		std::string requUriPath;
		httpparser::Request request;
		httpparser::HttpRequestParser requparser;
		httpparser::UrlParser urlparser;
		fcapshared::RuntimeOptionsStc optsCur = fcapshared::Shared::getRuntimeOptions();
		fcapshared::RuntimeOptionsStc optsNew = optsCur;
		fcapconstants::CamIdEn* pCurCamId = (
				optsCur.outputCams == fcapconstants::OutputCamsEn::CAM_L ?
					&gStaticOptionsStc.camL :
					(optsCur.outputCams == fcapconstants::OutputCamsEn::CAM_R ?
						&gStaticOptionsStc.camR : NULL));
		bool isNewStreamingClientAccepted;
		bool hasChangedOptsNewProcCalDoReset = false;
		bool hasChangedOptsNewProcPtRectCorners = false;

		httpparser::HttpRequestParser::ParseResult requParseRes = requparser.parse(request, buffer, buffer + bufSz);

		if (requParseRes != httpparser::HttpRequestParser::ParsingCompleted) {
			log(gThrIx, "Parsing failed");
			return;
		}

		/**log(gThrIx, request.inspect());**/
		methIsGet = (request.method.compare("GET") == 0);
		methIsOptions = (request.method.compare("OPTIONS") == 0);
		methOk = (methIsGet || methIsOptions);
		if (! methOk) {
			log(gThrIx, "405 Method Not Allowed");
			resHttpStat = 405;
		}

		requFullUri = URL_PSEUDO_HOST + request.uri;

		if (methOk && ! urlparser.parse(requFullUri)) {
			/**log(gThrIx, "404 invalid path '" + request.uri + "'");**/
			log(gThrIx, "404 invalid path");
			resHttpStat = 404;
		} else if (methOk) {
			requUriPath = urlparser.path();
			requUriOk = (! requUriPath.empty());
			if (requUriOk) {
				gRequQuery = urlparser.query();
			}
		}
		
		if (methIsGet && requUriOk && requUriPath.compare(URL_PATH_ROOT) == 0) {
			log(gThrIx, "200 Path=" + urlparser.path());
			resHttpMsgStream << buildWebsite();
			success = true;
		} else if (methIsGet && requUriOk && requUriPath.compare(URL_PATH_STREAM) == 0) {
			isNewStreamingClientAccepted = gCbIncStreamingClientCount();
			if (! isNewStreamingClientAccepted) {
				log(gThrIx, "500 Path=" + urlparser.path());
				log(gThrIx, "__cannot accept new streaming clients at the moment");
				resHttpMsgStream << "too many clients";
			} else {
				log(gThrIx, "200 Path=" + urlparser.path());
				resHttpMsgStream << "dummy";  // won't actually be sent
				success = true;
				startStream = true;
			}
		} else if (methIsGet && requUriOk && requUriPath.compare(URL_PATH_OUTPUT_CAMS_ENABLE) == 0) {
			log(gThrIx, "200 Path=" + urlparser.path());
			resHttpMsgStream << "dummy";  // won't actually be sent
			fcapconstants::OutputCamsEn tmpVal = fcapconstants::OutputCamsEn::CAM_BOTH;
			success = getOutputCamsFromQuery(tmpVal);
			if (success) {
				if (tmpVal == fcapconstants::OutputCamsEn::CAM_L &&
						optsCur.outputCams == fcapconstants::OutputCamsEn::CAM_R &&
						isCameraAvailabelL()) {
					optsNew.outputCams = fcapconstants::OutputCamsEn::CAM_BOTH;
				} else if (tmpVal == fcapconstants::OutputCamsEn::CAM_R &&
						optsCur.outputCams == fcapconstants::OutputCamsEn::CAM_L &&
						isCameraAvailabelR()) {
					optsNew.outputCams = fcapconstants::OutputCamsEn::CAM_BOTH;
				} else if (tmpVal == fcapconstants::OutputCamsEn::CAM_BOTH &&
						isCameraAvailabelL() && isCameraAvailabelR()) {
					optsNew.outputCams = fcapconstants::OutputCamsEn::CAM_BOTH;
				} else {
					gRespErrMsg = "cannot enable camera";
					success = false;
				}
			}
			returnJson = true;
		} else if (methIsGet && requUriOk && requUriPath.compare(URL_PATH_OUTPUT_CAMS_DISABLE) == 0) {
			log(gThrIx, "200 Path=" + urlparser.path());
			resHttpMsgStream << "dummy";  // won't actually be sent
			fcapconstants::OutputCamsEn tmpVal = fcapconstants::OutputCamsEn::CAM_BOTH;
			success = getOutputCamsFromQuery(tmpVal);
			if (success) {
				if (tmpVal == fcapconstants::OutputCamsEn::CAM_L) {
					switch (optsCur.outputCams) {
						case fcapconstants::OutputCamsEn::CAM_L:
							gRespErrMsg = "cannot disable only active camera";
							success = false;
							break;
						case fcapconstants::OutputCamsEn::CAM_R:
							break;
						default:  // BOTH
							optsNew.outputCams = fcapconstants::OutputCamsEn::CAM_R;
					}
				} else if (tmpVal == fcapconstants::OutputCamsEn::CAM_R) {
					switch (optsCur.outputCams) {
						case fcapconstants::OutputCamsEn::CAM_L:
							break;
						case fcapconstants::OutputCamsEn::CAM_R:
							gRespErrMsg = "cannot disable only active camera";
							success = false;
							break;
						default:  // BOTH
							optsNew.outputCams = fcapconstants::OutputCamsEn::CAM_L;
					}
				} else {
					gRespErrMsg = "cannot disable both cameras";
					success = false;
				}
			}
			returnJson = true;
		} else if (methIsGet && requUriOk && requUriPath.compare(URL_PATH_OUTPUT_CAMS_SWAP) == 0) {
			log(gThrIx, "200 Path=" + urlparser.path());
			resHttpMsgStream << "dummy";  // won't actually be sent
			if (optsCur.outputCams == fcapconstants::OutputCamsEn::CAM_L && isCameraAvailabelR()) {
				optsNew.outputCams = fcapconstants::OutputCamsEn::CAM_R;
				success = true;
			} else if (optsCur.outputCams == fcapconstants::OutputCamsEn::CAM_R && isCameraAvailabelL()) {
				optsNew.outputCams = fcapconstants::OutputCamsEn::CAM_L;
				success = true;
			} else {
				gRespErrMsg = "cannot swap cameras";
			}
			returnJson = true;
		} else if (methIsGet && requUriOk && requUriPath.compare(URL_PATH_PROC_BNC_BRIGHTN) == 0) {
			log(gThrIx, "200 Path=" + urlparser.path());
			resHttpMsgStream << "dummy";  // won't actually be sent
			success = getIntFromQuery(
					optsNew.procBncAdjBrightness,
					fcapconstants::PROC_BNC_MIN_ADJ_BRIGHTNESS,
					fcapconstants::PROC_BNC_MAX_ADJ_BRIGHTNESS
				);
			returnJson = true;
		} else if (methIsGet && requUriOk && requUriPath.compare(URL_PATH_PROC_BNC_CONTRAST) == 0) {
			log(gThrIx, "200 Path=" + urlparser.path());
			resHttpMsgStream << "dummy";  // won't actually be sent
			success = getIntFromQuery(
					optsNew.procBncAdjContrast,
					fcapconstants::PROC_BNC_MIN_ADJ_CONTRAST,
					fcapconstants::PROC_BNC_MAX_ADJ_CONTRAST
				);
			returnJson = true;
		} else if (methIsGet && requUriOk && requUriPath.compare(URL_PATH_PROC_CAL_SHOWCHESSCORNERS) == 0) {
			log(gThrIx, "200 Path=" + urlparser.path());
			resHttpMsgStream << "dummy";  // won't actually be sent
			success = getBoolFromQuery(optsNew.procCalShowCalibChessboardPoints);
			returnJson = true;
		} else if (methIsGet && requUriOk && requUriPath.compare(URL_PATH_PROC_CAL_RESET) == 0) {
			log(gThrIx, "200 Path=" + urlparser.path());
			resHttpMsgStream << "dummy";  // won't actually be sent
			if (pCurCamId != NULL) {
				success = true;
				optsNew.procCalDoReset[*pCurCamId] = true;
				optsNew.procCalDone[*pCurCamId] = false;
				hasChangedOptsNewProcCalDoReset = true;
			} else {
				gRespErrMsg = "cannot perform reset on both cameras";
			}
			returnJson = true;
		} else if (methIsGet && requUriOk && requUriPath.compare(URL_PATH_PROC_PT_RECTCORNER) == 0) {
			log(gThrIx, "200 Path=" + urlparser.path());
			resHttpMsgStream << "dummy";  // won't actually be sent
			cv::Point tmpPoint;
			success = getCoordsFromQuery(
					tmpPoint,
					cv::Point(0, 0),
					cv::Point(gStaticOptionsStc.resolutionOutput.width - 1, gStaticOptionsStc.resolutionOutput.height - 1)
				);
			if (success && pCurCamId == NULL) {
				gRespErrMsg = "cannot set corners on both cameras";
				success = false;
			}
			if (success) {
				optsNew.procPtRectCorners[*pCurCamId].push_back(tmpPoint);
				optsNew.procPtDone[*pCurCamId] = (optsNew.procPtRectCorners[*pCurCamId].size() == fcapconstants::PROC_PT_RECTCORNERS_MAX);
				hasChangedOptsNewProcPtRectCorners = true;
			}
			returnJson = true;
		} else if (methIsGet && requUriOk && requUriPath.compare(URL_PATH_PROC_PT_RESET) == 0) {
			log(gThrIx, "200 Path=" + urlparser.path());
			resHttpMsgStream << "dummy";  // won't actually be sent
			if (pCurCamId != NULL) {
				success = true;
				optsNew.procPtRectCorners[*pCurCamId].clear();
				optsNew.procPtDone[*pCurCamId] = false;
				hasChangedOptsNewProcPtRectCorners = true;
			} else {
				gRespErrMsg = "cannot perform reset on both cameras";
			}
			returnJson = true;
		} else if (methIsGet && requUriOk && requUriPath.compare(URL_PATH_FAVICON) == 0) {
			log(gThrIx, "200 Path=" + urlparser.path());
			resHttpStat = 404;
		} else if (methIsGet && requUriOk && requUriPath.compare(URL_PATH_STATUS) == 0) {
			/**log(gThrIx, "200 Path=" + urlparser.path());**/
			resHttpMsgStream << "dummy";  // won't actually be sent
			success = true;
			returnJson = true;
		} else if (methIsGet) {
			/**log(gThrIx, "404 invalid path '" + urlparser.path() + "'");**/
			log(gThrIx, "404 invalid path");
			resHttpStat = 404;
		} else if (methIsOptions) {
			// the browser is doing a "preflight" check of this API endpoint
			/**log(gThrIx, "200 OPTIONS");**/
			resHttpMsgStream << "dummy";  // won't actually be sent
			success = true;
			returnJson = true;
		}

		resHttpMsgString = resHttpMsgStream.str();
		if (success || returnJson) {
			resHttpStat = 200;
		}

		//
		if (success && startStream) {
			startStreaming();
		} else if (returnJson) {
			if (success) {
				if (optsNew.outputCams != optsCur.outputCams) {
					fcapshared::Shared::setRuntimeOptions_outputCams(optsNew.outputCams);
				}
				if (optsNew.procBncAdjBrightness != optsCur.procBncAdjBrightness) {
					fcapshared::Shared::setRuntimeOptions_procBncAdjBrightness(optsNew.procBncAdjBrightness);
				}
				if (optsNew.procBncAdjContrast != optsCur.procBncAdjContrast) {
					fcapshared::Shared::setRuntimeOptions_procBncAdjContrast(optsNew.procBncAdjContrast);
				}
				if (optsNew.procCalShowCalibChessboardPoints != optsCur.procCalShowCalibChessboardPoints) {
					fcapshared::Shared::setRuntimeOptions_procCalShowCalibChessboardPoints(optsNew.procCalShowCalibChessboardPoints);
				}
				if (hasChangedOptsNewProcCalDoReset) {
					fcapshared::Shared::setRuntimeOptions_procCalDoReset(*pCurCamId, true);
				}
				if (hasChangedOptsNewProcPtRectCorners) {
					fcapshared::Shared::setRuntimeOptions_procPtChangedRectCorners(*pCurCamId, true);
					fcapshared::Shared::setRuntimeOptions_procPtRectCorners(*pCurCamId, optsNew.procPtRectCorners[*pCurCamId]);
				}
			} else {
				log(gThrIx, "__ERR " + gRespErrMsg);
			}
			//
			if (methIsGet) {
				resHttpMsgString = buildJsonResult(success, pCurCamId, optsNew);
			} else {
				resHttpMsgString = "";
			}
			sendResponse(resHttpStat, &fcapconstants::HTTP_CONTENT_TYPE_JSON, &resHttpMsgString);
		} else {
			sendResponse(resHttpStat, &fcapconstants::HTTP_CONTENT_TYPE_HTML, &resHttpMsgString);
		}
	}

	// -----------------------------------------------------------------------------

	std::string ClientHandler::buildWebsite() {
		std::ostringstream resS;
		fcapshared::RuntimeOptionsStc opts = fcapshared::Shared::getRuntimeOptions();

		resS
				<< "<!DOCTYPE html>"
				<< "<html lang=\"en\">"
				<< "<head>"
					<< "<title>" << SERVER_NAME << "</title>"
				<< "</head>"
				<< "<body>"
					<< "<h1>" << SERVER_NAME << "</h1>"
					<< "<p>"
						<< "<a href='" << URL_PATH_STREAM << "'>MJPEG Stream</a>"
					<< "</p>";
		if (opts.outputCams == fcapconstants::OutputCamsEn::CAM_R) {
			resS
					<< "<p>"
						<< "<a href='" << URL_PATH_OUTPUT_CAMS_ENABLE << "?L'>Enable left camera</a>"
					<< "</p>";
		} else {
			resS
					<< "<p>"
						<< "<a href='" << URL_PATH_OUTPUT_CAMS_DISABLE << "?L'>Disable left camera</a>"
					<< "</p>";
		}
		if (opts.outputCams == fcapconstants::OutputCamsEn::CAM_L) {
			resS
					<< "<p>"
						<< "<a href='" << URL_PATH_OUTPUT_CAMS_ENABLE << "?R'>Enable right camera</a>"
					<< "</p>";
		} else {
			resS
					<< "<p>"
						<< "<a href='" << URL_PATH_OUTPUT_CAMS_DISABLE << "?R'>Disable right camera</a>"
					<< "</p>";
		}
		resS
				<< "<p>"
					<< "<a href='" << URL_PATH_OUTPUT_CAMS_SWAP << "'>Swap cameras</a>"
				<< "</p>";
		resS
					<< "<div style='margin-top:20px'>"
						<< "<img src='" << URL_PATH_STREAM << "' width='800' height='450' />"
					<< "</div>"
				<< "</body></html>";
		return resS.str();
	}

	std::string ClientHandler::buildResponse(const uint32_t httpStatusCode, const std::string *pHttpContentType, const std::string *pContent) {
		std::string httpStatus = "HTTP/1.1 ";
		switch (httpStatusCode) {
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
		ss << "Server: " << SERVER_NAME << "/" << SERVER_VERSION << "\r\n";
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

	bool ClientHandler::sendResponse(const uint32_t httpStatusCode, const std::string *pHttpContentType, const std::string *pContent) {
		std::string respMsg = buildResponse(httpStatusCode, pHttpContentType, pContent);

		/**log(gThrIx, "__>> " + respMsg);**/
		long bytesSent = ::write(gClientSocket, respMsg.c_str(), respMsg.size());
		if (bytesSent != (long)respMsg.size()) {
			log(gThrIx, "__Error sending response to client");
			return false;
		}
		return true;
	}

	void ClientHandler::startStreaming() {
		#define _MEASURE_TIME_COPY  0
		#define _MEASURE_TIME_SEND  0
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

		if (pData == NULL) {
			gCbDecStreamingClientCount();
			return;
		}
		//
		resB = sendResponse(200, &fcapconstants::HTTP_CONTENT_TYPE_MULTIPART, NULL);
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
			#if _MEASURE_TIME_COPY == 1
				auto timeStart = std::chrono::steady_clock::now();
			#endif
			haveFrame = gCbGetFrameFromQueue(gThrIx, &pData, rsvdBufSz, bufSz);
			#if _MEASURE_TIME_COPY == 1
				auto timeEnd = std::chrono::steady_clock::now();
				if (haveFrame) {
					log(gThrIx, "__copy frame took " + std::to_string(std::chrono::duration_cast<std::chrono::microseconds>(timeEnd - timeStart).count()) + " us");
				}
			#endif

			// send frame to client
			if (haveFrame) {
				/**log(gThrIx, "__send frame");**/
				//
				++timeFpsFrames;
				if (! timeFpsRun) {
					timeFpsStart = std::chrono::steady_clock::now();
					timeFpsRun = true;
				} else {
					timeFpsCur = std::chrono::steady_clock::now();
					timeFpsDiffMs = std::chrono::duration_cast<std::chrono::milliseconds>(timeFpsCur - timeFpsStart).count();
					if (timeFpsDiffMs >= 10000) {
						log(gThrIx, "__FPS=" + std::to_string(((float)timeFpsFrames / (float)timeFpsDiffMs) * 1000.0));
						//
						timeFpsStart = std::chrono::steady_clock::now();
						timeFpsFrames = 0;
					}
				}
				//
				#if _MEASURE_TIME_SEND == 1
					auto timeStart = std::chrono::steady_clock::now();
				#endif
				resB = sendFrame(pData, bufSz);
				if (! resB) {
					break;
				}
				#if _MEASURE_TIME_SEND == 1
					auto timeEnd = std::chrono::steady_clock::now();
					log(gThrIx, "__send frame took " + std::to_string(std::chrono::duration_cast<std::chrono::microseconds>(timeEnd - timeStart).count()) + " us");
				#endif
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
		/**log(gThrIx, ">> " + respMsg);**/
		bytesSent = ::write(gClientSocket, respMsg.c_str(), respMsg.size());
		if (bytesSent != (long)respMsg.size()) {
			if (bytesSent == -1) {
				log(gThrIx, "__Client connection closed");
			} else {
				log(gThrIx, "__Error sending response to client #HE (sent=" + std::to_string(bytesSent) + ")");
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
		log(gThrIx, strBuf);**/
		while (remBufSz != 0) {
			if (curBufSz > remBufSz) {
				curBufSz = remBufSz;
			}
			bytesSent = ::write(gClientSocket, pStartBuf, curBufSz);
			if (bytesSent != (long)curBufSz) {
				if (bytesSent == -1) {
					log(gThrIx, "__Client connection closed");
				} else {
					log(gThrIx, "__Error sending response to client #DA (sent=" +
							std::to_string(bytesSent) + ", exp=" + std::to_string(curBufSz) + ", ts=" + std::to_string(sentTot) +
							")");
				}
				return false;
			}
			sentTot += curBufSz;
			remBufSz -= curBufSz;
			pStartBuf += curBufSz;
		}
		/**log(gThrIx, "__sent " + std::to_string(sentTot) + " total");**/
		return true;
	}

	std::string ClientHandler::buildJsonResult(const bool success, const fcapconstants::CamIdEn *pCurCamId, fcapshared::RuntimeOptionsStc &optsRt) {
		json jsonObj;

		jsonObj["result"] = (success ? "success" : "error");
		if (success) {
			fcapconstants::OutputCamsEn outputCams = optsRt.outputCams;
			bool tmpProcCalDone;
			bool tmpProcPtDone;
			std::string tmpAvail;

			if (isCameraAvailabelL() && isCameraAvailabelR()) {
				tmpAvail = "BOTH";
			} else if (isCameraAvailabelL()) {
				tmpAvail = "L";
			} else if (isCameraAvailabelR()) {
				tmpAvail = "R";
			}
			jsonObj["availOutputCams"] = tmpAvail;
			switch (outputCams) {
				case fcapconstants::OutputCamsEn::CAM_L:
					jsonObj["outputCams"] = "L";
					break;
				case fcapconstants::OutputCamsEn::CAM_R:
					jsonObj["outputCams"] = "R";
					break;
				default:
					jsonObj["outputCams"] = "BOTH";
			}
			jsonObj["resolutionOutput_w"] = gStaticOptionsStc.resolutionOutput.width;
			jsonObj["resolutionOutput_h"] = gStaticOptionsStc.resolutionOutput.height;
			jsonObj["procBncAdjBrightness"] = optsRt.procBncAdjBrightness;
			jsonObj["procBncAdjContrast"] = optsRt.procBncAdjContrast;
			if (pCurCamId == NULL) {
				tmpProcCalDone = (optsRt.procCalDone[fcapconstants::CamIdEn::CAM_0] && optsRt.procCalDone[fcapconstants::CamIdEn::CAM_1]);
				tmpProcPtDone = (optsRt.procPtDone[fcapconstants::CamIdEn::CAM_0] && optsRt.procPtDone[fcapconstants::CamIdEn::CAM_1]);
			} else {
				tmpProcCalDone = optsRt.procCalDone[*pCurCamId];
				tmpProcPtDone = optsRt.procPtDone[*pCurCamId];
			}
			jsonObj["procCalDone"] = tmpProcCalDone;
			jsonObj["procCalShowCalibChessboardPoints"] = optsRt.procCalShowCalibChessboardPoints;
			jsonObj["procPtDone"] = tmpProcPtDone;
			if (pCurCamId != NULL) {
				uint8_t tmpSz = optsRt.procPtRectCorners[*pCurCamId].size();
				for (uint8_t x = 1; x <= fcapconstants::PROC_PT_RECTCORNERS_MAX; x++) {
					jsonObj["procPtRectCorners_" + std::to_string(x) + "x"] = (tmpSz >= x ?
							std::to_string(optsRt.procPtRectCorners[*pCurCamId][x - 1].x) :
							"-");
					jsonObj["procPtRectCorners_" + std::to_string(x) + "y"] = (tmpSz >= x ?
							std::to_string(optsRt.procPtRectCorners[*pCurCamId][x - 1].y) :
							"-");
				}
			}
		} else {
			jsonObj["message"] = gRespErrMsg;
		}
		return jsonObj.dump();
	}

	// -----------------------------------------------------------------------------

	bool ClientHandler::isCameraAvailabelL() {
		if (gStaticOptionsStc.camL == fcapconstants::CamIdEn::CAM_0) {
			return (! gStaticOptionsStc.camSource0.empty());
		}
		return (! gStaticOptionsStc.camSource1.empty());
	}

	bool ClientHandler::isCameraAvailabelR() {
		if (gStaticOptionsStc.camR == fcapconstants::CamIdEn::CAM_0) {
			return (! gStaticOptionsStc.camSource0.empty());
		}
		return (! gStaticOptionsStc.camSource1.empty());
	}

	// -----------------------------------------------------------------------------

	bool ClientHandler::getBoolFromQuery(bool &valOut) {
		try {
			if (gRequQuery.empty()) {
				throw;
			}
			if (gRequQuery.compare("0") != 0 && gRequQuery.compare("1") != 0) {
				throw;
			}
			valOut = (gRequQuery.compare("1") == 0);
		} catch (std::exception& err) {
			gRespErrMsg = "invalid boolean value (allowed: 0 and 1)";
			return false;
		}
		return true;
	}

	bool ClientHandler::getIntFromQuery(int16_t &valOut, const int16_t valMin, const int16_t valMax) {
		try {
			if (gRequQuery.empty()) {
				throw;
			}
			int16_t tmpInt = stoi(gRequQuery);
			if (tmpInt < valMin || tmpInt > valMax) {
				throw;
			}
			valOut = tmpInt;
		} catch (std::exception& err) {
			gRespErrMsg = "invalid integer value (allowed: " + std::to_string(valMin) + ".." + std::to_string(valMax) + ")";
			return false;
		}
		return true;
	}

	bool ClientHandler::getOutputCamsFromQuery(fcapconstants::OutputCamsEn &valOut) {
		try {
			if (gRequQuery.empty()) {
				throw;
			}
			if (gRequQuery.compare("L") == 0) {
				valOut = fcapconstants::OutputCamsEn::CAM_L;
			} else if (gRequQuery.compare("R") == 0) {
				valOut = fcapconstants::OutputCamsEn::CAM_R;
			} else if (gRequQuery.compare("BOTH") == 0)  {
				valOut = fcapconstants::OutputCamsEn::CAM_BOTH;
			} else {
				throw;
			}
		} catch (std::exception& err) {
			gRespErrMsg = "invalid output camera (allowed: 'L', 'R', 'BOTH')";
			return false;
		}
		return true;
	}

	void ClientHandler::_stringSplit(const std::string &valIn, const std::string &split, std::string &valOut1, std::string &valOut2) {
		if (valIn.empty()) {
			throw;
		}
		size_t posIx = valIn.find(split);
		if (posIx == std::string::npos) {
			throw;
		}
		valOut1 = valIn.substr(0, posIx);
		valOut2 = valIn.substr(posIx + 1);
	}

	bool ClientHandler::getCoordsFromQuery(cv::Point &valOut, const cv::Point &valMin, const cv::Point &valMax) {
		try {
			std::string strA1;
			std::string strA1k;
			std::string strA1v;
			std::string strA2;
			std::string strA2k;
			std::string strA2v;

			_stringSplit(gRequQuery, "&", strA1, strA2);
			_stringSplit(strA1, "=", strA1k, strA1v);
			_stringSplit(strA2, "=", strA2k, strA2v);
			if (strA1k.compare("x") == 0) {
				valOut.x = stoi(strA1v);
			} else if (strA1k.compare("y") == 0) {
				valOut.y = stoi(strA1v);
			} else {
				throw;
			}
			if (strA2k.compare("x") == 0) {
				valOut.x = stoi(strA2v);
			} else if (strA2k.compare("y") == 0) {
				valOut.y = stoi(strA2v);
			} else {
				throw;
			}
			if (valOut.x < valMin.x || valOut.x > valMax.x) {
				throw;
			}
			if (valOut.y < valMin.y || valOut.y > valMax.y) {
				throw;
			}
		} catch (std::exception& err) {
			gRespErrMsg = "invalid coordinates (example: 'x=1&y=2')";
			return false;
		}
		return true;
	}

}  // namespace http
