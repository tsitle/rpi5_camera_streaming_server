#ifndef INCLUDED_HTTP_TCP_SERVER
#define INCLUDED_HTTP_TCP_SERVER

#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string>

namespace http {

	class TcpServer {
		public:
			TcpServer(std::string ipAddress, int port);
			~TcpServer();
			bool isSocketOk() { return gCanListen; }
			void startListen();

		private:
			std::string gServerIpAddr;
			int gServerPort;
			int gServerSocket;
			long gIncomingMsg;
			struct sockaddr_in gServerSocketAddress;
			unsigned int gServerLenSocketAddr;
			unsigned int gThreadCount;
			bool gCanListen;

			//

			bool startServer();
			void closeServer();
			int acceptConnection();
			static void log(const std::string &message);
			static void exitWithError(const std::string &errorMessage);
	};

}  // namespace http

#endif
