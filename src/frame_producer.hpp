#ifndef INCLUDED_FRAME_PRODUCER
#define INCLUDED_FRAME_PRODUCER

#include <string>
#include <thread>
#include <opencv2/opencv.hpp>

namespace fprod {

	class FrameProducer {
		public:
			static std::thread startThread();
			//
			FrameProducer();
			~FrameProducer();

		private:
			cv::VideoCapture gCapL;
			cv::VideoCapture gCapR;

			//

			static void _startThread_internal();
			//
			void log(const std::string &message);
			std::string pipe_format_x_to_str(const unsigned int formatX);
			std::string build_gstreamer_pipeline(
					const unsigned int camNr,
					const unsigned int formatX1,
					const unsigned int formatX2,
					const unsigned int fps,
					const cv::Size captureSz,
					const cv::Size outputSz);
			bool openStreams();
			void runX1(void);
	};

}  // namespace fprod

#endif
