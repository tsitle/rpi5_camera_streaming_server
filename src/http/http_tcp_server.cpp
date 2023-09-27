#include <chrono>
#include <iostream>
#include <sstream>
#include <unistd.h>  // ::close()
#include <sys/socket.h>  // ::shutdown(), ::accept(), ::setsockopt, SO_LINGER, struct linger
#include <fcntl.h>  // ::fcntl(), F_GETFL, O_NONBLOCK
#include <netdb.h>  // getprotobyname(), struct protoent

#include "../shared.hpp"
#include "http_tcp_server.hpp"
#include "http_clienthandler.hpp"

using namespace std::chrono_literals;

namespace http {

	RunningCltsStc TcpServer::gThrVarRunningCltsStc;
	std::mutex TcpServer::gThrMtxRunningCltHnds;

	// -----------------------------------------------------------------------------
	// -----------------------------------------------------------------------------

	TcpServer::TcpServer(std::string ipAddress, uint16_t port) :
			gServerIpAddr(ipAddress),
			gServerPort(port),
			gServerSocket(),
			gServerSocketAddress(),
			gServerLenSocketAddr(sizeof(gServerSocketAddress)),
			gThreadCount(0),
			gCanListen(false) {
		initRunningHandlersStc();
		//
		gServerSocketAddress.sin_family = AF_INET;
		gServerSocketAddress.sin_port = htons(gServerPort);
		gServerSocketAddress.sin_addr.s_addr = ::inet_addr(gServerIpAddr.c_str());

		gCanListen = startServer();
		if (! gCanListen) {
			log("Failed to start server with PORT: " + std::to_string(ntohs(gServerSocketAddress.sin_port)));
		}
	}

	TcpServer::~TcpServer() {
		closeServer();
	}

	void TcpServer::startListen() {
		bool needToStop = false;
		int32_t newSocket;
		uint32_t thrIx;

		if (! gCanListen) {
			exitWithError("cannot listen to socket");
			return;
		}
		if (::listen(gServerSocket, 20) < 0) {
			exitWithError("Socket listen failed");
			return;
		}

		std::ostringstream ss;
		ss << "*** Listening on address: "
				<< ::inet_ntoa(gServerSocketAddress.sin_addr)
				<< " PORT: " << ntohs(gServerSocketAddress.sin_port) << " ***";
		log(ss.str());

		while (true) {
			// check if we need to stop
			needToStop = fcapshared::Shared::getFlagNeedToStop();
			if (needToStop) {
				break;
			}

			/**log("Waiting for a new connection...");**/
			newSocket = acceptConnection();
			if (newSocket < 0) {
				continue;
			}

			//
			/**log("Starting new handler thread...");**/
			thrIx = ++gThreadCount;
			ClientHandler::startThread(
					thrIx,
					newSocket,
					addRunningHandler,
					removeRunningHandler,
					incStreamingClientCount,
					decStreamingClientCount,
					getFrameFromQueueForClient
				);
		}
	}

	void TcpServer::initRunningHandlersStc() {
		std::unique_lock<std::mutex> thrLock{gThrMtxRunningCltHnds, std::defer_lock};

		thrLock.lock();
		gThrVarRunningCltsStc.runningHandlersCount = 0;
		gThrVarRunningCltsStc.runningStreamsCount = 0;
		for (uint32_t x = 0; x < fcapsettings::SETT_MAX_STREAMING_CLIENTS; x++) {
			gThrVarRunningCltsStc.frameQueues[x].cltThrIx = 0;
			gThrVarRunningCltsStc.frameQueues[x].isActive = false;
			gThrVarRunningCltsStc.frameQueues[x].pFrameQueue = NULL;
		}
		thrLock.unlock();
	}

	uint32_t TcpServer::getRunningHandlersCount() {
		uint32_t resI;
		std::unique_lock<std::mutex> thrLock{gThrMtxRunningCltHnds, std::defer_lock};

		thrLock.lock();
		resI = gThrVarRunningCltsStc.runningHandlersCount;
		thrLock.unlock();
		return resI;
	}

	void TcpServer::addRunningHandler(const uint32_t thrIx) {
		std::unique_lock<std::mutex> thrLock{gThrMtxRunningCltHnds, std::defer_lock};

		thrLock.lock();
		++gThrVarRunningCltsStc.runningHandlersCount;
		for (uint32_t x = 0; x < fcapsettings::SETT_MAX_STREAMING_CLIENTS; x++) {
			if (gThrVarRunningCltsStc.frameQueues[x].isActive) {
				continue;
			}
			gThrVarRunningCltsStc.frameQueues[x].cltThrIx = thrIx;
			gThrVarRunningCltsStc.frameQueues[x].isActive = true;
			gThrVarRunningCltsStc.frameQueues[x].pFrameQueue = new frame::FrameQueueJpeg();
			break;
		}
		thrLock.unlock();
	}

	void TcpServer::removeRunningHandler(const uint32_t thrIx) {
		std::unique_lock<std::mutex> thrLock{gThrMtxRunningCltHnds, std::defer_lock};

		thrLock.lock();
		if (gThrVarRunningCltsStc.runningHandlersCount != 0) {
			--gThrVarRunningCltsStc.runningHandlersCount;
		}
		for (uint32_t x = 0; x < fcapsettings::SETT_MAX_STREAMING_CLIENTS; x++) {
			if (! gThrVarRunningCltsStc.frameQueues[x].isActive || gThrVarRunningCltsStc.frameQueues[x].cltThrIx != thrIx) {
				continue;
			}
			gThrVarRunningCltsStc.frameQueues[x].cltThrIx = 0;
			gThrVarRunningCltsStc.frameQueues[x].isActive = false;
			delete gThrVarRunningCltsStc.frameQueues[x].pFrameQueue;
			break;
		}
		thrLock.unlock();
	}

