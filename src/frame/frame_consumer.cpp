#include <stdio.h>
#include <chrono>

#include "../shared.hpp"
#include "../settings.hpp"
#include "frame_consumer.hpp"

using namespace std::chrono_literals;

namespace fcons {

	std::vector<cv::Mat> FrameConsumer::gThrVarInpQueueL;
	std::vector<cv::Mat> FrameConsumer::gThrVarInpQueueR;
	unsigned int FrameConsumer::gThrVarDroppedFramesInp = 0;
	unsigned int FrameConsumer::gThrVarDroppedFramesOutp = 0;
	std::mutex FrameConsumer::gThrMtxInpQu;
	std::condition_variable FrameConsumer::gThrCondInpQu;

	// -----------------------------------------------------------------------------
	// -----------------------------------------------------------------------------

	std::thread FrameConsumer::startThread() {
		std::thread threadObj(_startThread_internal);
		return threadObj;
	}

	// -----------------------------------------------------------------------------

	FrameConsumer::FrameConsumer() : gFrameProcessor() {
		gCompressionParams.push_back(cv::IMWRITE_JPEG_QUALITY);
		gCompressionParams.push_back(fcapsettings::SETT_JPEG_QUALITY);
	}

	FrameConsumer::~FrameConsumer() {
	}

	// -----------------------------------------------------------------------------
	// -----------------------------------------------------------------------------

	void FrameConsumer::_startThread_internal() {
		if (! fcapsettings::SETT_OPEN_CAM_STREAMS) {
			return;
		}

		FrameConsumer frameConsObj = FrameConsumer();
		frameConsObj.runX2();
	}

	// -----------------------------------------------------------------------------

	void FrameConsumer::log(const std::string &message) {
		std::cout << "FCONS: " << message << std::endl;
	}

	bool FrameConsumer::waitForCamStreams() {
		std::unique_lock<std::mutex> thrLockCamStreamsOpened{fcapshared::gThrMtxCamStreamsOpened, std::defer_lock};
		bool camStreamsOpen = false;

		thrLockCamStreamsOpened.lock();
		if (fcapshared::gThrCondCamStreamsOpened.wait_for(thrLockCamStreamsOpened, 30s, []{return fcapshared::gThrVarCamStreamsOpened;})) {
			camStreamsOpen = true;
		}
		thrLockCamStreamsOpened.unlock();

		return camStreamsOpen;
	}

