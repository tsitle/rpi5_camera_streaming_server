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

	void Shared::setRuntimeOptions_procCalDone(const fcapconstants::CamIdEn camId, const bool val) {
		std::unique_lock<std::mutex> thrLock{gThrMtxRuntimeOptions, std::defer_lock};

		initStcRuntimeOptions();
		//
		thrLock.lock();
		gThrVarRuntimeOptions.procCalDone[camId] = val;
		thrLock.unlock();
	}

	void Shared::setRuntimeOptions_procCalDoReset(const fcapconstants::CamIdEn camId, const bool val) {
		std::unique_lock<std::mutex> thrLock{gThrMtxRuntimeOptions, std::defer_lock};

		initStcRuntimeOptions();
		//
		thrLock.lock();
		gThrVarRuntimeOptions.procCalDoReset[camId] = val;
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

	void Shared::setRuntimeOptions_procPtDone(const fcapconstants::CamIdEn camId, const bool val) {
		std::unique_lock<std::mutex> thrLock{gThrMtxRuntimeOptions, std::defer_lock};

		initStcRuntimeOptions();
		//
		thrLock.lock();
		gThrVarRuntimeOptions.procPtDone[camId] = val;
		thrLock.unlock();
	}

	void Shared::setRuntimeOptions_procPtDoReset(const fcapconstants::CamIdEn camId, const bool val) {
		std::unique_lock<std::mutex> thrLock{gThrMtxRuntimeOptions, std::defer_lock};

		initStcRuntimeOptions();
		//
		thrLock.lock();
		gThrVarRuntimeOptions.procPtDoReset[camId] = val;
		thrLock.unlock();
	}

	void Shared::setRuntimeOptions_procPtChangedRectCorners(const fcapconstants::CamIdEn camId, const bool val) {
		std::unique_lock<std::mutex> thrLock{gThrMtxRuntimeOptions, std::defer_lock};

		initStcRuntimeOptions();
		//
		thrLock.lock();
		gThrVarRuntimeOptions.procPtChangedRectCorners[camId] = val;
		thrLock.unlock();
	}

	void Shared::setRuntimeOptions_procPtRectCorners(const fcapconstants::CamIdEn camId, const std::vector<cv::Point> val) {
		std::unique_lock<std::mutex> thrLock{gThrMtxRuntimeOptions, std::defer_lock};

		initStcRuntimeOptions();
		//
		thrLock.lock();
		gThrVarRuntimeOptions.procPtRectCorners[camId].clear();
		uint8_t tmpSz = val.size();
		for (uint8_t x = 1; x <= tmpSz && x <= fcapconstants::PROC_PT_RECTCORNERS_MAX; x++) {
			cv::Point tmpPoint = val[x - 1];
			gThrVarRuntimeOptions.procPtRectCorners[camId].push_back(tmpPoint);
		}
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
			//
			gThrVarRuntimeOptions.procBncAdjBrightness = fcapsettings::PROC_BNC_DEFAULT_ADJ_BRIGHTNESS;
			gThrVarRuntimeOptions.procBncAdjContrast = fcapsettings::PROC_BNC_DEFAULT_ADJ_CONTRAST;
			//
			gThrVarRuntimeOptions.procCalDone[fcapconstants::CamIdEn::CAM_0] = false;
			gThrVarRuntimeOptions.procCalDone[fcapconstants::CamIdEn::CAM_1] = false;
			gThrVarRuntimeOptions.procCalDoReset[fcapconstants::CamIdEn::CAM_0] = false;
			gThrVarRuntimeOptions.procCalDoReset[fcapconstants::CamIdEn::CAM_1] = false;
			gThrVarRuntimeOptions.procCalShowCalibChessboardPoints = fcapsettings::PROC_CAL_DEFAULT_SHOWCALIBCHESSPOINTS;
			//
			gThrVarRuntimeOptions.procPtDone[fcapconstants::CamIdEn::CAM_0] = false;
			gThrVarRuntimeOptions.procPtDone[fcapconstants::CamIdEn::CAM_1] = false;
			gThrVarRuntimeOptions.procPtDoReset[fcapconstants::CamIdEn::CAM_0] = false;
			gThrVarRuntimeOptions.procPtDoReset[fcapconstants::CamIdEn::CAM_1] = false;
			gThrVarRuntimeOptions.procPtChangedRectCorners[fcapconstants::CamIdEn::CAM_0] = false;
			gThrVarRuntimeOptions.procPtChangedRectCorners[fcapconstants::CamIdEn::CAM_1] = false;
			gThrVarRuntimeOptions.procPtRectCorners[fcapconstants::CamIdEn::CAM_0] = std::vector<cv::Point>();
			gThrVarRuntimeOptions.procPtRectCorners[fcapconstants::CamIdEn::CAM_1] = std::vector<cv::Point>();
			//
			gThrVarSetRuntimeOptions = true;
		}
		thrLock.unlock();
	}

}  // namespace fcapshared