	bool TcpServer::incStreamingClientCount() {
		bool resB;
		std::unique_lock<std::mutex> thrLock{gThrMtxRunningCltHnds, std::defer_lock};

		thrLock.lock();
		resB = (gThrVarRunningCltsStc.runningStreamsCount < fcapsettings::SETT_MAX_STREAMING_CLIENTS);
		if (resB) {
			++gThrVarRunningCltsStc.runningStreamsCount;
		}
		thrLock.unlock();
		return resB;
	}

	void TcpServer::decStreamingClientCount() {
		std::unique_lock<std::mutex> thrLock{gThrMtxRunningCltHnds, std::defer_lock};

		thrLock.lock();
		if (gThrVarRunningCltsStc.runningStreamsCount != 0) {
			--gThrVarRunningCltsStc.runningStreamsCount;
		}
		thrLock.unlock();
	}

	void TcpServer::broadcastFrameToStreamingClients(std::vector<unsigned char> &frameJpeg) {
		std::unique_lock<std::mutex> thrLock{gThrMtxRunningCltHnds, std::defer_lock};

		thrLock.lock();
		for (uint32_t x = 0; x < fcapsettings::SETT_MAX_STREAMING_CLIENTS; x++) {
			if (! gThrVarRunningCltsStc.frameQueues[x].isActive) {
				continue;
			}
			gThrVarRunningCltsStc.frameQueues[x].pFrameQueue->appendFrameToQueue(frameJpeg);
		}
		thrLock.unlock();
	}

	bool TcpServer::getFrameFromQueueForClient(const uint32_t thrIx, uint8_t** ppData, uint32_t &dataRsvdSz, uint32_t &dataSzOut) {
		bool resB = false;
		std::unique_lock<std::mutex> thrLock{gThrMtxRunningCltHnds, std::defer_lock};

		thrLock.lock();
		for (uint32_t x = 0; x < fcapsettings::SETT_MAX_STREAMING_CLIENTS; x++) {
			if (! gThrVarRunningCltsStc.frameQueues[x].isActive || gThrVarRunningCltsStc.frameQueues[x].cltThrIx != thrIx) {
				continue;
			}
			resB = gThrVarRunningCltsStc.frameQueues[x].pFrameQueue->getFrameFromQueue(ppData, dataRsvdSz, dataSzOut);
			break;
		}
		thrLock.unlock();
		return resB;
	}

	// -----------------------------------------------------------------------------
	// -----------------------------------------------------------------------------

	bool TcpServer::startServer() {
		const uint8_t MAX_TRIES = 20;
		uint8_t tries = MAX_TRIES;

		//
		struct protoent *pPeTcp = ::getprotobyname("TCP");
		if (pPeTcp == NULL) {
			exitWithError("Cannot get proto name for TCP");
			return false;
		}

		gServerSocket = ::socket(AF_INET, SOCK_STREAM, pPeTcp->p_proto);
		if (gServerSocket < 0) {
			exitWithError("Cannot create socket");
			return false;
		}

		// set socket options
		const int value1 = 1;
		struct linger sl;
		sl.l_onoff = 1;  // non-zero value enables linger option in kernel
		sl.l_linger = 0;  // timeout interval in seconds
		::setsockopt(gServerSocket, SOL_SOCKET, SO_LINGER, &sl, sizeof(sl));
		::setsockopt(gServerSocket, SOL_SOCKET, SO_REUSEADDR, &value1, sizeof(int));
		::setsockopt(gServerSocket, SOL_SOCKET, SO_REUSEPORT, &value1, sizeof(int));

		// bind socket to address
		while (tries-- != 0) {
			if (::bind(gServerSocket, (sockaddr*)&gServerSocketAddress, gServerLenSocketAddr) >= 0) {
				break;
			}
			if (tries != 0) {
				/**
				 * with the socket options SO_LINGER + SO_REUSEADDR + SO_REUSEPORT this should
				 * actually not happen...
				 */
				log("address blocked (TCP port " + std::to_string(gServerPort) + "). waiting...");
				std::this_thread::sleep_for(std::chrono::milliseconds(5000));
			}
			// check if we need to stop
			if (fcapshared::Shared::getFlagNeedToStop()) {
				break;
			}
		}
		if (tries == 0) {
			exitWithError("Cannot connect socket to address");
			return false;
		}

		// make socket non-blocking
		int32_t flags = ::fcntl(gServerSocket, F_GETFL);
		::fcntl(gServerSocket, F_SETFL, flags | O_NONBLOCK);

		return true;
	}

	void TcpServer::closeServer() {
		const uint32_t BUFFER_SIZE = 16 * 1024;
		char buffer[BUFFER_SIZE] = {0};
		int32_t bytesReceived;

		::shutdown(gServerSocket, SHUT_RDWR);
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
		while ((bytesReceived = ::read(gServerSocket, buffer, BUFFER_SIZE)) > 0) {
			// read until no more data arrives
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
		}
		::close(gServerSocket);
	}

	int32_t TcpServer::acceptConnection() {
		int32_t new_socket = ::accept(gServerSocket, (sockaddr*)&gServerSocketAddress, &gServerLenSocketAddr);
		if (new_socket < 0) {
			/*
			exitWithError("Server failed to accept incoming connection");
			*/
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
			return -1;
		}
		return new_socket;
	}

	void TcpServer::log(const std::string &message) {
		std::cout << "HTTP: " << message << std::endl;
	}

	void TcpServer::exitWithError(const std::string &errorMessage) {
		log("ERROR: " + errorMessage);
		fcapshared::Shared::setFlagNeedToStop();
	}

}  // namespace http
