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
		bool procBncChanged;
		int16_t procBncAdjBrightness;
		int16_t procBncAdjContrast;
		int16_t procBncAdjGamma;
		std::map<fcapconstants::CamIdEn, bool> procCalDone;
		std::map<fcapconstants::CamIdEn, bool> procCalDoStart;
		std::map<fcapconstants::CamIdEn, bool> procCalDoReset;
		std::map<fcapconstants::CamIdEn, bool> procCalChanged;
		std::map<fcapconstants::CamIdEn, bool> procCalShowCalibChessboardPoints;
		bool procGridChanged;
		bool procGridShow;
		std::map<fcapconstants::CamIdEn, bool> procPtDone;
		std::map<fcapconstants::CamIdEn, bool> procPtDoReset;
		std::map<fcapconstants::CamIdEn, bool> procPtChanged;
		std::map<fcapconstants::CamIdEn, std::vector<cv::Point>> procPtRectCorners;
		bool procRoiChanged;
		uint8_t procRoiSizePerc;
		std::map<fcapconstants::CamIdEn, bool> procTrDoReset;
		std::map<fcapconstants::CamIdEn, bool> procTrChanged;
		std::map<fcapconstants::CamIdEn, cv::Point> procTrFixDelta;
		std::map<fcapconstants::CamIdEn, cv::Point> procTrDynDelta;

		RuntimeOptionsStc() {
			reset();
		}

		void reset() {
			outputCams = fcapconstants::OutputCamsEn::CAM_L;
			cameraFps = 0;
			resolutionOutput = cv::Size(0, 0);
			//
			procBncChanged = false;
			procBncAdjBrightness = 0;
			procBncAdjContrast = 0;
			procBncAdjGamma = 0;
			//
			procGridChanged = false;
			procGridShow = false;
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
			procCalDone[camId] = false;
			procCalDoStart[camId] = false;
			procCalDoReset[camId] = false;
			procCalChanged[camId] = false;
			procCalShowCalibChessboardPoints[camId] = false;
			//
			procPtDone[camId] = false;
			procPtDoReset[camId] = false;
			procPtChanged[camId] = false;
			procPtRectCorners[camId] = std::vector<cv::Point>();
			//
			procTrDoReset[camId] = false;
			procTrChanged[camId] = false;
			procTrFixDelta[camId] = cv::Point();
			procTrDynDelta[camId] = cv::Point();
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
			static void setRtOpts_procBncChanged(const bool val);
			static void setRtOpts_procBncAdjBrightness(const int16_t val);
			static void setRtOpts_procBncAdjContrast(const int16_t val);
			static void setRtOpts_procBncAdjGamma(const int16_t val);
			///
			static void setRtOpts_procCalDone(const fcapconstants::CamIdEn camId, const bool val);
			static void setRtOpts_procCalDoStart(const fcapconstants::CamIdEn camId, const bool val);
			static void setRtOpts_procCalDoReset(const fcapconstants::CamIdEn camId, const bool val);
			static void setRtOpts_procCalChanged(const fcapconstants::CamIdEn camId, const bool val);
			static void setRtOpts_procCalShowCalibChessboardPoints(const fcapconstants::CamIdEn camId, const bool val);
			///
			static void setRtOpts_procGridChanged(const bool val);
			static void setRtOpts_procGridShow(const bool val);
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
			static void setRtOpts_procTrFixDelta(const fcapconstants::CamIdEn camId, const cv::Point val);
			static void setRtOpts_procTrDynDelta(const fcapconstants::CamIdEn camId, const cv::Point val);
			//
			static bool fileExists(const std::string &fname);
			static std::string toLower(const std::string &inp);
			static std::string toUpper(const std::string &inp);

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
