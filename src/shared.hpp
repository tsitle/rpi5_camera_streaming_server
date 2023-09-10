#ifndef SHARED_HPP_
#define SHARED_HPP_

#include <condition_variable>
#include <mutex>
#include <thread>
#include <opencv2/opencv.hpp>

#include "constants.hpp"

namespace fcapshared {

	extern bool gThrVarDoStop;
	extern std::mutex gThrMtxStop;
	extern std::condition_variable gThrCondStop;

	//
	extern bool gThrVarCamStreamsOpened;
	extern std::mutex gThrMtxCamStreamsOpened;
	extern std::condition_variable gThrCondCamStreamsOpened;

	//
	extern std::vector<std::vector<unsigned char>> gThrVarOutpQueue;
	extern std::mutex gThrMtxOutpQu;
	extern std::condition_variable gThrCondOutpQu;

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
	extern RuntimeOptionsStc gThrVargRuntimeOptions;
	extern std::mutex gThrMtxRuntimeOptions;

	//
	extern RuntimeOptionsStc getRuntimeOptions();
	extern void setRuntimeOptions_outputCams(fcapconstants::OutputCamsEn val);

}  // namespace fcapshared

#endif  // SHARED_HPP_
