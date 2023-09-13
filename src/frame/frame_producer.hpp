#ifndef FRAME_PRODUCER_HPP_
#define FRAME_PRODUCER_HPP_

#include <chrono>
#include <string>
#include <thread>
#include <opencv2/opencv.hpp>

#include "../http/http_tcp_server.hpp"

namespace frame {

	class FrameProducer {
		public:
			static std::thread startThread(http::CbGetRunningHandlersCount cbGetRunningHandlersCount);
			//
			FrameProducer(http::CbGetRunningHandlersCount cbGetRunningHandlersCount);
			~FrameProducer();
			static bool waitForCamStreams();
			static bool getFlagCamStreamsOpened();

		private:
			cv::VideoCapture gCapL;
			cv::VideoCapture gCapR;
			http::CbGetRunningHandlersCount gCbGetRunningHandlersCount;
			//
			static bool gThrVarCamStreamsOpened;
			static std::mutex gThrMtxCamStreamsOpened;
			static std::condition_variable gThrCondCamStreamsOpened;

			//

			static void _startThread_internal(http::CbGetRunningHandlersCount cbGetRunningHandlersCount);
			static void setFlagCamStreamsOpened(const bool state);
			static bool _getFlagCamStreamsOpened(const std::chrono::milliseconds dur);
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

}  // namespace frame

#endif  // FRAME_PRODUCER_HPP_
