#ifndef HTTP_CLIENTHANDLER_HPP_
#define HTTP_CLIENTHANDLER_HPP_

#include <stdio.h>
#include <stdlib.h>
#include <string>

#include "../cfgfile.hpp"
#include "http_tcp_server.hpp"

namespace http {

	class ClientHandler {
		public:
			ClientHandler(
					const uint32_t thrIx,
					const int32_t socket,
					CbAddRunningHandler cbAddRunningHandler,
					CbRemoveRunningHandler cbRemoveRunningHandler,
					CbIncStreamingClientCount cbIncStreamingClientCount,
					CbDecStreamingClientCount cbDecStreamingClientCount,
					CbGetFrameFromQueue cbGetFrameFromQueue);
			~ClientHandler();
			static std::thread startThread(
					const uint32_t thrIx,
					const int32_t socket,
					CbAddRunningHandler cbAddRunningHandler,
					CbRemoveRunningHandler cbRemoveRunningHandler,
					CbIncStreamingClientCount cbIncStreamingClientCount,
					CbDecStreamingClientCount cbDecStreamingClientCount,
					CbGetFrameFromQueue cbGetFrameFromQueue);

		private:
			typedef bool (ClientHandler::*HandleRouteFnc)(void);
			//
			const std::string URL_PATH_STREAM = "/stream.mjpeg";
			const std::string URL_PATH_OUTPUT_CAMS_ENABLE = "/output/cams/enable";
			const std::string URL_PATH_OUTPUT_CAMS_DISABLE = "/output/cams/disable";
			const std::string URL_PATH_OUTPUT_CAMS_SWAP = "/output/cams/swap";
			//
			std::map<const std::string, const HandleRouteFnc> HANDLEROUTE_LUT = {
					{"/", &ClientHandler::_handleRoute_ROOT},
					{URL_PATH_STREAM, &ClientHandler::_handleRoute_STREAM},
					{"/favicon.ico", &ClientHandler::_handleRoute_FAVICON},
					{URL_PATH_OUTPUT_CAMS_ENABLE, &ClientHandler::_handleRoute_OUTPUT_CAMS_ENABLE},
					{URL_PATH_OUTPUT_CAMS_DISABLE, &ClientHandler::_handleRoute_OUTPUT_CAMS_DISABLE},
					{URL_PATH_OUTPUT_CAMS_SWAP, &ClientHandler::_handleRoute_OUTPUT_CAMS_SWAP},
					{"/proc/bnc/brightness", &ClientHandler::_handleRoute_PROC_BNC_BRIGHTN},
					{"/proc/bnc/contrast", &ClientHandler::_handleRoute_PROC_BNC_CONTRAST},
					{"/proc/cal/showchesscorners", &ClientHandler::_handleRoute_PROC_CAL_SHOWCHESSCORNERS},
					{"/proc/cal/reset", &ClientHandler::_handleRoute_PROC_CAL_RESET},
					{"/proc/pt/rect_corner", &ClientHandler::_handleRoute_PROC_PT_RECTCORNER},
					{"/proc/pt/reset", &ClientHandler::_handleRoute_PROC_PT_RESET},
					{"/status", &ClientHandler::_handleRoute_STATUS}
				};
			//
			uint32_t gThrIx;
			int32_t gClientSocket;
			fcapcfgfile::StaticOptionsStc gStaticOptionsStc;
			std::string gRespMultipartPrefix;
			CbIncStreamingClientCount gCbIncStreamingClientCount;
			CbDecStreamingClientCount gCbDecStreamingClientCount;
			CbGetFrameFromQueue gCbGetFrameFromQueue;
			std::string gRequUriPath;
			std::string gRequUriQuery;
			uint32_t gRespHttpStat;
			std::string gRespHttpMsgString;
			std::string gRespErrMsg;
			bool gRespReturnJson;
			bool gIsNewStreamingClientAccepted;
			fcapshared::RuntimeOptionsStc gRtOptsCur;
			fcapshared::RuntimeOptionsStc gRtOptsNew;
			fcapconstants::CamIdEn* gPCurCamId;

			//

			static void _startThread_internal(
					const uint32_t thrIx,
					const int32_t socket,
					CbAddRunningHandler cbAddRunningHandler,
					CbRemoveRunningHandler cbRemoveRunningHandler,
					CbIncStreamingClientCount cbIncStreamingClientCount,
					CbDecStreamingClientCount cbDecStreamingClientCount,
					CbGetFrameFromQueue cbGetFrameFromQueue);
			static void log(const uint32_t thrIx, const std::string &message);
			//
			void handleRequest(const char *buffer, const uint32_t bufSz);
			bool _handleRequest_get();
			//
			bool _handleRoute_ROOT();
			bool _handleRoute_STREAM();
			bool _handleRoute_FAVICON();
			bool _handleRoute_OUTPUT_CAMS_ENABLE();
			bool _handleRoute_OUTPUT_CAMS_DISABLE();
			bool _handleRoute_OUTPUT_CAMS_SWAP();
			bool _handleRoute_PROC_BNC_BRIGHTN();
			bool _handleRoute_PROC_BNC_CONTRAST();
			bool _handleRoute_PROC_CAL_SHOWCHESSCORNERS();
			bool _handleRoute_PROC_CAL_RESET();
			bool _handleRoute_PROC_PT_RECTCORNER();
			bool _handleRoute_PROC_PT_RESET();
			bool _handleRoute_STATUS();
			//
			std::string buildWebsite();
			std::string buildResponse(const std::string *pHttpContentType, const std::string *pContent);
			bool sendResponse(const std::string *pHttpContentType, const std::string *pContent);
			std::string buildJsonResult(const bool success);
			//
			void startStreaming();
			bool sendFrame(uint8_t *pData, const uint32_t bufferSz);
			//
			bool isCameraAvailabelL();
			bool isCameraAvailabelR();
			//
			bool getBoolFromQuery(bool &valOut);
			bool getIntFromQuery(int16_t &valOut, const int16_t valMin, const int16_t valMax);
			bool getOutputCamsFromQuery(fcapconstants::OutputCamsEn &valOut);
			void _stringSplit(const std::string &valIn, const std::string &split, std::string &valOut1, std::string &valOut2);
			bool getCoordsFromQuery(cv::Point &valOut, const cv::Point &valMin, const cv::Point &valMax);
	};

}  // namespace http

#endif  // HTTP_CLIENTHANDLER_HPP_
