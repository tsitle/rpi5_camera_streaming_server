#ifndef CFGFILE_HPP_
#define CFGFILE_HPP_

#include <mutex>
#include <opencv2/opencv.hpp>

#include "constants.hpp"
#include "settings.hpp"

namespace fcapcfgfile {

	struct ProcEnabledStc {
		bool bnc;
		bool cal;
		bool pt;
		bool tr;
		bool overlCam;
		bool overlCal;

		ProcEnabledStc() {
			reset();
		}

		void reset() {
			bnc = true;
			cal = true;
			pt = true;
			tr = true;
			overlCam = true;
			overlCal = true;
		}
	};

	struct StaticOptionsStc {
		uint16_t serverPort;
		cv::Size gstreamerResolutionCapture;
		cv::Size resolutionOutput;
		uint8_t cameraFps;
		fcapconstants::CamIdEn camL;
		fcapconstants::CamIdEn camR;
		fcapconstants::CamSourceEn camSourceType;
		std::string camSource0;
		std::string camSource1;
		std::string pngOutputPath;
		bool outputPngs;
		std::string calibOutputPath;
		ProcEnabledStc procEnabled;

		StaticOptionsStc() {
			reset();
		}

		void reset() {
			serverPort = fcapsettings::SETT_DEFAULT_SERVER_PORT;
			gstreamerResolutionCapture = fcapsettings::SETT_DEFAULT_CAPTURE_SZ;
			resolutionOutput = fcapsettings::SETT_DEFAULT_OUTPUT_SZ;
			cameraFps = fcapsettings::SETT_DEFAULT_FPS;
			camL = fcapconstants::CamIdEn::CAM_0;
			camR = fcapconstants::CamIdEn::CAM_1;
			camSourceType = fcapconstants::CamSourceEn::UNSPECIFIED;
			camSource0 = "";
			camSource1 = "";
			pngOutputPath = ".";
			outputPngs = false;
			calibOutputPath = ".";
			procEnabled.reset();
		}
	};

	class CfgFile {
		public:
			static bool readConfigFile(const std::string &cfgfileFn);
			//
			static StaticOptionsStc getStaticOptions();

		private:
			static StaticOptionsStc gThrVarStaticOptions;
			static std::mutex gThrMtxStaticOptions;

			//

			static void log(const std::string &message);
			static std::string getDefaultStaticConfig();
			static cv::Size getSizeFromString(const std::string &x, const std::string &nameArg);
			static fcapconstants::CamSourceEn getCamSourceFromString(const std::string &x, const std::string &nameArg);
			static fcapconstants::CamIdEn getCamIdFromString(const std::string &x, const std::string &nameArg);
	};

}  // namespace fcapcfgfile

#endif  // CFGFILE_HPP_
