#include <iostream>
#include <signal.h>
#include <thread>
#include <opencv2/opencv.hpp>

#include "shared.hpp"
#include "settings.hpp"
#include "cfgfile.hpp"
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
	sigemptyset(&sigIntHandler.sa_mask);
	sigIntHandler.sa_flags = 0;
	::sigaction(SIGINT, &sigIntHandler, nullptr);

	// SIGPIPE will occur e.g. when a TCP client has closed the connection
	if (::signal(SIGPIPE, SIG_IGN) == SIG_ERR) {  // ignore SIGPIPE
		log("Could not modify SIGPIPE");
		return false;
	}

	return true;
}

void printHelp(char** argv) {
	std::cout << "Usage: " + std::string(argv[0]) + " [-c FILENAME]" << std::endl;
}

bool parseCmdlineArgs(int argc, char** argv, std::string &cfgfileFn) {
	if (argc == 1) {
		return true;
	}
	if (argc == 2 || argc > 3) {
		printHelp(argv);
		return false;
	}

	std::string sArgv1(argv[1]);
	if (sArgv1.compare("-h") == 0 || sArgv1.compare("--h") == 0 || sArgv1.compare("--help") == 0) {
		printHelp(argv);
		return false;
	}
	if (sArgv1.compare("-c") != 0) {
		log("Invalid arg '" + std::string(argv[1]) + "'");
		printHelp(argv);
		return false;
	}
	cfgfileFn = argv[2];

	return true;
}

// -----------------------------------------------------------------------------

int main(int argc, char** argv) {
	std::string cfgfileFn;

	if (! parseCmdlineArgs(argc, argv, cfgfileFn)) {
		return -1;
	}
	//
	if (! fcapcfgfile::CfgFile::readConfigFile(cfgfileFn)) {
		return -1;
	}
	fcapcfgfile::StaticOptionsStc staticOptionsStc = fcapcfgfile::CfgFile::getStaticOptions();

	//
	fcapshared::Shared::setRtOpts_cameraFps(staticOptionsStc.cameraFps);

	//
	if (! initSignalHandlers()) {
		return -1;
	}

	// create HTTP server
	http::TcpServer server = http::TcpServer("0.0.0.0", staticOptionsStc.serverPort);
	if (! server.isSocketOk()) {
		return -1;
	}

	// start frame producer thread
	std::thread threadProdObj = frame::FrameProducer::startThread(server.getRunningHandlersCount);

	// start frame consumer thread
	std::thread threadConsObj = frame::FrameConsumer::startThread(
			server.getRunningHandlersCount,
			server.broadcastFrameToStreamingClients
		);

	// start HTTP server
	server.startListen();

	// shutdown
	///
	threadProdObj.join();
	threadConsObj.join();
	///
	uint32_t runningClts = 1;
	log("Wait for threads #CLIENT...");
	while (runningClts > 0) {
		runningClts = server.getRunningHandlersCount();
		//
		if (runningClts > 0) {
			log("still waiting for threads... (" + std::to_string(runningClts) + " running)");
			std::this_thread::sleep_for(std::chrono::milliseconds(500));
		}
	}

	return 0;
}
