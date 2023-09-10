#ifndef INCLUDED_SHARED
#define INCLUDED_SHARED

#include <condition_variable>
#include <mutex>
#include <thread>
#include <opencv2/opencv.hpp>

namespace fcapshared {

	extern bool gThrVarDoStop;
	extern std::mutex gThrMtxStop;
	extern std::condition_variable gThrCondStop;

	extern bool gThrVarCamStreamsOpened;
	extern std::mutex gThrMtxCamStreamsOpened;
	extern std::condition_variable gThrCondCamStreamsOpened;

	extern std::vector<std::vector<unsigned char>> gThrVarOutpQueue;
	extern std::mutex gThrMtxOutpQu;
	extern std::condition_variable gThrCondOutpQu;

	struct RunningCltsStc {
		int runningHandlersCount;
		int runningStreamsCount;
	};
	extern RunningCltsStc gThrVarRunningCltsStc;
	extern std::unordered_map<unsigned int, bool> gThrVarRunningCltHndsMap;
	extern std::mutex gThrMtxRunningCltHnds;

}  // namespace fcapshared

#endif
