#include <chrono>
#include <iostream>
#include <sstream>
#include <unistd.h>  // ::close()
#include <fcntl.h>

#include "shared.hpp"
#include "http_tcp_server.hpp"
#include "http_clienthandler.hpp"

using namespace std::chrono_literals;

namespace http {

	TcpServer::TcpServer(std::string ipAddress, int port) :
			gServerIpAddr(ipAddress),
			gServerPort(port),
			gServerSocket(),
			gIncomingMsg(),
			gServerSocketAddress(),
			gServerLenSocketAddr(sizeof(gServerSocketAddress)),
			gThreadCount(0),
			gCanListen(false) {
		gServerSocketAddress.sin_family = AF_INET;
		gServerSocketAddress.sin_port = ::htons(gServerPort);
		gServerSocketAddress.sin_addr.s_addr = ::inet_addr(gServerIpAddr.c_str());

		gCanListen = startServer();
		if (! gCanListen) {
			log("Failed to start server with PORT: " + std::to_string(::ntohs(gServerSocketAddress.sin_port)));
		}
	}

	TcpServer::~TcpServer() {
		closeServer();
	}

	void TcpServer::startListen() {
		std::unique_lock<std::mutex> thrLockStop{fcapshared::gThrMtxStop, std::defer_lock};
		std::unique_lock<std::mutex> thrLockRunningCltHnds{fcapshared::gThrMtxRunningCltHnds, std::defer_lock};
		bool needToStop = false;
		int newSocket;
		unsigned thrIx;

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
				<< " PORT: " << ::ntohs(gServerSocketAddress.sin_port) << " ***";
		log(ss.str());

		while (true) {
			// check if we need to stop
			thrLockStop.lock();
			if (fcapshared::gThrCondStop.wait_for(thrLockStop, 1ms, []{return fcapshared::gThrVarDoStop;})) {
				needToStop = true;
			}
			thrLockStop.unlock();
			if (needToStop) {
				break;
			}

			//
			/**log("Waiting for a new connection...");**/
			newSocket = acceptConnection();
			if (newSocket < 0) {
				continue;
			}

			// check if we need to stop
			thrLockStop.lock();
			if (fcapshared::gThrCondStop.wait_for(thrLockStop, 1ms, []{return fcapshared::gThrVarDoStop;})) {
				needToStop = true;
			}
			thrLockStop.unlock();
			if (needToStop) {
				break;
			}

			//
			/**log("Starting new handler thread...");**/
			thrIx = ++gThreadCount;
			ClientHandler::startThread(thrIx, newSocket);
		}
	}

	// -----------------------------------------------------------------------------
	// -----------------------------------------------------------------------------

	bool TcpServer::startServer() {
		gServerSocket = ::socket(AF_INET, SOCK_STREAM, 0);
		if (gServerSocket < 0) {
			exitWithError("Cannot create socket");
			return false;
		}

		if (::bind(gServerSocket, (sockaddr*)&gServerSocketAddress, gServerLenSocketAddr) < 0) {
			exitWithError("Cannot connect socket to address");
			return false;
		}

		// make socket non-blocking
		int flags = ::fcntl(gServerSocket, F_GETFL);
		::fcntl(gServerSocket, F_SETFL, flags| O_NONBLOCK);

		return true;
	}

	void TcpServer::closeServer() {
		::close(gServerSocket);
	}

	int TcpServer::acceptConnection() {
		int new_socket = ::accept(gServerSocket, (sockaddr*)&gServerSocketAddress, &gServerLenSocketAddr);
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
		std::unique_lock<std::mutex> thrLockStop{fcapshared::gThrMtxStop, std::defer_lock};

		log("ERROR: " + errorMessage);
		thrLockStop.lock();
		fcapshared::gThrVarDoStop = true;
		thrLockStop.unlock();
		fcapshared::gThrCondStop.notify_all();
	}

}  // namespace http
