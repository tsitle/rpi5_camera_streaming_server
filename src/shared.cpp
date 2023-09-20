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

		thrLock.lock();
		resStc = gThrVarRuntimeOptions;
		thrLock.unlock();
		return resStc;
	}

	void Shared::setRtOpts_outputCams(const fcapconstants::OutputCamsEn val) {
		std::unique_lock<std::mutex> thrLock{gThrMtxRuntimeOptions, std::defer_lock};

		thrLock.lock();
		gThrVarRuntimeOptions.outputCams = val;
		thrLock.unlock();
	}

	void Shared::setRtOpts_cameraFps(const uint8_t val) {
		std::unique_lock<std::mutex> thrLock{gThrMtxRuntimeOptions, std::defer_lock};

		thrLock.lock();
		gThrVarRuntimeOptions.cameraFps = val;
		thrLock.unlock();
	}

	void Shared::setRtOpts_cameraReady(const fcapconstants::CamIdEn camId, const bool val) {
		std::unique_lock<std::mutex> thrLock{gThrMtxRuntimeOptions, std::defer_lock};

		thrLock.lock();
		gThrVarRuntimeOptions.cameraReady[camId] = val;
		thrLock.unlock();
	}

	void Shared::setRtOpts_procBncChanged(const fcapconstants::CamIdEn camId, const bool val) {
		std::unique_lock<std::mutex> thrLock{gThrMtxRuntimeOptions, std::defer_lock};

		thrLock.lock();
		gThrVarRuntimeOptions.procBncChanged[camId] = val;
		thrLock.unlock();
	}

	void Shared::setRtOpts_procBncAdjBrightness(const int16_t val) {
		if (val < fcapconstants::PROC_BNC_MIN_ADJ_BRIGHTNESS || val > fcapconstants::PROC_BNC_MAX_ADJ_BRIGHTNESS) {
			return;
		}

		std::unique_lock<std::mutex> thrLock{gThrMtxRuntimeOptions, std::defer_lock};

		thrLock.lock();
		gThrVarRuntimeOptions.procBncAdjBrightness = val;
		thrLock.unlock();
	}

	void Shared::setRtOpts_procBncAdjContrast(const int16_t val) {
		if (val < fcapconstants::PROC_BNC_MIN_ADJ_CONTRAST || val > fcapconstants::PROC_BNC_MAX_ADJ_CONTRAST) {
			return;
		}

		std::unique_lock<std::mutex> thrLock{gThrMtxRuntimeOptions, std::defer_lock};

		thrLock.lock();
		gThrVarRuntimeOptions.procBncAdjContrast = val;
		thrLock.unlock();
	}

	void Shared::setRtOpts_procCalDone(const fcapconstants::CamIdEn camId, const bool val) {
		std::unique_lock<std::mutex> thrLock{gThrMtxRuntimeOptions, std::defer_lock};

		thrLock.lock();
		gThrVarRuntimeOptions.procCalDone[camId] = val;
		thrLock.unlock();
	}

	void Shared::setRtOpts_procCalDoReset(const fcapconstants::CamIdEn camId, const bool val) {
		std::unique_lock<std::mutex> thrLock{gThrMtxRuntimeOptions, std::defer_lock};

		thrLock.lock();
		gThrVarRuntimeOptions.procCalDoReset[camId] = val;
		thrLock.unlock();
	}

	void Shared::setRtOpts_procCalChanged(const fcapconstants::CamIdEn camId, const bool val) {
		std::unique_lock<std::mutex> thrLock{gThrMtxRuntimeOptions, std::defer_lock};

		thrLock.lock();
		gThrVarRuntimeOptions.procCalChanged[camId] = val;
		thrLock.unlock();
	}

	void Shared::setRtOpts_procCalShowCalibChessboardPoints(const bool val) {
		std::unique_lock<std::mutex> thrLock{gThrMtxRuntimeOptions, std::defer_lock};

		thrLock.lock();
		gThrVarRuntimeOptions.procCalShowCalibChessboardPoints = val;
		thrLock.unlock();
	}

	void Shared::setRtOpts_procPtDone(const fcapconstants::CamIdEn camId, const bool val) {
		std::unique_lock<std::mutex> thrLock{gThrMtxRuntimeOptions, std::defer_lock};

		thrLock.lock();
		gThrVarRuntimeOptions.procPtDone[camId] = val;
		thrLock.unlock();
	}

	void Shared::setRtOpts_procPtDoReset(const fcapconstants::CamIdEn camId, const bool val) {
		std::unique_lock<std::mutex> thrLock{gThrMtxRuntimeOptions, std::defer_lock};

		thrLock.lock();
		gThrVarRuntimeOptions.procPtDoReset[camId] = val;
		thrLock.unlock();
	}

	void Shared::setRtOpts_procPtChanged(const fcapconstants::CamIdEn camId, const bool val) {
		std::unique_lock<std::mutex> thrLock{gThrMtxRuntimeOptions, std::defer_lock};

		thrLock.lock();
		gThrVarRuntimeOptions.procPtChanged[camId] = val;
		thrLock.unlock();
	}

	void Shared::setRtOpts_procPtRectCorners(const fcapconstants::CamIdEn camId, const std::vector<cv::Point> val) {
		std::unique_lock<std::mutex> thrLock{gThrMtxRuntimeOptions, std::defer_lock};

		thrLock.lock();
		gThrVarRuntimeOptions.procPtRectCorners[camId].clear();
		uint8_t tmpSz = val.size();
		for (uint8_t x = 1; x <= tmpSz && x <= fcapconstants::PROC_PT_RECTCORNERS_MAX; x++) {
			cv::Point tmpPoint = val[x - 1];
			gThrVarRuntimeOptions.procPtRectCorners[camId].push_back(tmpPoint);
		}
		thrLock.unlock();
	}

	void Shared::setRtOpts_procTrDoReset(const fcapconstants::CamIdEn camId, const bool val) {
		std::unique_lock<std::mutex> thrLock{gThrMtxRuntimeOptions, std::defer_lock};

		thrLock.lock();
		gThrVarRuntimeOptions.procTrDoReset[camId] = val;
		thrLock.unlock();
	}

	void Shared::setRtOpts_procTrChanged(const fcapconstants::CamIdEn camId, const bool val) {
		std::unique_lock<std::mutex> thrLock{gThrMtxRuntimeOptions, std::defer_lock};

		thrLock.lock();
		gThrVarRuntimeOptions.procTrChanged[camId] = val;
		thrLock.unlock();
	}

	void Shared::setRtOpts_procTrDelta(const fcapconstants::CamIdEn camId, const cv::Point val) {
		std::unique_lock<std::mutex> thrLock{gThrMtxRuntimeOptions, std::defer_lock};

		thrLock.lock();
		gThrVarRuntimeOptions.procTrDelta[camId] = val;
		thrLock.unlock();
	}

	// -----------------------------------------------------------------------------

	bool Shared::fileExists(const std::string &fname) {
		struct stat buffer;
		return (stat(fname.c_str(), &buffer) == 0);
	}

}  // namespace fcapshared
