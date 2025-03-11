#ifndef FRAME_QUEUE_RAWINPUT_HPP_
#define FRAME_QUEUE_RAWINPUT_HPP_

#include <condition_variable>
#include <thread>
#include <opencv2/opencv.hpp>

#include "../cfgfile.hpp"
#include "../settings.hpp"

namespace frame {

	class FrameQueueRawInput {
		public:
			FrameQueueRawInput();
			~FrameQueueRawInput();
			bool isQueueEmpty() const;
			uint32_t getDroppedFramesCount() const;
			void resetDroppedFramesCount();
			void appendFramesToQueue(const cv::Mat *pFrameRawL, const cv::Mat *pFrameRawR);
			bool getFramesFromQueue(cv::Mat *pFrameRawL, cv::Mat *pFrameRawR);
			void flushQueue();

		private:
			static bool gThrVarHaveFrames;
			static std::mutex gThrMtx;
			static std::condition_variable gThrCond;
			fcapcfgfile::StaticOptionsStc gStaticOptionsStc;
			std::map<fcapconstants::CamIdEn, uint8_t*[fcapsettings::IF_QUEUE_SIZE]> gPEntries;
			std::map<fcapconstants::CamIdEn, uint32_t[fcapsettings::IF_QUEUE_SIZE]> gEntriesRsvdSz;
			std::map<fcapconstants::CamIdEn, uint32_t[fcapsettings::IF_QUEUE_SIZE]> gEntriesUsedSz;
			uint8_t gCountInBuf;
			uint8_t gIxToStore;
			uint8_t gIxToOutput;
			uint32_t gDroppedFrames;
			cv::Size gFrameSz;

			//

			static void log(const std::string &message);
			void appendFrameToQueue(const fcapconstants::CamIdEn camId, const cv::Mat &frameRaw);
			void appendFrameToQueueBytes(const fcapconstants::CamIdEn camId, const void *pData, const uint32_t dataSz);
	};

}  // namespace frame

#endif  // FRAME_QUEUE_RAWINPUT_HPP_
