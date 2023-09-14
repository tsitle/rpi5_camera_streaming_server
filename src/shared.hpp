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
		uint8_t cameraFps;
	};

	class Shared {
		public:
			static void setFlagNeedToStop();
			static bool getFlagNeedToStop();
			//
			static RuntimeOptionsStc getRuntimeOptions();
			static void setRuntimeOptions_outputCams(fcapconstants::OutputCamsEn val);
			static void setRuntimeOptions_cameraFps(uint8_t val);

		private:
			static bool gThrVarNeedToStop;
			static std::mutex gThrMtxNeedToStop;
			static std::condition_variable gThrCondNeedToStop;
			//
			static bool gThrVarSetRuntimeOptions;
			static RuntimeOptionsStc gThrVarRuntimeOptions;
			static std::mutex gThrMtxRuntimeOptions;

			//

			static void initStcRuntimeOptions();
	};

}  // namespace fcapshared

#endif  // SHARED_HPP_
