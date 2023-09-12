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
					const int socket,
					CbAddRunningHandler cbAddRunningHandler,
					CbRemoveRunningHandler cbRemoveRunningHandler,
					CbIncStreamingClientCount cbIncStreamingClientCount,
					CbDecStreamingClientCount cbDecStreamingClientCount,
					CbGetFrameFromQueue cbGetFrameFromQueue);
			~ClientHandler();
			static std::thread startThread(
					const uint32_t thrIx,
					const int socket,
					CbAddRunningHandler cbAddRunningHandler,
					CbRemoveRunningHandler cbRemoveRunningHandler,
					CbIncStreamingClientCount cbIncStreamingClientCount,
					CbDecStreamingClientCount cbDecStreamingClientCount,
					CbGetFrameFromQueue cbGetFrameFromQueue);

		private:
			uint32_t gThrIx;
			int gClientSocket;
			std::string gRespMultipartPrefix;
			CbIncStreamingClientCount gCbIncStreamingClientCount;
			CbDecStreamingClientCount gCbDecStreamingClientCount;
			CbGetFrameFromQueue gCbGetFrameFromQueue;

			//

			static void _startThread_internal(
					const uint32_t thrIx,
					const int socket,
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
			//
			void startStreaming();
			bool sendFrame(uint8_t* pData, const uint32_t bufferSz);
	};

}  // namespace http

#endif  // HTTP_CLIENTHANDLER_HPP_
