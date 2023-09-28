#include <chrono>
#include <stdio.h>

#include "frame_queue_rawinput.hpp"

using namespace std::chrono_literals;

namespace frame {

	bool FrameQueueRawInput::gThrVarHaveFrames = false;
	std::mutex FrameQueueRawInput::gThrMtx;
	std::condition_variable FrameQueueRawInput::gThrCond;

	// -----------------------------------------------------------------------------
	// -----------------------------------------------------------------------------

	FrameQueueRawInput::FrameQueueRawInput() :
			gCountInBuf(0),
			gIxToStore(0),
			gIxToOutput(0),
			gDroppedFrames(0),
			gFrameSz(0, 0) {
		gStaticOptionsStc = fcapcfgfile::CfgFile::getStaticOptions();
		//
		for (int camIdInt = (int)fcapconstants::CamIdEn::CAM_0; camIdInt <= (int)fcapconstants::CamIdEn::CAM_1; camIdInt++) {
			for (uint8_t x = 0; x < fcapsettings::IF_QUEUE_SIZE; x++) {
				gEntriesRsvdSz[(fcapconstants::CamIdEn)camIdInt][x] = 64 * 1024;
				gPEntries[(fcapconstants::CamIdEn)camIdInt][x] = (uint8_t*)::malloc(gEntriesRsvdSz[(fcapconstants::CamIdEn)camIdInt][x]);
				gEntriesUsedSz[(fcapconstants::CamIdEn)camIdInt][x] = 0;
			}
		}
	}

	FrameQueueRawInput::~FrameQueueRawInput() {
		std::unique_lock<std::mutex> thrLock{gThrMtx, std::defer_lock};

		thrLock.lock();
		for (int camIdInt = (int)fcapconstants::CamIdEn::CAM_0; camIdInt <= (int)fcapconstants::CamIdEn::CAM_1; camIdInt++) {
			for (uint8_t x = 0; x < fcapsettings::IF_QUEUE_SIZE; x++) {
				if (gPEntries[(fcapconstants::CamIdEn)camIdInt][x] != NULL) {
					::free(gPEntries[(fcapconstants::CamIdEn)camIdInt][x]);
					gPEntries[(fcapconstants::CamIdEn)camIdInt][x] = NULL;
				}
				gEntriesRsvdSz[(fcapconstants::CamIdEn)camIdInt][x] = 0;
				gEntriesUsedSz[(fcapconstants::CamIdEn)camIdInt][x] = 0;
			}
		}
		gCountInBuf = 0;
		thrLock.unlock();
	}

	bool FrameQueueRawInput::isQueueEmpty() {
		bool resB;
		std::unique_lock<std::mutex> thrLock{gThrMtx, std::defer_lock};

		thrLock.lock();
		resB = (gCountInBuf == 0);
		thrLock.unlock();
		return resB;
	}

	uint32_t FrameQueueRawInput::getDroppedFramesCount() {
		uint32_t resI;
		std::unique_lock<std::mutex> thrLock{gThrMtx, std::defer_lock};

		thrLock.lock();
		resI = gDroppedFrames;
		thrLock.unlock();
		return resI;
	}

	void FrameQueueRawInput::resetDroppedFramesCount() {
		std::unique_lock<std::mutex> thrLock{gThrMtx, std::defer_lock};

		thrLock.lock();
		gDroppedFrames = 0;
		thrLock.unlock();
	}

	void FrameQueueRawInput::appendFramesToQueue(const cv::Mat *pFrameRawL, const cv::Mat *pFrameRawR) {
		std::unique_lock<std::mutex> thrLock{gThrMtx, std::defer_lock};

		if (pFrameRawL == nullptr && pFrameRawR == nullptr) {
			return;
		}
		//
		thrLock.lock();
		//
		if (gCountInBuf == fcapsettings::IF_QUEUE_SIZE) {
			++gDroppedFrames;
			++gIxToOutput;
			if (gIxToOutput == fcapsettings::IF_QUEUE_SIZE) {
				gIxToOutput = 0;
			}
		}
		//
		if (pFrameRawL != nullptr) {
			appendFrameToQueue(gStaticOptionsStc.camL, *pFrameRawL);
		}
		if (pFrameRawR != nullptr) {
			appendFrameToQueue(gStaticOptionsStc.camR, *pFrameRawR);
		}
		//
		if (++gIxToStore == fcapsettings::IF_QUEUE_SIZE) {
			gIxToStore = 0;
		}
		if (++gCountInBuf > fcapsettings::IF_QUEUE_SIZE) {
			gCountInBuf = fcapsettings::IF_QUEUE_SIZE;
		}
		//
		gThrVarHaveFrames = true;
		//
		thrLock.unlock();
		gThrCond.notify_all();
	}

