#ifndef FRAME_PRODUCER_HPP_
#define FRAME_PRODUCER_HPP_

#include <chrono>
#include <string>
#include <thread>
#include <opencv2/opencv.hpp>

#include "../cfgfile.hpp"
#include "../http/http_tcp_server.hpp"

namespace frame {

	class FrameProducer {
		public:
			static std::thread startThread(http::CbGetRunningHandlersCount cbGetRunningHandlersCount);
			//
			explicit FrameProducer(http::CbGetRunningHandlersCount cbGetRunningHandlersCount);
			~FrameProducer() = default;
			static bool waitForCamStreams();
			static bool getFlagCamStreamsOpened();
			static void setFlagRestartCamStreams();

		private:
			fcapcfgfile::StaticOptionsStc gStaticOptionsStc;
			cv::VideoCapture gCapL;
			cv::VideoCapture gCapR;
			http::CbGetRunningHandlersCount gCbGetRunningHandlersCount;
			//
			static bool gThrVarCamStreamsOpened;
			static std::mutex gThrMtxCamStreamsOpened;
			static std::condition_variable gThrCondCamStreamsOpened;
			//
			static bool gThrVarRestartCamStreams;
			static std::mutex gThrMtxRestartCamStreams;
			static std::condition_variable gThrCondRestartCamStreams;

			//

			static void _startThread_internal(http::CbGetRunningHandlersCount cbGetRunningHandlersCount);
			static void setFlagCamStreamsOpened(bool state);
			static bool _getFlagCamStreamsOpened(std::chrono::milliseconds dur);
			static bool getFlagRestartCamStreams();
			//
			static void log(const std::string &message);
			static std::string pipe_format_en_to_str(fcapconstants::GstreamerPipeFmtEn formatEn);
			std::string build_gstreamer_pipeline(const std::string &camSource, uint8_t cameraFps) const;
			bool openStreams();
			void runX1();
	};

}  // namespace frame

#endif  // FRAME_PRODUCER_HPP_
