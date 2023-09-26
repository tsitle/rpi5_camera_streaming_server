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
		bool flip;
		bool pt;
		bool roi;
		bool tr;
		bool overlCam;
		bool overlCal;

		ProcEnabledStc() {
			reset();
		}

		void reset() {
			bnc = true;
			cal = true;
			flip = true;
			pt = true;
			roi = true;
			tr = true;
			overlCam = true;
			overlCal = true;
		}
	};

	struct FlipStc {
		bool hor;
		bool ver;

		FlipStc() {
			reset();
		}

		void reset() {
			hor = false;
			ver = false;
		}
	};

	struct StaticOptionsStc {
		uint16_t serverPort;
		cv::Size gstreamerResolutionCapture;
		cv::Size resolutionInputStream;
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
		bool enableAdaptFps;
		std::map<fcapconstants::CamIdEn, FlipStc> flip;

		StaticOptionsStc() {
			reset();
		}

		void reset() {
			serverPort = fcapsettings::DEFAULT_SERVER_PORT;
			gstreamerResolutionCapture = fcapsettings::DEFAULT_CAPTURE_SZ;
			resolutionInputStream = fcapsettings::DEFAULT_INPUT_SZ;
			cameraFps = fcapsettings::DEFAULT_FPS;
			camL = fcapconstants::CamIdEn::CAM_0;
			camR = fcapconstants::CamIdEn::CAM_1;
			camSourceType = fcapconstants::CamSourceEn::UNSPECIFIED;
			camSource0 = "";
			camSource1 = "";
			pngOutputPath = ".";
			outputPngs = fcapsettings::DEFAULT_OUTPUT_PNGS;
			calibOutputPath = ".";
			procEnabled.reset();
			enableAdaptFps = fcapsettings::DEFAULT_ENABLE_ADAPTIVE_FPS;
			flip[fcapconstants::CamIdEn::CAM_0] = FlipStc();
			flip[fcapconstants::CamIdEn::CAM_1] = FlipStc();
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
			static void getDefaultStaticConfig(void **ppJsonObj);
			static cv::Size getSizeFromString(const std::string &x, const std::string &nameArg);
			static fcapconstants::CamSourceEn getCamSourceFromString(const std::string &x, const std::string &nameArg);
			static fcapconstants::CamIdEn getCamIdFromString(const std::string &x, const std::string &nameArg);
	};

}  // namespace fcapcfgfile

#endif  // CFGFILE_HPP_
