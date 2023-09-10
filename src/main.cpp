#include <chrono>
#include <iostream>
#include <signal.h>
#include <thread>
#include <opencv2/opencv.hpp>

#include "shared.hpp"
#include "settings.hpp"
#include "frame_producer.hpp"
#include "frame_consumer.hpp"
#include "http_tcp_server.hpp"

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------

//
bool fcapshared::gThrVarDoStop = false;
std::mutex fcapshared::gThrMtxStop;
std::condition_variable fcapshared::gThrCondStop;

//
bool fcapshared::gThrVarCamStreamsOpened = false;
std::mutex fcapshared::gThrMtxCamStreamsOpened;
std::condition_variable fcapshared::gThrCondCamStreamsOpened;

//
std::vector<std::vector<unsigned char>> fcapshared::gThrVarOutpQueue;
std::mutex fcapshared::gThrMtxOutpQu;
std::condition_variable fcapshared::gThrCondOutpQu;

//
fcapshared::RunningCltsStc fcapshared::gThrVarRunningCltsStc;
std::unordered_map<unsigned int, bool> fcapshared::gThrVarRunningCltHndsMap;
std::mutex fcapshared::gThrMtxRunningCltHnds;

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------

void log(const std::string &message) {
	std::cout << "M: " << message << std::endl;
}

void sigHandlerCtrlC(__attribute__((unused)) int s) {
	/*#define _UNUSED(x) (void)(x)
	_UNUSED(s);
	#undef _UNUSED*/

	log("Caught CTRL-C");

	std::unique_lock<std::mutex> thrLockStop{fcapshared::gThrMtxStop, std::defer_lock};
	thrLockStop.lock();
	fcapshared::gThrVarDoStop = true;
	thrLockStop.unlock();
	fcapshared::gThrCondStop.notify_all();
}

/*void sigHandlerPipe(int s) {
	log("Caught SIGPIPE");
}*/

// -----------------------------------------------------------------------------

int main() {
	fcapshared::gThrVarRunningCltsStc.runningHandlersCount = 0;
	fcapshared::gThrVarRunningCltsStc.runningStreamsCount = 0;

	//
	struct sigaction sigIntHandler;
	sigIntHandler.sa_handler = sigHandlerCtrlC;
	::sigemptyset(&sigIntHandler.sa_mask);
	sigIntHandler.sa_flags = 0;
	::sigaction(SIGINT, &sigIntHandler, NULL);

	// SIGPIPE will occur e.g. when a TCP client has closed the connection
	/*
	struct sigaction sigPipeHandler;
	sigPipeHandler.sa_handler = sigHandlerPipe;
	::sigemptyset(&sigPipeHandler.sa_mask);
	sigPipeHandler.sa_flags = 0;
	::sigaction(SIGPIPE, &sigPipeHandler, NULL);
	*/
	if (::signal(SIGPIPE, SIG_IGN) == SIG_ERR) {  // ignore SIGPIPE
		return -1;
	}

	// create HTTP server
	http::TcpServer server = http::TcpServer("0.0.0.0", (int)fcapsettings::SETT_SERVER_PORT);
	if (! server.isSocketOk()) {
		return -1;
	}

	// start frame producer thread
	std::thread threadProdObj = fprod::FrameProducer::startThread();

	// start frame consumer thread
	std::thread threadConsObj = fcons::FrameConsumer::startThread();

	// start HTTP server
	server.startListen();

	// shutdown
	///
	/**if (fcapsettings::SETT_OPEN_CAM_STREAMS) {
		log("Wait for thread #FPROD...");
	}**/
	threadProdObj.join();
	/**if (fcapsettings::SETT_OPEN_CAM_STREAMS) {
		log("done.");
	}**/
	///
	/**if (fcapsettings::SETT_OPEN_CAM_STREAMS) {
		log("Wait for thread #FCONS...");
	}**/
	threadConsObj.join();
	/**if (fcapsettings::SETT_OPEN_CAM_STREAMS) {
		log("done.");
	}**/
	///
	std::unique_lock<std::mutex> thrLockRunningCltHnds{fcapshared::gThrMtxRunningCltHnds, std::defer_lock};
	int runningClts = 1;
	log("Wait for threads #CLIENT...");
	while (runningClts > 0) {
		thrLockRunningCltHnds.lock();
		runningClts = fcapshared::gThrVarRunningCltsStc.runningHandlersCount;
		thrLockRunningCltHnds.unlock();
		//
		if (runningClts > 0) {
			log("still waiting for threads... (" + std::to_string(runningClts) + " running)");
			thrLockRunningCltHnds.lock();
			for (auto const& it : fcapshared::gThrVarRunningCltHndsMap) {
				if (! it.second) {
					continue;
				}
				log("  running #" + std::to_string(it.first));
			}
			thrLockRunningCltHnds.unlock();
			std::this_thread::sleep_for(std::chrono::milliseconds(500));
		}
	}

	return 0;
}
