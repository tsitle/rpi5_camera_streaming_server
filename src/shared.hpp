#ifndef SHARED_HPP_
#define SHARED_HPP_

#include <condition_variable>
#include <mutex>
#include <thread>
#include <opencv2/opencv.hpp>

#include "constants.hpp"
#include "frame/frame_queue.hpp"

namespace fcapshared {

	//
	extern bool gThrVarCamStreamsOpened;
	extern std::mutex gThrMtxCamStreamsOpened;
	extern std::condition_variable gThrCondCamStreamsOpened;

	//
	extern frame::FrameQueueJpeg gFrameQueueOutp;

	//
	struct RunningCltsStc {
		int runningHandlersCount;
		int runningStreamsCount;
	};
	extern RunningCltsStc gThrVarRunningCltsStc;
	extern std::unordered_map<unsigned int, bool> gThrVarRunningCltHndsMap;
	extern std::mutex gThrMtxRunningCltHnds;

	//
	struct RuntimeOptionsStc {
		fcapconstants::OutputCamsEn outputCams;
	};

	class Shared {
		public:
			static void initGlobals();
			//
			static void setFlagNeedToStop();
			static bool getFlagNeedToStop();
			//
			static RuntimeOptionsStc getRuntimeOptions();
			static void setRuntimeOptions_outputCams(fcapconstants::OutputCamsEn val);

		private:
			static bool gThrVarNeedToStop;
			static std::mutex gThrMtxNeedToStop;
			static std::condition_variable gThrCondNeedToStop;
			//
			static RuntimeOptionsStc gThrVargRuntimeOptions;
			static std::mutex gThrMtxRuntimeOptions;
	};

}  // namespace fcapshared

#endif  // SHARED_HPP_
