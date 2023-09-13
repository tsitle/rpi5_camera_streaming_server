#ifndef HTTP_TCP_SERVER_HPP_
#define HTTP_TCP_SERVER_HPP_

#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string>

#include "../settings.hpp"
#include "../frame/frame_queue.hpp"

namespace http {

	struct FrameQueueStc {
		uint32_t cltThrIx;
		bool isActive;
		frame::FrameQueueJpeg* pFrameQueue;
	};

	struct RunningCltsStc {
		uint32_t runningHandlersCount;
		uint32_t runningStreamsCount;
		FrameQueueStc frameQueues[fcapsettings::SETT_MAX_STREAMING_CLIENTS];
	};

	typedef uint32_t (*CbGetRunningHandlersCount)();
	typedef void (*CbAddRunningHandler)(const uint32_t thrIx);
	typedef void (*CbRemoveRunningHandler)(const uint32_t thrIx);
	typedef bool (*CbIncStreamingClientCount)();
	typedef void (*CbDecStreamingClientCount)();
	typedef void (*CbBroadcastFrameToStreamingClients)(std::vector<unsigned char> &frameJpeg);
	typedef bool (*CbGetFrameFromQueue)(const uint32_t thrIx, uint8_t** ppData, uint32_t &dataRsvdSz, uint32_t &dataSzOut);

	class TcpServer {
		public:
			TcpServer(std::string ipAddress, uint16_t port);
			~TcpServer();
			bool isSocketOk() { return gCanListen; }
			void startListen();
			static void initRunningHandlersStc();
			static uint32_t getRunningHandlersCount();
			static void addRunningHandler(const uint32_t thrIx);
			static void removeRunningHandler(const uint32_t thrIx);
			static bool incStreamingClientCount();
			static void decStreamingClientCount();
			static void broadcastFrameToStreamingClients(std::vector<unsigned char> &frameJpeg);
			static bool getFrameFromQueueForClient(const uint32_t thrIx, uint8_t** ppData, uint32_t &dataRsvdSz, uint32_t &dataSzOut);

		private:
			static RunningCltsStc gThrVarRunningCltsStc;
			static std::mutex gThrMtxRunningCltHnds;
			//
			std::string gServerIpAddr;
			uint16_t gServerPort;
			int32_t gServerSocket;
			struct sockaddr_in gServerSocketAddress;
			uint32_t gServerLenSocketAddr;
			uint32_t gThreadCount;
			bool gCanListen;

			//

			bool startServer();
			void closeServer();
			int32_t acceptConnection();
			static void log(const std::string &message);
			static void exitWithError(const std::string &errorMessage);
	};

}  // namespace http

#endif  // HTTP_TCP_SERVER_HPP_
