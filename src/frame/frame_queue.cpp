#include <chrono>
#include <stdio.h>

#include "frame_queue.hpp"

using namespace std::chrono_literals;

namespace frame {

	FrameQueue::FrameQueue(bool isForJpegs) :
			gIsForJpegs(isForJpegs),
			gCountInBuf(0),
			gIxToStore(0),
			gIxToOutput(0),
			gDroppedFrames(0) {
		#define _UNUSED(x) (void)(x)
		_UNUSED(gIsForJpegs);
		#undef _UNUSED
		//
		for (uint8_t x = 0; x < fcapsettings::QUEUE_SIZE; x++) {
			gEntriesRsvdSz[x] = 64 * 1024;
			gPEntries[x] = (uint8_t*)::malloc(gEntriesRsvdSz[x]);
			gEntriesUsedSz[x] = 0;
		}
	}

	FrameQueue::~FrameQueue() {
		std::unique_lock<std::mutex> thrLock{gThrMtx, std::defer_lock};

		thrLock.lock();
		for (uint8_t x = 0; x < fcapsettings::QUEUE_SIZE; x++) {
			if (gPEntries[x] != NULL) {
				::free(gPEntries[x]);
				gPEntries[x] = NULL;
			}
			gEntriesRsvdSz[x] = 0;
			gEntriesUsedSz[x] = 0;
		}
		gCountInBuf = 0;
		thrLock.unlock();
	}

	bool FrameQueue::isQueueEmpty() {
		bool resB;
		std::unique_lock<std::mutex> thrLock{gThrMtx, std::defer_lock};

		thrLock.lock();
		resB = (gCountInBuf == 0);
		thrLock.unlock();
		return resB;
	}

	uint32_t FrameQueue::getDroppedFramesCount() {
		uint32_t resI;
		std::unique_lock<std::mutex> thrLock{gThrMtx, std::defer_lock};

		thrLock.lock();
		resI = gDroppedFrames;
		thrLock.unlock();
		return resI;
	}

	// -----------------------------------------------------------------------------
	// -----------------------------------------------------------------------------

	void FrameQueue::log(const std::string &message) {
		std::cout << "FQUEUE: [" << (gIsForJpegs ? "J" : "R") << "] " << message << std::endl;
	}

	void FrameQueue::appendFrameToQueueBytes(void *pData, const uint32_t dataSz) {
		std::unique_lock<std::mutex> thrLock{gThrMtx, std::defer_lock};

		thrLock.lock();
		if (gCountInBuf == fcapsettings::QUEUE_SIZE) {
			++gDroppedFrames;
			++gIxToOutput;
			if (gIxToOutput == fcapsettings::QUEUE_SIZE) {
				gIxToOutput = 0;
			}
		}
		//
		if (dataSz > gEntriesRsvdSz[gIxToStore]) {
			/**if (gIsForJpegs) {
				log("app _re " + std::to_string(dataSz) + " ix=" + std::to_string(gIxToStore));
			}**/
			gPEntries[gIxToStore] = (uint8_t*)::realloc(gPEntries[gIxToStore], dataSz);
			gEntriesRsvdSz[gIxToStore] = dataSz;
		}
		gEntriesUsedSz[gIxToStore] = dataSz;
		/**if (gIsForJpegs) {
			log("app _cp " + std::to_string(dataSz) + " ix=" + std::to_string(gIxToStore));
		}**/
		::memcpy(gPEntries[gIxToStore], pData, dataSz);
		//
		/**if (gIsForJpegs) {
			char strBuf[128];
			snprintf(strBuf, sizeof(strBuf), "__b 0x%02X%02X 0x%02X%02X", gPEntries[gIxToStore][0], gPEntries[gIxToStore][1], gPEntries[gIxToStore][dataSz - 2], gPEntries[gIxToStore][dataSz - 1]);
			log("put sz=" + std::to_string(gEntriesUsedSz[gIxToStore]) + ": " + strBuf);
		}**/
		//
		if (++gIxToStore == fcapsettings::QUEUE_SIZE) {
			gIxToStore = 0;
		}
		if (++gCountInBuf > fcapsettings::QUEUE_SIZE) {
			gCountInBuf = fcapsettings::QUEUE_SIZE;
		}
		/**if (gIsForJpegs) { log("app end"); }**/
		thrLock.unlock();
		gThrCond.notify_all();
	}

	// -----------------------------------------------------------------------------
	// -----------------------------------------------------------------------------

	FrameQueueRaw::FrameQueueRaw() :
			FrameQueue(false),
			gFrameSz(0, 0) {
	}

	FrameQueueRaw::~FrameQueueRaw() {
	}

