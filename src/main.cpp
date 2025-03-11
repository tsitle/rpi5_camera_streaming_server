#include <iostream>
#include <csignal>
#include <thread>

#include "shared.hpp"
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
	std::cout << "Usage: " + std::string(argv[0]) + " [[--version] | [-c FILENAME]]" << std::endl;
}

bool parseCmdlineArgs(int argc, char** argv, std::string &cfgfileFn) {
	if (argc == 1) {
		return true;
	}

	const std::string sArgv1(argv[1]);
	if (sArgv1 == "-h" || sArgv1 == "--h" || sArgv1 == "--help") {
		printHelp(argv);
		return false;
	}
	if (sArgv1 == "--version") {
		std::cout << fcapconstants::HTTP_SERVER_NAME + " v" + fcapconstants::HTTP_SERVER_VERSION << std::endl;
		return false;
	}
	if (sArgv1 != "-c") {
		log("Invalid arg '" + std::string(argv[1]) + "'");
		printHelp(argv);
		return false;
	}
	if (argc != 3) {
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
	std::thread threadProdObj = frame::FrameProducer::startThread(http::TcpServer::getRunningHandlersCount);

	// start frame consumer thread
	std::thread threadConsObj = frame::FrameConsumer::startThread(
			http::TcpServer::getRunningHandlersCount,
			http::TcpServer::broadcastFrameToStreamingClients
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
		runningClts = http::TcpServer::getRunningHandlersCount();
		//
		if (runningClts > 0) {
			log("still waiting for threads... (" + std::to_string(runningClts) + " running)");
			std::this_thread::sleep_for(std::chrono::milliseconds(500));
		}
	}

	return 0;
}
