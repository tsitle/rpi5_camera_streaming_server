#ifndef SHARED_HPP_
#define SHARED_HPP_

#include <condition_variable>
#include <mutex>
#include <thread>
#include <opencv2/opencv.hpp>

#include "constants.hpp"
#include "settings.hpp"
#include "frame/frame_queue.hpp"

namespace fcapshared {

	struct RuntimeOptionsStc {
		fcapconstants::OutputCamsEn outputCams;
		uint8_t cameraFps;
		int16_t procBncAdjBrightness;
		int16_t procBncAdjContrast;
		std::map<fcapconstants::CamIdEn, bool> procCalDone;
		std::map<fcapconstants::CamIdEn, bool> procCalDoReset;
		bool procCalShowCalibChessboardPoints;
		std::map<fcapconstants::CamIdEn, bool> procPtDone;
		std::map<fcapconstants::CamIdEn, bool> procPtDoReset;
		std::map<fcapconstants::CamIdEn, bool> procPtChangedRectCorners;
		std::map<fcapconstants::CamIdEn, std::vector<cv::Point>> procPtRectCorners;
		std::map<fcapconstants::CamIdEn, bool> procTrDoReset;
		std::map<fcapconstants::CamIdEn, bool> procTrChangedDelta;
		std::map<fcapconstants::CamIdEn, cv::Point> procTrDelta;

		RuntimeOptionsStc() {
			reset();
		}

		void reset() {
			outputCams = fcapconstants::OutputCamsEn::CAM_L;
			cameraFps = 0;
			//
			procBncAdjBrightness = fcapsettings::PROC_BNC_DEFAULT_ADJ_BRIGHTNESS;
			procBncAdjContrast = fcapsettings::PROC_BNC_DEFAULT_ADJ_CONTRAST;
			//
			procCalShowCalibChessboardPoints = fcapsettings::PROC_CAL_DEFAULT_SHOWCALIBCHESSPOINTS;
			//
			_resetForCamId(fcapconstants::CamIdEn::CAM_0);
			_resetForCamId(fcapconstants::CamIdEn::CAM_1);
		}

		void _resetForCamId(fcapconstants::CamIdEn camId) {
			procCalDone[camId] = false;
			procCalDoReset[camId] = false;
			//
			procPtDone[camId] = false;
			procPtDoReset[camId] = false;
			procPtChangedRectCorners[camId] = false;
			procPtRectCorners[camId] = std::vector<cv::Point>();
			//
			procTrDoReset[camId] = false;
			procTrChangedDelta[camId] = false;
			procTrDelta[camId] = cv::Point();
		}
	};

	class Shared {
		public:
			static void setFlagNeedToStop();
			static bool getFlagNeedToStop();
			//
			///
			static RuntimeOptionsStc getRuntimeOptions();
			///
			static void setRuntimeOptions_outputCams(const fcapconstants::OutputCamsEn val);
			static void setRuntimeOptions_cameraFps(const uint8_t val);
			///
			static void setRuntimeOptions_procBncAdjBrightness(const int16_t val);
			static void setRuntimeOptions_procBncAdjContrast(const int16_t val);
			///
			static void setRuntimeOptions_procCalDone(const fcapconstants::CamIdEn camId, const bool val);
			static void setRuntimeOptions_procCalDoReset(const fcapconstants::CamIdEn camId, const bool val);
			static void setRuntimeOptions_procCalShowCalibChessboardPoints(const bool val);
			///
			static void setRuntimeOptions_procPtDone(const fcapconstants::CamIdEn camId, const bool val);
			static void setRuntimeOptions_procPtDoReset(const fcapconstants::CamIdEn camId, const bool val);
			static void setRuntimeOptions_procPtChangedRectCorners(const fcapconstants::CamIdEn camId, const bool val);
			static void setRuntimeOptions_procPtRectCorners(const fcapconstants::CamIdEn camId, const std::vector<cv::Point> val);
			///
			static void setRuntimeOptions_procTrDoReset(const fcapconstants::CamIdEn camId, const bool val);
			static void setRuntimeOptions_procTrChangedDelta(const fcapconstants::CamIdEn camId, const bool val);
			static void setRuntimeOptions_procTrDelta(const fcapconstants::CamIdEn camId, const cv::Point val);
			//
			static bool fileExists(const std::string &fname);

		private:
			static bool gThrVarNeedToStop;
			static std::mutex gThrMtxNeedToStop;
			static std::condition_variable gThrCondNeedToStop;
			//
			static RuntimeOptionsStc gThrVarRuntimeOptions;
			static std::mutex gThrMtxRuntimeOptions;
	};

}  // namespace fcapshared

#endif  // SHARED_HPP_