	bool FrameQueueRawInput::getFramesFromQueue(cv::Mat *pFrameRawL, cv::Mat *pFrameRawR) {
		bool resB = false;
		bool haveMoreFrames = false;
		std::unique_lock<std::mutex> thrLock{gThrMtx, std::defer_lock};

		if (pFrameRawL == nullptr && pFrameRawR == nullptr) {
			return false;
		}
		//
		thrLock.lock();
		if (gThrCond.wait_for(thrLock, 1ms, []{ return gThrVarHaveFrames; })) {
			if (gCountInBuf != 0) {
				resB = true;
				if (pFrameRawL != nullptr) {
					if (gEntriesUsedSz[gStaticOptionsStc.camL][gIxToOutput] == 0) {
						resB = false;
					} else {
						*pFrameRawL = cv::Mat(gFrameSz, CV_8UC3, gPEntries[gStaticOptionsStc.camL][gIxToOutput]);
					}
				}
				if (resB && pFrameRawR != nullptr) {
					if (gEntriesUsedSz[gStaticOptionsStc.camR][gIxToOutput] == 0) {
						resB = false;
					} else {
						*pFrameRawR = cv::Mat(gFrameSz, CV_8UC3, gPEntries[gStaticOptionsStc.camR][gIxToOutput]);
					}
				}
				if (++gIxToOutput == fcapsettings::IF_QUEUE_SIZE) {
					gIxToOutput = 0;
				}
				--gCountInBuf;
				gThrVarHaveFrames = (gCountInBuf != 0);
				haveMoreFrames = gThrVarHaveFrames;
			} else {
				/*log("count 0");*/
				std::this_thread::sleep_for(std::chrono::milliseconds(1));
			}
		}
		thrLock.unlock();
		//
		if (haveMoreFrames) {
			gThrCond.notify_all();
		}
		return resB;
	}

	void FrameQueueRawInput::flushQueue() {
		std::unique_lock<std::mutex> thrLock{gThrMtx, std::defer_lock};

		thrLock.lock();
		gCountInBuf = 0;
		gIxToStore = 0;
		gIxToOutput = 0;
		gDroppedFrames = 0;
		gThrVarHaveFrames = 0;
		thrLock.unlock();
	}

	// -----------------------------------------------------------------------------
	// -----------------------------------------------------------------------------

	void FrameQueueRawInput::log(const std::string &message) {
		std::cout << "FQUEUE: [RAWINP] " << message << std::endl;
	}

	void FrameQueueRawInput::appendFrameToQueue(const fcapconstants::CamIdEn camId, const cv::Mat &frameRaw) {
		/**if (frameRaw.type() != CV_8UC3) {
			log("invalid type " + std::to_string(frameRaw.type()));
			return;
		}**/
		/**log("fR.t=" + std::to_string(frameRaw.total()) +
				", fR.c=" + std::to_string(frameRaw.channels()) +
				", fR.e=" + std::to_string(frameRaw.elemSize()));**/
		//
		if (gFrameSz.width == 0) {
			gFrameSz.width = frameRaw.cols;
			gFrameSz.height = frameRaw.rows;
		}
		//
		uint32_t newEntrySz = (uint32_t)(frameRaw.total() * frameRaw.channels());
		appendFrameToQueueBytes(camId, reinterpret_cast<unsigned char*>(&frameRaw.data[0]), newEntrySz);
	}

	void FrameQueueRawInput::appendFrameToQueueBytes(const fcapconstants::CamIdEn camId, const void *pData, const uint32_t dataSz) {
		if (dataSz > gEntriesRsvdSz[camId][gIxToStore]) {
			gPEntries[camId][gIxToStore] = (uint8_t*)::realloc(gPEntries[camId][gIxToStore], dataSz);
			gEntriesRsvdSz[camId][gIxToStore] = dataSz;
		}
		gEntriesUsedSz[camId][gIxToStore] = dataSz;
		::memcpy(gPEntries[camId][gIxToStore], pData, dataSz);
	}

}  // namespace frame
