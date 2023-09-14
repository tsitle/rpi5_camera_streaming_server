#ifndef FRAME_QUEUE_HPP_
#define FRAME_QUEUE_HPP_

#include <condition_variable>
#include <thread>
#include <opencv2/opencv.hpp>

#include "../settings.hpp"

namespace frame {

	class FrameQueue {
		public:
			FrameQueue(bool isForJpegs);
			~FrameQueue();
			bool isQueueEmpty();
			uint32_t getDroppedFramesCount();

		private:
			bool gIsForJpegs;

		protected:
			uint8_t* gPEntries[fcapsettings::QUEUE_SIZE];
			uint32_t gEntriesRsvdSz[fcapsettings::QUEUE_SIZE];
			uint32_t gEntriesUsedSz[fcapsettings::QUEUE_SIZE];
			uint8_t gCountInBuf;
			uint8_t gIxToStore;
			uint8_t gIxToOutput;
			uint32_t gDroppedFrames;
			std::mutex gThrMtx;
			std::condition_variable gThrCond;

			//

			void log(const std::string &message);
			void appendFrameToQueueBytes(void *pData, const uint32_t dataSz);
	};

	class FrameQueueRaw : public FrameQueue {
		public:
			FrameQueueRaw();
			~FrameQueueRaw();
			void appendFrameToQueue(cv::Mat &frameRaw);
			bool getFrameFromQueue(cv::Mat &frameRawOut);

		private:
			cv::Size gFrameSz;
	};

	class FrameQueueJpeg : public FrameQueue {
		public:
			FrameQueueJpeg();
			~FrameQueueJpeg();
			void appendFrameToQueue(std::vector<unsigned char> &frameJpeg);
			bool getFrameFromQueue(uint8_t** ppData, uint32_t &dataRsvdSz, uint32_t &dataSzOut);
	};

}  // namespace frame

#endif  // FRAME_QUEUE_HPP_
