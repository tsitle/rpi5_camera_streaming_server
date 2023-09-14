#include <chrono>

#include "shared.hpp"

using namespace std::chrono_literals;

namespace fcapshared {

	//
	bool Shared::gThrVarNeedToStop = false;
	std::mutex Shared::gThrMtxNeedToStop;
	std::condition_variable Shared::gThrCondNeedToStop;

	//
	bool Shared::gThrVarSetRuntimeOptions = false;
	RuntimeOptionsStc Shared::gThrVarRuntimeOptions;
	std::mutex Shared::gThrMtxRuntimeOptions;

	// -----------------------------------------------------------------------------
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

		initStcRuntimeOptions();
		//
		thrLock.lock();
		resStc = gThrVarRuntimeOptions;
		thrLock.unlock();
		return resStc;
	}

	void Shared::setRuntimeOptions_outputCams(fcapconstants::OutputCamsEn val) {
		std::unique_lock<std::mutex> thrLock{gThrMtxRuntimeOptions, std::defer_lock};

		initStcRuntimeOptions();
		//
		thrLock.lock();
		gThrVarRuntimeOptions.outputCams = val;
		thrLock.unlock();
	}

	// -----------------------------------------------------------------------------
	// -----------------------------------------------------------------------------

	void Shared::initStcRuntimeOptions() {
		std::unique_lock<std::mutex> thrLock{gThrMtxRuntimeOptions, std::defer_lock};

		thrLock.lock();
		if (! gThrVarSetRuntimeOptions) {
			gThrVarRuntimeOptions.outputCams = fcapconstants::OutputCamsEn::CAM_L;
			//
			gThrVarSetRuntimeOptions = true;
		}
		thrLock.unlock();
	}

}  // namespace fcapshared
