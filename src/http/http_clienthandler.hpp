#ifndef HTTP_CLIENTHANDLER_HPP_
#define HTTP_CLIENTHANDLER_HPP_

#include <stdio.h>
#include <stdlib.h>
#include <string>

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
			std::string buildResponse(const uint32_t httpStatusCode, const std::string* pHttpContentType, const std::string* pContent);
			bool sendResponse(const uint32_t httpStatusCode, const std::string* pHttpContentType, const std::string* pContent);
			std::string buildJsonResult(const bool success, const fcapshared::RuntimeOptionsStc &optsRt);
			//
			void startStreaming();
			bool sendFrame(uint8_t* pData, const uint32_t bufferSz);
			//
			bool getBoolFromQuery(std::string query, bool &valOut);
			bool getIntFromQuery(std::string query, int16_t &valOut, const int16_t valMin, const int16_t valMax);
			bool getOutputCamsFromQuery(std::string query, fcapconstants::OutputCamsEn &valOut);
	};

}  // namespace http

#endif  // HTTP_CLIENTHANDLER_HPP_
