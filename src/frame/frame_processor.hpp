#ifndef FRAME_PROCESSOR_HPP_
#define FRAME_PROCESSOR_HPP_

#include <string>
#include <opencv2/opencv.hpp>

namespace frame {

	class FrameProcessor {
		public:
			FrameProcessor();
			~FrameProcessor();
			void processFrame(cv::Mat *pFrameL, cv::Mat *pFrameR, cv::Mat **ppFrameOut);

		private:
			void log(const std::string &message);
	};

}  // namespace frame

#endif  // FRAME_PROCESSOR_HPP_
