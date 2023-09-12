#include <chrono>
#include <iostream>
#include <sstream>
#include <sys/socket.h>  // ::setsockopt()
#include <unistd.h>  // ::write()
#include <opencv2/opencv.hpp>

#include "../shared.hpp"
#include "../constants.hpp"
#include "../settings.hpp"
#include "../httpparser/httprequestparser.hpp"
#include "../httpparser/request.hpp"
#include "../httpparser/urlparser.hpp"
#include "http_clienthandler.hpp"

using namespace std::chrono_literals;

namespace http {

	const uint32_t BUFFER_SIZE = 32 * 1024;

	const std::string URL_PSEUDO_HOST = "http://pseudohost";

	const std::string SERVER_NAME = "HttpCamServer";
	const std::string SERVER_VERSION = "0.1";

	const std::string URL_PATH_ROOT = "/";
	const std::string URL_PATH_STREAM = "/stream.mjpeg";
	const std::string URL_PATH_FAVICON = "/favicon.ico";
	const std::string URL_PATH_ENABLE_CAM_L = "/enable_cam_l";
	const std::string URL_PATH_ENABLE_CAM_R = "/enable_cam_r";
	const std::string URL_PATH_DISABLE_CAM_L = "/disable_cam_l";
	const std::string URL_PATH_DISABLE_CAM_R = "/disable_cam_r";

	// -----------------------------------------------------------------------------
	// -----------------------------------------------------------------------------

