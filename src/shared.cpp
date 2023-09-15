#include <chrono>
#include <sys/stat.h>  // for stat()

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
		if (gThrCondNeedToStop.wait_for(thrLock, 1ms, []{ return gThrVarNeedToStop; })) {
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

	void Shared::setRuntimeOptions_outputCams(const fcapconstants::OutputCamsEn val) {
		std::unique_lock<std::mutex> thrLock{gThrMtxRuntimeOptions, std::defer_lock};

		initStcRuntimeOptions();
		//
		thrLock.lock();
		gThrVarRuntimeOptions.outputCams = val;
		thrLock.unlock();
	}

	void Shared::setRuntimeOptions_cameraFps(const uint8_t val) {
		std::unique_lock<std::mutex> thrLock{gThrMtxRuntimeOptions, std::defer_lock};

		initStcRuntimeOptions();
		//
		thrLock.lock();
		gThrVarRuntimeOptions.cameraFps = val;
		thrLock.unlock();
	}

	void Shared::setRuntimeOptions_procBncAdjBrightness(const int16_t val) {
		if (val < fcapconstants::PROC_BNC_MIN_ADJ_BRIGHTNESS || val > fcapconstants::PROC_BNC_MAX_ADJ_BRIGHTNESS) {
			return;
		}

		std::unique_lock<std::mutex> thrLock{gThrMtxRuntimeOptions, std::defer_lock};

		initStcRuntimeOptions();
		//
		thrLock.lock();
		gThrVarRuntimeOptions.procBncAdjBrightness = val;
		thrLock.unlock();
	}

	void Shared::setRuntimeOptions_procBncAdjContrast(const int16_t val) {
		if (val < fcapconstants::PROC_BNC_MIN_ADJ_CONTRAST || val > fcapconstants::PROC_BNC_MAX_ADJ_CONTRAST) {
			return;
		}

		std::unique_lock<std::mutex> thrLock{gThrMtxRuntimeOptions, std::defer_lock};

		initStcRuntimeOptions();
		//
		thrLock.lock();
		gThrVarRuntimeOptions.procBncAdjContrast = val;
		thrLock.unlock();
	}

	void Shared::setRuntimeOptions_procCalShowCalibChessboardPoints(const bool val) {
		std::unique_lock<std::mutex> thrLock{gThrMtxRuntimeOptions, std::defer_lock};

		initStcRuntimeOptions();
		//
		thrLock.lock();
		gThrVarRuntimeOptions.procCalShowCalibChessboardPoints = val;
		thrLock.unlock();
	}

	// -----------------------------------------------------------------------------

	bool Shared::fileExists(const std::string &fname) {
		struct stat buffer;
		return (stat(fname.c_str(), &buffer) == 0);
	}

	// -----------------------------------------------------------------------------
	// -----------------------------------------------------------------------------

	void Shared::initStcRuntimeOptions() {
		std::unique_lock<std::mutex> thrLock{gThrMtxRuntimeOptions, std::defer_lock};

		thrLock.lock();
		if (! gThrVarSetRuntimeOptions) {
			gThrVarRuntimeOptions.outputCams = fcapconstants::OutputCamsEn::CAM_L;
			gThrVarRuntimeOptions.cameraFps = 0;
			gThrVarRuntimeOptions.procBncAdjBrightness = fcapsettings::PROC_BNC_DEFAULT_ADJ_BRIGHTNESS;
			gThrVarRuntimeOptions.procBncAdjContrast = fcapsettings::PROC_BNC_DEFAULT_ADJ_CONTRAST;
			gThrVarRuntimeOptions.procCalShowCalibChessboardPoints = fcapsettings::PROC_CAL_DEFAULT_SHOWCALIBCHESSPOINTS;
			//
			gThrVarSetRuntimeOptions = true;
		}
		thrLock.unlock();
	}

}  // namespace fcapshared
