#ifndef HTTP_CLIENTHANDLER_HPP_
#define HTTP_CLIENTHANDLER_HPP_

#include <stdio.h>
#include <stdlib.h>
#include <string>

namespace http {

	class ClientHandler {
		public:
			ClientHandler(const unsigned int thrIx, const int socket);
			~ClientHandler();
			static std::thread startThread(const unsigned int thrIx, const int socket);

		private:
			unsigned int gThrIx;
			int gClientSocket;
			std::string gRespMultipartPrefix;

			//

			static void _startThread_internal(const unsigned int thrIx, const int socket);
			static void log(const unsigned int thrIx, const std::string &message);
			void handleRequest(const char *buffer, const unsigned int bufSz);
			std::string buildResponse(const unsigned int httpStatusCode, const std::string* pHttpContentType, const std::string* pContent);
			bool sendResponse(const unsigned int httpStatusCode, const std::string* pHttpContentType, const std::string* pContent);
			void startStreaming();
			bool sendFrame(unsigned char* pData, const unsigned int bufferSz);
	};

}  // namespace http

#endif  // HTTP_CLIENTHANDLER_HPP_
