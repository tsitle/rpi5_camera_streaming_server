#ifndef FRAME_CONSUMER_HPP_
#define FRAME_CONSUMER_HPP_

#include <string>
#include <thread>
#include <opencv2/opencv.hpp>

#include "../constants.hpp"
#include "../cfgfile.hpp"
#include "../http/http_tcp_server.hpp"
#include "frame_processor.hpp"
#include "frame_queue_rawinput.hpp"

namespace frame {

	class FrameConsumer {
		public:
			static FrameQueueRawInput gFrameQueueInp;

			static std::thread startThread(
					http::CbGetRunningHandlersCount cbGetRunningHandlersCount,
					http::CbBroadcastFrameToStreamingClients cbBroadcastFrameToStreamingClients);
			//
			FrameConsumer(
					http::CbGetRunningHandlersCount cbGetRunningHandlersCount,
					http::CbBroadcastFrameToStreamingClients cbBroadcastFrameToStreamingClients);
			~FrameConsumer();

		private:
			fcapcfgfile::StaticOptionsStc gStaticOptionsStc;
			std::vector<int> gCompressionParams;
			FrameProcessor gFrameProcessor;
			http::CbGetRunningHandlersCount gCbGetRunningHandlersCount;
			http::CbBroadcastFrameToStreamingClients gCbBroadcastFrameToStreamingClients;

			//

			static void _startThread_internal(
					http::CbGetRunningHandlersCount cbGetRunningHandlersCount,
					http::CbBroadcastFrameToStreamingClients cbBroadcastFrameToStreamingClients);
			//
			void log(const std::string &message);
			void runX2();
			void outputFrameToQueue(const cv::Mat &frame);
	};

}  // namespace frame

#endif  // FRAME_CONSUMER_HPP_
