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
			uint32_t gThrIx;
			int32_t gClientSocket;
			fcapcfgfile::StaticOptionsStc gStaticOptionsStc;
			std::string gRespMultipartPrefix;
			CbIncStreamingClientCount gCbIncStreamingClientCount;
			CbDecStreamingClientCount gCbDecStreamingClientCount;
			CbGetFrameFromQueue gCbGetFrameFromQueue;

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
			std::string buildWebsite();
			std::string buildResponse(const uint32_t httpStatusCode, const std::string *pHttpContentType, const std::string *pContent);
			bool sendResponse(const uint32_t httpStatusCode, const std::string *pHttpContentType, const std::string *pContent);
			std::string buildJsonResult(const bool success, const fcapconstants::CamIdEn *pCurCamId, fcapshared::RuntimeOptionsStc &optsRt);
			//
			void startStreaming();
			bool sendFrame(uint8_t *pData, const uint32_t bufferSz);
			//
			bool getBoolFromQuery(std::string query, bool &valOut);
			bool getIntFromQuery(std::string query, int16_t &valOut, const int16_t valMin, const int16_t valMax);
			bool getOutputCamsFromQuery(std::string query, fcapconstants::OutputCamsEn &valOut);
			void _stringSplit(const std::string &valIn, const std::string &split, std::string &valOut1, std::string &valOut2);
			bool getCoordsFromQuery(std::string query, cv::Point &valOut, const cv::Point &valMin, const cv::Point &valMax);
	};

}  // namespace http

#endif  // HTTP_CLIENTHANDLER_HPP_