	void FrameQueueRaw::appendFrameToQueue(cv::Mat &frameRaw) {
		std::unique_lock<std::mutex> thrLock{gThrMtx, std::defer_lock};

		/**if (frameRaw.type() != CV_8UC3) {
			log("invalid type " + std::to_string(frameRaw.type()));
			return;
		}**/
		/**log("fR.t=" + std::to_string(frameRaw.total()) +
				", fR.c=" + std::to_string(frameRaw.channels()) +
				", fR.e=" + std::to_string(frameRaw.elemSize()));**/
		thrLock.lock();
		if (gFrameSz.width == 0) {
			gFrameSz.width = frameRaw.cols;
			gFrameSz.height = frameRaw.rows;
		}
		thrLock.unlock();
		//
		uint32_t newEntrySz = (uint32_t)(frameRaw.total() * frameRaw.channels());
		appendFrameToQueueBytes(reinterpret_cast<unsigned char*>(&frameRaw.data[0]), newEntrySz);
	}

	bool FrameQueueRaw::getFrameFromQueue(cv::Mat &frameRawOut) {
		bool resB = false;
		std::unique_lock<std::mutex> thrLock{gThrMtx, std::defer_lock};

		thrLock.lock();
		if (gThrCond.wait_for(thrLock, 1ms, []{ return true; })) {
			if (gCountInBuf != 0) {
				/**log("get beg ix=" + std::to_string(gIxToOutput));**/
				frameRawOut = cv::Mat(gFrameSz, CV_8UC3, gPEntries[gIxToOutput]);
				/**log("get end");**/
				if (++gIxToOutput == fcapsettings::QUEUE_SIZE) {
					gIxToOutput = 0;
				}
				--gCountInBuf;
				resB = true;
			} else {
				/*log("count 0");*/
				std::this_thread::sleep_for(std::chrono::milliseconds(1));
			}
		}
		thrLock.unlock();
		return resB;
	}

	// -----------------------------------------------------------------------------
	// -----------------------------------------------------------------------------

	FrameQueueJpeg::FrameQueueJpeg() :
			FrameQueue(true) {
	}

	FrameQueueJpeg::~FrameQueueJpeg() {
	}

	void FrameQueueJpeg::appendFrameToQueue(std::vector<unsigned char> &frameJpeg) {
		appendFrameToQueueBytes(reinterpret_cast<uint8_t*>(frameJpeg.data()), frameJpeg.size());
	}

	bool FrameQueueJpeg::getFrameFromQueue(uint8_t** ppData, uint32_t &dataRsvdSz, uint32_t &dataSzOut) {
		bool resB = false;
		std::unique_lock<std::mutex> thrLock{gThrMtx, std::defer_lock};

		thrLock.lock();
		if (gThrCond.wait_for(thrLock, 1ms, []{ return true; })) {
			if (gCountInBuf != 0) {
				/**log("get beg ix=" + std::to_string(gIxToOutput));**/
				uint32_t entrySz = gEntriesUsedSz[gIxToOutput];
				//
				if (dataRsvdSz == 0) {
					dataRsvdSz = entrySz + 1024;
					/**log("get _ma " + std::to_string(dataRsvdSz));**/
					*ppData = (uint8_t*)::malloc(dataRsvdSz);
				} else if (entrySz > dataRsvdSz) {
					dataRsvdSz = entrySz;
					/**log("get _ra " + std::to_string(dataRsvdSz));**/
					*ppData = (uint8_t*)::realloc(*ppData, dataRsvdSz);
				}
				if (*ppData != NULL) {
					/**char strBuf1[128];
					snprintf(strBuf1, sizeof(strBuf1), "__b 0x%02X%02X 0x%02X%02X", gPEntries[gIxToOutput][0], gPEntries[gIxToOutput][1], gPEntries[gIxToOutput][entrySz - 2], gPEntries[gIxToOutput][entrySz - 1]);
					log("get _src " + strBuf1);**/
					//
					/**log("get _cp " + std::to_string(entrySz));**/
					::memcpy(*ppData, gPEntries[gIxToOutput], entrySz);
					dataSzOut = entrySz;
					//
					/**char strBuf2[128];
					snprintf(strBuf2, sizeof(strBuf2), "__b 0x%02X%02X 0x%02X%02X", (*ppData)[0], (*ppData)[1], (*ppData)[entrySz - 2], (*ppData)[entrySz - 1]);
					log("get _dst " + strBuf2);**/
					//
					resB = true;
				} else {
					dataSzOut = 0;
				}
				if (++gIxToOutput == fcapsettings::QUEUE_SIZE) {
					gIxToOutput = 0;
				}
				--gCountInBuf;
				/**log("get end");**/
			} else {
				/*log("count 0");*/
				std::this_thread::sleep_for(std::chrono::milliseconds(1));
			}
		}
		thrLock.unlock();
		return resB;
	}

}  // namespace frame
