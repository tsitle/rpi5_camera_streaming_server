#ifndef FRAME_QUEUE_JPEG_HPP_
#define FRAME_QUEUE_JPEG_HPP_

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
			void resetDroppedFramesCount();

		private:
			bool gIsForJpegs;

		protected:
			uint8_t* gPEntries[fcapsettings::IF_QUEUE_SIZE];
			uint32_t gEntriesRsvdSz[fcapsettings::IF_QUEUE_SIZE];
			uint32_t gEntriesUsedSz[fcapsettings::IF_QUEUE_SIZE];
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

	class FrameQueueJpeg : public FrameQueue {
		public:
			FrameQueueJpeg();
			~FrameQueueJpeg();
			void appendFrameToQueue(std::vector<unsigned char> &frameJpeg);
			bool getFrameFromQueue(uint8_t** ppData, uint32_t &dataRsvdSz, uint32_t &dataSzOut);
	};

}  // namespace frame

#endif  // FRAME_QUEUE_JPEG_HPP_
