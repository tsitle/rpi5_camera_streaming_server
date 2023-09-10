#ifndef INCLUDED_FRAME_CONSUMER
#define INCLUDED_FRAME_CONSUMER

#include <string>
#include <thread>
#include <opencv2/opencv.hpp>

#include "frame_processor.hpp"

namespace fcons {

	class FrameConsumer {
		public:
			static std::vector<cv::Mat> gThrVarInpQueueL;
			static std::vector<cv::Mat> gThrVarInpQueueR;
			static unsigned int gThrVarDroppedFramesInp;
			static unsigned int gThrVarDroppedFramesOutp;
			static std::mutex gThrMtxInpQu;
			static std::condition_variable gThrCondInpQu;

			//

			static std::thread startThread();
			//
			FrameConsumer();
			~FrameConsumer();

		private:
			std::vector<int> gCompressionParams;
			fproc::FrameProcessor gFrameProcessor;

			//

			static void _startThread_internal();
			//
			void log(const std::string &message);
			bool waitForCamStreams();
			void runX2();
			unsigned int getDroppedFramesCountInp();
			unsigned int getDroppedFramesCountOutp();
			void outputFrameToQueue(const cv::Mat &frame);
	};

}  // namespace fcons

#endif