	void FrameConsumer::runX2() {
		const unsigned int _MAX_QUEUE_SIZE = 10;
		std::unique_lock<std::mutex> thrLockInpQu{gThrMtxInpQu, std::defer_lock};
		std::unique_lock<std::mutex> thrLockStop{fcapshared::gThrMtxStop, std::defer_lock};
		std::unique_lock<std::mutex> thrLockRunningCltHnds{fcapshared::gThrMtxRunningCltHnds, std::defer_lock};
		cv::Mat frameL;
		cv::Mat frameR;
		cv::Mat blendedImg;
		cv::Mat *pFrameOut = NULL;
		unsigned int frameNr = 0;
		char strBufPath[512];
		char strBufFn[1024];
		bool haveFrameL = false;
		bool haveFrameR = false;
		bool haveFrames = false;
		bool needToStop = false;
		bool willDiscard = false;
		bool haveClients;
		unsigned int quSzL;
		unsigned int quSzR;
		unsigned int toNeedToStop = 100;

		// wait for camera streams to be open
		needToStop = ! waitForCamStreams();
		if (needToStop) {
			log("camera streams not open. aborting.");
			thrLockStop.lock();
			fcapshared::gThrVarDoStop = true;
			thrLockStop.unlock();
			fcapshared::gThrCondStop.notify_all();
			return;
		}

		//
		/**log("STARTED");**/
		snprintf(strBufPath, sizeof(strBufPath), "%s", fcapsettings::SETT_PNG_PATH.c_str());
		try {
			unsigned int toOpts = 10;
			fcapshared::RuntimeOptionsStc opts = fcapshared::getRuntimeOptions();

			while (true) {
				// update runtime options
				if (--toOpts == 0) {
					opts = fcapshared::getRuntimeOptions();
					//
					toOpts = 10;
				}

				// read frames from queues
				haveFrameL = false;
				haveFrameR = false;
				haveFrames = false;
				thrLockInpQu.lock();
				if (gThrCondInpQu.wait_for(thrLockInpQu, 1ms, []{return ((! gThrVarInpQueueL.empty()) || (! gThrVarInpQueueR.empty()));})) {
					if (opts.outputCams != fcapconstants::OutputCamsEn::CAM_R && ! gThrVarInpQueueL.empty()) {
						frameL = gThrVarInpQueueL.back();
						gThrVarInpQueueL.pop_back();
						haveFrameL = true;
					}
					//
					if (opts.outputCams != fcapconstants::OutputCamsEn::CAM_L && ! gThrVarInpQueueR.empty()) {
						frameR = gThrVarInpQueueR.back();
						gThrVarInpQueueR.pop_back();
						haveFrameR = true;
					}
					//
					if ((opts.outputCams == fcapconstants::OutputCamsEn::CAM_L && haveFrameL) ||
							(opts.outputCams == fcapconstants::OutputCamsEn::CAM_R && haveFrameR) ||
							(opts.outputCams == fcapconstants::OutputCamsEn::CAM_BOTH && haveFrameL && haveFrameR)) {
						haveFrames = true;
					}
				}
				thrLockInpQu.unlock();

				// handle frames
				if (haveFrames) {
					++frameNr;
					willDiscard = true;
					//
					if (opts.outputCams != fcapconstants::OutputCamsEn::CAM_R &&
							fcapsettings::SETT_WRITE_PNG_TO_FILE_L) {
						snprintf(strBufFn, sizeof(strBufFn), "%s/testout-%03d-L.png", strBufPath, frameNr);
						cv::imwrite(std::string(strBufFn), frameL);
						/**log("CapL: wrote frame to '" << strBufFn << "'");**/
					}
					if (opts.outputCams != fcapconstants::OutputCamsEn::CAM_L &&
							fcapsettings::SETT_WRITE_PNG_TO_FILE_R) {
						snprintf(strBufFn, sizeof(strBufFn), "%s/testout-%03d-R.png", strBufPath, frameNr);
						cv::imwrite(std::string(strBufFn), frameR);
						/**log("CapR: wrote frame to '" << strBufFn << "'");**/
					}
					// process frames
					if (opts.outputCams == fcapconstants::OutputCamsEn::CAM_L) {
						gFrameProcessor.processFrame(&frameL, NULL, &pFrameOut);
					} else if (opts.outputCams == fcapconstants::OutputCamsEn::CAM_R) {
						gFrameProcessor.processFrame(NULL, &frameR, &pFrameOut);
					} else if (opts.outputCams == fcapconstants::OutputCamsEn::CAM_BOTH) {
						pFrameOut = &blendedImg;
						gFrameProcessor.processFrame(&frameL, &frameR, &pFrameOut);
						//
						if (fcapsettings::SETT_WRITE_PNG_TO_FILE_BLEND) {
							snprintf(strBufFn, sizeof(strBufFn), "%s/testout-%03d-B.png", strBufPath, frameNr);
							cv::imwrite(std::string(strBufFn), blendedImg);
							/**log("CapB: wrote frame to '" << strBufFn << "'");**/
						}
					}
					// output the resulting frame
					if (pFrameOut != NULL) {
						outputFrameToQueue(*pFrameOut);
						willDiscard = false;
					}
					//
					if (willDiscard) {
						log("discarded frame");
					}

					// flush queues so we don't waste RAM with old frames
					///
					thrLockRunningCltHnds.lock();
					haveClients = (fcapshared::gThrVarRunningCltsStc.runningStreamsCount > 0);
					thrLockRunningCltHnds.unlock();
					///
					thrLockInpQu.lock();
					quSzL = gThrVarInpQueueL.size();
					quSzR = gThrVarInpQueueR.size();
					if (quSzL > _MAX_QUEUE_SIZE || quSzR > _MAX_QUEUE_SIZE) {
						if (haveClients) {
							gThrVarDroppedFramesInp += (quSzL > quSzR ? quSzL : quSzR);
						}
						/**log("Emptying queues"
								", queueL=" + std::to_string(quSzL) +
								", queueR=" + std::to_string(quSzR) +
								" ...");**/
						while (! gThrVarInpQueueL.empty()) {
							gThrVarInpQueueL.pop_back();
						}
						while (! gThrVarInpQueueR.empty()) {
							gThrVarInpQueueR.pop_back();
						}
					}
					thrLockInpQu.unlock();
				} else {
					std::this_thread::sleep_for(std::chrono::milliseconds(100));
				}

				// check if we need to stop
				if (--toNeedToStop == 0) {
					thrLockStop.lock();
					if (fcapshared::gThrCondStop.wait_for(thrLockStop, 1ms, []{return fcapshared::gThrVarDoStop;})) {
						needToStop = true;
					}
					thrLockStop.unlock();
					if (needToStop) {
						/**log("Stopping...");**/
						//
						thrLockRunningCltHnds.lock();
						haveClients = (fcapshared::gThrVarRunningCltsStc.runningStreamsCount > 0);
						thrLockRunningCltHnds.unlock();
						//
						unsigned int droppedL = 0;
						unsigned int droppedR = 0;
						thrLockInpQu.lock();
						while (! gThrVarInpQueueL.empty()) {
							++droppedL;
							gThrVarInpQueueL.pop_back();
						}
						while (! gThrVarInpQueueR.empty()) {
							++droppedR;
							gThrVarInpQueueR.pop_back();
						}
						if (haveClients) {
							gThrVarDroppedFramesInp += (droppedL > droppedR ? droppedL : droppedR);
						}
						thrLockInpQu.unlock();
						break;
					}
					//
					toNeedToStop = fcapsettings::SETT_FPS;  // check every FPS * 100ms
				}
			}
		} catch (std::exception& err) {
			log("ERROR: " + std::string(err.what()));
		}

		//
		log("dropped fames inp=" + std::to_string(getDroppedFramesCountInp()));
		log("dropped fames out=" + std::to_string(getDroppedFramesCountOutp()));

		//
		log("ENDED");
	}

