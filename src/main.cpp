#include <iostream>
#include <signal.h>
#include <thread>
#include <opencv2/opencv.hpp>

#include "shared.hpp"
#include "settings.hpp"
#include "frame/frame_producer.hpp"
#include "frame/frame_consumer.hpp"
#include "http/http_tcp_server.hpp"

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
	fcapshared::Shared::setFlagNeedToStop();
}

bool initSignalHandlers() {
	// SIGINT
	struct sigaction sigIntHandler;
	sigIntHandler.sa_handler = sigHandlerCtrlC;
	::sigemptyset(&sigIntHandler.sa_mask);
	sigIntHandler.sa_flags = 0;
	::sigaction(SIGINT, &sigIntHandler, NULL);

	// SIGPIPE will occur e.g. when a TCP client has closed the connection
	if (::signal(SIGPIPE, SIG_IGN) == SIG_ERR) {  // ignore SIGPIPE
		log("Could not modify SIGPIPE");
		return false;
	}

	return true;
}

// -----------------------------------------------------------------------------

int main() {
	fcapshared::Shared::initGlobals();
	if (! initSignalHandlers()) {
		return -1;
	}

	// create HTTP server
	http::TcpServer server = http::TcpServer("0.0.0.0", (int)fcapsettings::SETT_SERVER_PORT);
	if (! server.isSocketOk()) {
		return -1;
	}

	// start frame producer thread
	std::thread threadProdObj = frame::FrameProducer::startThread();

	// start frame consumer thread
	std::thread threadConsObj = frame::FrameConsumer::startThread();

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
