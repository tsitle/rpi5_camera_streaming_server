#ifndef CFGFILE_HPP_
#define CFGFILE_HPP_

#include <mutex>
#include <opencv2/opencv.hpp>

#include "constants.hpp"

namespace fcapcfgfile {

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
	};

	class CfgFile {
		public:
			static bool readConfigFile(const std::string &cfgfileFn);
			//
			static StaticOptionsStc getStaticOptions();

		private:
			static bool gThrVarSetStaticOptions;
			static StaticOptionsStc gThrVarStaticOptions;
			static std::mutex gThrMtxStaticOptions;

			//

			static void log(const std::string &message);
			static void initStcStaticOptions();
			static bool fileExists(const std::string &name);
			static std::string getDefaultStaticConfig();
			static cv::Size getSizeFromString(const std::string &x, const std::string &nameArg);
			static fcapconstants::CamSourceEn getCamSourceFromString(const std::string &x, const std::string &nameArg);
			static fcapconstants::CamIdEn getCamIdFromString(const std::string &x, const std::string &nameArg);
	};

}  // namespace fcapcfgfile

#endif  // CFGFILE_HPP_
