#ifndef FRAME_QUEUE_HPP_
#define FRAME_QUEUE_HPP_

#include <condition_variable>
#include <thread>
#include <opencv2/opencv.hpp>

namespace frame {

	const uint8_t QUEUE_SIZE = 5;

	class FrameQueue {
		public:
			FrameQueue(bool isForJpegs);
			~FrameQueue();
			void setFrameSize(cv::Size frameSz);
			bool isQueueEmpty();
			uint32_t getDroppedFramesCount();

		private:
			__attribute__((unused)) bool gIsForJpegs;

		protected:
			uint8_t* gPEntries[QUEUE_SIZE];
			uint32_t gEntriesRsvdSz[QUEUE_SIZE];
			uint32_t gEntriesUsedSz[QUEUE_SIZE];
			uint8_t gCountInBuf;
			uint8_t gIxToStore;
			uint8_t gIxToOutput;
			uint32_t gDroppedFrames;
			cv::Size gFrameSz;
			std::mutex gThrMtx;
			std::condition_variable gThrCond;

			//

			void appendFrameToQueueBytes(void *pData, const uint32_t dataSz);
	};

	class FrameQueueRaw : public FrameQueue {
		public:
			FrameQueueRaw();
			~FrameQueueRaw();
			void appendFrameToQueue(cv::Mat &frameRaw);
			bool getFrameFromQueue(cv::Mat &frameRawOut);
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