	unsigned int FrameConsumer::getDroppedFramesCountInp() {
		unsigned int resI;
		std::unique_lock<std::mutex> thrLockInpQu{gThrMtxInpQu, std::defer_lock};

		thrLockInpQu.lock();
		resI = gThrVarDroppedFramesInp;
		thrLockInpQu.unlock();
		return resI;
	}

	unsigned int FrameConsumer::getDroppedFramesCountOutp() {
		unsigned int resI;
		std::unique_lock<std::mutex> thrLockInpQu{gThrMtxInpQu, std::defer_lock};

		thrLockInpQu.lock();
		resI = gThrVarDroppedFramesOutp;
		thrLockInpQu.unlock();
		return resI;
	}

	void FrameConsumer::outputFrameToQueue(const cv::Mat &frame) {
		const unsigned int _MAX_QUEUE_SIZE = 10;
		std::unique_lock<std::mutex> thrLockInpQu{gThrMtxInpQu, std::defer_lock};
		std::unique_lock<std::mutex> thrLockOutpQu{fcapshared::gThrMtxOutpQu, std::defer_lock};
		std::unique_lock<std::mutex> thrLockRunningCltHnds{fcapshared::gThrMtxRunningCltHnds, std::defer_lock};
		std::vector<unsigned char> jpegFrame;
		unsigned int dropped = 0;
		bool haveClients;

		//
		thrLockRunningCltHnds.lock();
		haveClients = (fcapshared::gThrVarRunningCltsStc.runningStreamsCount > 0);
		thrLockRunningCltHnds.unlock();
		if (! haveClients) {
			return;
		}
		//
		cv::imencode(".jpg", frame, jpegFrame, gCompressionParams);
		//
		thrLockOutpQu.lock();
		while (fcapshared::gThrVarOutpQueue.size() > _MAX_QUEUE_SIZE) {
			/**log("discard frame output");**/
			fcapshared::gThrVarOutpQueue.pop_back();
			++dropped;
		}
		/**log("put frame");**/
		fcapshared::gThrVarOutpQueue.push_back(jpegFrame);
		thrLockOutpQu.unlock();
		fcapshared::gThrCondOutpQu.notify_all();
		//
		if (dropped != 0) {
			thrLockInpQu.lock();
			gThrVarDroppedFramesOutp += dropped;
			thrLockInpQu.unlock();
		}
	}

}  // namespace fcons
