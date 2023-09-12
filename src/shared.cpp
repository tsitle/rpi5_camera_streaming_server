#include <chrono>

#include "shared.hpp"
#include "settings.hpp"

using namespace std::chrono_literals;

namespace fcapshared {

	//
	bool gThrVarCamStreamsOpened = false;
	std::mutex gThrMtxCamStreamsOpened;
	std::condition_variable gThrCondCamStreamsOpened;

	//
	frame::FrameQueueJpeg gFrameQueueOutp;

	//
	RunningCltsStc gThrVarRunningCltsStc;
	std::unordered_map<unsigned int, bool> gThrVarRunningCltHndsMap;
	std::mutex gThrMtxRunningCltHnds;

	// -----------------------------------------------------------------------------
	// -----------------------------------------------------------------------------

	//
	bool Shared::gThrVarNeedToStop = false;
	std::mutex Shared::gThrMtxNeedToStop;
	std::condition_variable Shared::gThrCondNeedToStop;

	//
	RuntimeOptionsStc Shared::gThrVargRuntimeOptions;
	std::mutex Shared::gThrMtxRuntimeOptions;

	// -----------------------------------------------------------------------------
	// -----------------------------------------------------------------------------

	void Shared::initGlobals() {
		fcapshared::gThrVarRunningCltsStc.runningHandlersCount = 0;
		fcapshared::gThrVarRunningCltsStc.runningStreamsCount = 0;

		//
		fcapshared::gFrameQueueOutp.setFrameSize(cv::Size(fcapsettings::SETT_OUTPUT_SZ.width, fcapsettings::SETT_OUTPUT_SZ.height));

		//
		gThrVargRuntimeOptions.outputCams = fcapconstants::OutputCamsEn::CAM_L;
	}

	// -----------------------------------------------------------------------------

	void Shared::setFlagNeedToStop() {
		std::unique_lock<std::mutex> thrLock{gThrMtxNeedToStop, std::defer_lock};

		thrLock.lock();
		gThrVarNeedToStop = true;
		thrLock.unlock();
		gThrCondNeedToStop.notify_all();
	}

	bool Shared::getFlagNeedToStop() {
		bool resB = false;
		std::unique_lock<std::mutex> thrLock{gThrMtxNeedToStop, std::defer_lock};

		thrLock.lock();
		if (gThrCondNeedToStop.wait_for(thrLock, 1ms, []{return gThrVarNeedToStop;})) {
			resB = true;
		}
		thrLock.unlock();
		return resB;
	}

	// -----------------------------------------------------------------------------

	RuntimeOptionsStc Shared::getRuntimeOptions() {
		RuntimeOptionsStc resStc;
		std::unique_lock<std::mutex> thrLock{gThrMtxRuntimeOptions, std::defer_lock};

		thrLock.lock();
		resStc = gThrVargRuntimeOptions;
		thrLock.unlock();
		return resStc;
	}

	void Shared::setRuntimeOptions_outputCams(fcapconstants::OutputCamsEn val) {
		std::unique_lock<std::mutex> thrLock{gThrMtxRuntimeOptions, std::defer_lock};

		thrLock.lock();
		gThrVargRuntimeOptions.outputCams = val;
		thrLock.unlock();
	}

}  // namespace fcapshared
