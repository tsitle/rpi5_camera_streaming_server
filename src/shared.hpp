#ifndef SHARED_HPP_
#define SHARED_HPP_

#include <condition_variable>
#include <mutex>
#include <thread>
#include <opencv2/opencv.hpp>

#include "constants.hpp"
#include "settings.hpp"

namespace fcapshared {

	struct RuntimeOptionsStc {
		fcapconstants::OutputCamsEn outputCams;
		uint8_t cameraFps;
		cv::Size resolutionOutput;
		std::map<fcapconstants::CamIdEn, bool> cameraReady;
		std::map<fcapconstants::CamIdEn, bool> procBncChanged;
		int16_t procBncAdjBrightness;
		int16_t procBncAdjContrast;
		std::map<fcapconstants::CamIdEn, bool> procCalDone;
		std::map<fcapconstants::CamIdEn, bool> procCalDoStart;
		std::map<fcapconstants::CamIdEn, bool> procCalDoReset;
		std::map<fcapconstants::CamIdEn, bool> procCalChanged;
		bool procCalShowCalibChessboardPoints;
		std::map<fcapconstants::CamIdEn, bool> procPtDone;
		std::map<fcapconstants::CamIdEn, bool> procPtDoReset;
		std::map<fcapconstants::CamIdEn, bool> procPtChanged;
		std::map<fcapconstants::CamIdEn, std::vector<cv::Point>> procPtRectCorners;
		bool procRoiChanged;
		uint8_t procRoiSizePerc;
		std::map<fcapconstants::CamIdEn, bool> procTrDoReset;
		std::map<fcapconstants::CamIdEn, bool> procTrChanged;
		std::map<fcapconstants::CamIdEn, cv::Point> procTrDelta;

		RuntimeOptionsStc() {
			reset();
		}

		void reset() {
			outputCams = fcapconstants::OutputCamsEn::CAM_L;
			cameraFps = 0;
			resolutionOutput = cv::Size(0, 0);
			//
			procBncAdjBrightness = fcapsettings::PROC_BNC_DEFAULT_ADJ_BRIGHTNESS;
			procBncAdjContrast = fcapsettings::PROC_BNC_DEFAULT_ADJ_CONTRAST;
			//
			procCalShowCalibChessboardPoints = fcapsettings::PROC_CAL_DEFAULT_SHOWCALIBCHESSPOINTS;
			//
			procRoiChanged = false;
			procRoiSizePerc = 100;
			//
			_resetForCamId(fcapconstants::CamIdEn::CAM_0);
			_resetForCamId(fcapconstants::CamIdEn::CAM_1);
		}

		void _resetForCamId(fcapconstants::CamIdEn camId) {
			cameraReady[camId] = false;
			//
			procBncChanged[camId] = false;
			//
			procCalDone[camId] = false;
			procCalDoStart[camId] = false;
			procCalDoReset[camId] = false;
			procCalChanged[camId] = false;
			//
			procPtDone[camId] = false;
			procPtDoReset[camId] = false;
			procPtChanged[camId] = false;
			procPtRectCorners[camId] = std::vector<cv::Point>();
			//
			procTrDoReset[camId] = false;
			procTrChanged[camId] = false;
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
			static void setRtOpts_outputCams(const fcapconstants::OutputCamsEn val);
			static void setRtOpts_cameraFps(const uint8_t val);
			static void setRtOpts_resolutionOutput(const cv::Size &val);
			static void setRtOpts_cameraReady(const fcapconstants::CamIdEn camId, const bool val);
			///
			static void setRtOpts_procBncChanged(const fcapconstants::CamIdEn camId, const bool val);
			static void setRtOpts_procBncAdjBrightness(const int16_t val);
			static void setRtOpts_procBncAdjContrast(const int16_t val);
			///
			static void setRtOpts_procCalDone(const fcapconstants::CamIdEn camId, const bool val);
			static void setRtOpts_procCalDoStart(const fcapconstants::CamIdEn camId, const bool val);
			static void setRtOpts_procCalDoReset(const fcapconstants::CamIdEn camId, const bool val);
			static void setRtOpts_procCalChanged(const fcapconstants::CamIdEn camId, const bool val);
			static void setRtOpts_procCalShowCalibChessboardPoints(const bool val);
			///
			static void setRtOpts_procPtDone(const fcapconstants::CamIdEn camId, const bool val);
			static void setRtOpts_procPtDoReset(const fcapconstants::CamIdEn camId, const bool val);
			static void setRtOpts_procPtChanged(const fcapconstants::CamIdEn camId, const bool val);
			static void setRtOpts_procPtRectCorners(const fcapconstants::CamIdEn camId, const std::vector<cv::Point> val);
			///
			static void setRtOpts_procRoiChanged(const bool val);
			static void setRtOpts_procRoiSizePerc(const uint8_t val);
			///
			static void setRtOpts_procTrDoReset(const fcapconstants::CamIdEn camId, const bool val);
			static void setRtOpts_procTrChanged(const fcapconstants::CamIdEn camId, const bool val);
			static void setRtOpts_procTrDelta(const fcapconstants::CamIdEn camId, const cv::Point val);
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
