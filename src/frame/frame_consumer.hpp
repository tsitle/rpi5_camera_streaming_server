#ifndef FRAME_CONSUMER_HPP_
#define FRAME_CONSUMER_HPP_

#include <string>
#include <thread>
#include <opencv2/opencv.hpp>

#include "frame_processor.hpp"
#include "frame_queue.hpp"

namespace frame {

	class FrameConsumer {
		public:
			static FrameQueueRaw gFrameQueueInpL;
			static FrameQueueRaw gFrameQueueInpR;

			static std::thread startThread();
			//
			FrameConsumer();
			~FrameConsumer();

		private:
			std::vector<int> gCompressionParams;
			FrameProcessor gFrameProcessor;

			//

			static void _startThread_internal();
			//
			void log(const std::string &message);
			bool waitForCamStreams();
			void runX2();
			void outputFrameToQueue(const cv::Mat &frame);
	};

}  // namespace frame

#endif  // FRAME_CONSUMER_HPP_
