#ifndef HTTP_TCP_SERVER_HPP_
#define HTTP_TCP_SERVER_HPP_

#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string>

#include "../settings.hpp"
#include "../frame/frame_queue_jpeg.hpp"

namespace httppriv {

	struct HttpClientStc {
		uint32_t cltThrIx;
		bool isActive;

		HttpClientStc() {
			cltThrIx = 0;
			isActive = false;
		}
	};

	struct FrameQueueStc {
		uint32_t cltThrIx;
		bool isActive;
		frame::FrameQueueJpeg* pFrameQueue;
		uint32_t clientId;
		uint32_t fps;

		FrameQueueStc() {
			cltThrIx = 0;
			isActive = false;
			pFrameQueue = nullptr;
			clientId = 0;
			fps = 0;
		}
	};

	struct RunningCltsStc {
		uint32_t runningHandlersCount;
		uint32_t runningStreamsCount;
		HttpClientStc httpClients[fcapsettings::TCP_MAX_CLIENTS];
		FrameQueueStc frameQueues[fcapsettings::TCP_MAX_STREAMING_CLIENTS];

		RunningCltsStc() {
			runningHandlersCount = 0;
			runningStreamsCount = 0;
		}
	};

};  // namespace httppriv

namespace http {

	typedef uint32_t (*CbGetRunningHandlersCount)();
	typedef bool (*CbAddRunningHandler)(const uint32_t thrIx);
	typedef void (*CbRemoveRunningHandler)(const uint32_t thrIx);
	typedef bool (*CbIncStreamingClientCount)(const uint32_t thrIx, const uint32_t cid);
	typedef void (*CbDecStreamingClientCount)(const uint32_t thrIx);
	typedef void (*CbBroadcastFrameToStreamingClients)(std::vector<unsigned char> &frameJpeg);
	typedef bool (*CbGetFrameFromQueue)(const uint32_t thrIx, uint8_t** ppData, uint32_t &dataRsvdSz, uint32_t &dataSzOut);
	typedef void (*CbSetFramerateInfo)(const uint32_t cid, const uint32_t fps);
	typedef uint32_t (*CbGetFramerateInfo)(const uint32_t cid);

	class TcpServer {
		public:
			TcpServer(std::string ipAddress, uint16_t port);
			~TcpServer();
			bool isSocketOk() { return gCanListen; }
			void startListen();
			static uint32_t getRunningHandlersCount();
			static bool addRunningHandler(const uint32_t thrIx);
			static void removeRunningHandler(const uint32_t thrIx);
			static bool incStreamingClientCount(const uint32_t thrIx, const uint32_t cid);
			static void decStreamingClientCount(const uint32_t thrIx);
			static void broadcastFrameToStreamingClients(std::vector<unsigned char> &frameJpeg);
			static bool getFrameFromQueueForClient(const uint32_t thrIx, uint8_t** ppData, uint32_t &dataRsvdSz, uint32_t &dataSzOut);
			static void setFramerateInfo(const uint32_t cid, const uint32_t fps);
			static uint32_t getFramerateInfo(const uint32_t cid);

		private:
			static httppriv::RunningCltsStc gThrVarRunningCltsStc;
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

};  // namespace http

#endif  // HTTP_TCP_SERVER_HPP_
