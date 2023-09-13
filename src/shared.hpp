#ifndef SHARED_HPP_
#define SHARED_HPP_

#include <condition_variable>
#include <mutex>
#include <thread>
#include <opencv2/opencv.hpp>

#include "constants.hpp"
#include "frame/frame_queue.hpp"

namespace fcapshared {

	struct RuntimeOptionsStc {
		fcapconstants::OutputCamsEn outputCams;
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
	};

	class Shared {
		public:
			static void initGlobals();
			static bool readConfigFile();
			//
			static void setFlagNeedToStop();
			static bool getFlagNeedToStop();
			//
			static RuntimeOptionsStc getRuntimeOptions();
			static void setRuntimeOptions_outputCams(fcapconstants::OutputCamsEn val);
			//
			static StaticOptionsStc getStaticOptions();

		private:
			static bool gThrVarNeedToStop;
			static std::mutex gThrMtxNeedToStop;
			static std::condition_variable gThrCondNeedToStop;
			//
			static RuntimeOptionsStc gThrVargRuntimeOptions;
			static std::mutex gThrMtxRuntimeOptions;
			//
			static StaticOptionsStc gThrVargStaticOptions;
			static std::mutex gThrMtxStaticOptions;

			//

			static void log(const std::string &message);
			static void initStcRuntimeOptions();
			static void initStcStaticOptions();
			static std::string getDefaultStaticConfig();
			static cv::Size getSizeFromString(const std::string &x, const std::string &nameArg);
			static fcapconstants::CamSourceEn getCamSourceFromString(const std::string &x, const std::string &nameArg);
			static fcapconstants::CamIdEn getCamIdFromString(const std::string &x, const std::string &nameArg);
	};

}  // namespace fcapshared

#endif  // SHARED_HPP_