	ClientHandler::ClientHandler(
			const uint32_t thrIx,
			const int socket,
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
		int bytesReceived = ::read(socket, buffer, BUFFER_SIZE);
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
			const int socket,
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
			const int socket,
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
		bool success = false;
		bool startStream = false;
		bool returnJson = false;
		uint32_t resHttpStat = 500;
		std::ostringstream resHttpMsgStream;
		std::string resHttpMsgString;
		std::string requFullUri;
		httpparser::Request request;
		httpparser::HttpRequestParser requparser;
		httpparser::UrlParser urlparser;
		fcapshared::RuntimeOptionsStc opts = fcapshared::Shared::getRuntimeOptions();
		fcapconstants::OutputCamsEn optsOutputCamsVal = opts.outputCams;
		bool isNewStreamingClientAccepted;

		httpparser::HttpRequestParser::ParseResult res = requparser.parse(request, buffer, buffer + bufSz);

		if (res != httpparser::HttpRequestParser::ParsingCompleted) {
			log(gThrIx, "Parsing failed");
			return;
		}

		/**log(gThrIx, request.inspect());**/
		requFullUri = URL_PSEUDO_HOST + request.uri;
		if (! urlparser.parse(requFullUri)) {
			/**log(gThrIx, "404 invalid path '" + request.uri + "'");**/
			log(gThrIx, "404 invalid path");
			resHttpStat = 404;
		} else if (urlparser.path().compare(URL_PATH_ROOT) == 0) {
			log(gThrIx, "200 Path=" + urlparser.path());
			resHttpMsgStream << buildWebsite();
			success = true;
		} else if (urlparser.path().compare(URL_PATH_STREAM) == 0) {
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
		} else if (urlparser.path().compare(URL_PATH_ENABLE_CAM_L) == 0) {
			log(gThrIx, "200 Path=" + urlparser.path());
			resHttpMsgStream << "dummy";  // won't actually be sent
			if (opts.outputCams == fcapconstants::OutputCamsEn::CAM_R) {
				optsOutputCamsVal = fcapconstants::OutputCamsEn::CAM_BOTH;
			}
			success = true;
			returnJson = true;
		} else if (urlparser.path().compare(URL_PATH_ENABLE_CAM_R) == 0) {
			log(gThrIx, "200 Path=" + urlparser.path());
			resHttpMsgStream << "dummy";  // won't actually be sent
			if (opts.outputCams == fcapconstants::OutputCamsEn::CAM_L) {
				optsOutputCamsVal = fcapconstants::OutputCamsEn::CAM_BOTH;
			}
			success = true;
			returnJson = true;
		} else if (urlparser.path().compare(URL_PATH_DISABLE_CAM_L) == 0) {
			log(gThrIx, "200 Path=" + urlparser.path());
			resHttpMsgStream << "dummy";  // won't actually be sent
			if (opts.outputCams == fcapconstants::OutputCamsEn::CAM_BOTH) {
				optsOutputCamsVal = fcapconstants::OutputCamsEn::CAM_R;
				success = true;
			} else {
				success = false;
			}
			returnJson = true;
		} else if (urlparser.path().compare(URL_PATH_DISABLE_CAM_R) == 0) {
			log(gThrIx, "200 Path=" + urlparser.path());
			resHttpMsgStream << "dummy";  // won't actually be sent
			if (opts.outputCams == fcapconstants::OutputCamsEn::CAM_BOTH) {
				optsOutputCamsVal = fcapconstants::OutputCamsEn::CAM_L;
				success = true;
			} else {
				success = false;
			}
			returnJson = true;
		} else if (urlparser.path().compare(URL_PATH_FAVICON) == 0) {
			log(gThrIx, "200 Path=" + urlparser.path());
			resHttpStat = 404;
		} else {
			/**log(gThrIx, "404 invalid path '" + urlparser.path() + "'");**/
			/**/log(gThrIx, "404 invalid query '" + urlparser.query() + "'");/**/
			log(gThrIx, "404 invalid path");
			resHttpStat = 404;
		}

		resHttpMsgString = resHttpMsgStream.str();
		if (success || (! success && returnJson)) {
			resHttpStat = 200;
		}

		//
		if (success && startStream) {
			startStreaming();
		} else if (success && returnJson) {
			if (optsOutputCamsVal != opts.outputCams) {
				fcapshared::Shared::setRuntimeOptions_outputCams(optsOutputCamsVal);
			}
			//
			resHttpMsgString = "{\"result\":\"success\"}";
			sendResponse(resHttpStat, &fcapconstants::HTTP_CONTENT_TYPE_JSON, &resHttpMsgString);
		} else if (! success && returnJson) {
			resHttpMsgString = "{\"result\":\"error\"}";
			sendResponse(resHttpStat, &fcapconstants::HTTP_CONTENT_TYPE_JSON, &resHttpMsgString);
		} else {
			sendResponse(resHttpStat, &fcapconstants::HTTP_CONTENT_TYPE_HTML, &resHttpMsgString);
		}
	}

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
						<< "<a href='" << URL_PATH_ENABLE_CAM_L << "'>Enable left camera</a>"
					<< "</p>";
		} else {
			resS
					<< "<p>"
						<< "<a href='" << URL_PATH_DISABLE_CAM_L << "'>Disable left camera</a>"
					<< "</p>";
		}
		if (opts.outputCams == fcapconstants::OutputCamsEn::CAM_L) {
			resS
					<< "<p>"
						<< "<a href='" << URL_PATH_ENABLE_CAM_R << "'>Enable right camera</a>"
					<< "</p>";
		} else {
			resS
					<< "<p>"
						<< "<a href='" << URL_PATH_DISABLE_CAM_R << "'>Disable right camera</a>"
					<< "</p>";
		}
		resS
					<< "<div style='margin-top:20px'>"
						<< "<img src='" << URL_PATH_STREAM << "' width='800' height='450' />"
					<< "</div>"
				<< "</body></html>";
		return resS.str();
	}

	std::string ClientHandler::buildResponse(const uint32_t httpStatusCode, const std::string* pHttpContentType, const std::string* pContent) {
		std::string httpStatus = "HTTP/1.1 ";
		switch (httpStatusCode) {
			case 200:
				httpStatus += "200 OK";
				break;
			case 404:
				httpStatus += "404 Not Found";
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

	bool ClientHandler::sendResponse(const uint32_t httpStatusCode, const std::string* pHttpContentType, const std::string* pContent) {
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

	bool ClientHandler::sendFrame(uint8_t* pData, const uint32_t bufferSz) {
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

}  // namespace http
