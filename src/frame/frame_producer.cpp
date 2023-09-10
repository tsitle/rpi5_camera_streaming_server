#include <stdio.h>
#include <chrono>

#include "../shared.hpp"
#include "../settings.hpp"
#include "frame_producer.hpp"
#include "frame_consumer.hpp"

/**
 * Directly access the camera through GStreamer (which then uses Video4Linux)
 * and use threads
 *
 * Requires libcamera >= v0.1.0 for Autofocus support
 *
 * GStreamer help for libcamera:
 *   https://github.com/raspberrypi/libcamera
 */

using namespace std::chrono_literals;

namespace fprod {

	std::thread FrameProducer::startThread() {
		std::thread threadObj(_startThread_internal);
		return threadObj;
	}

	// -----------------------------------------------------------------------------
	FrameProducer::FrameProducer() {
	}

	FrameProducer::~FrameProducer() {
	}

	// -----------------------------------------------------------------------------
	// -----------------------------------------------------------------------------

	void FrameProducer::_startThread_internal() {
		if (! fcapsettings::SETT_OPEN_CAM_STREAMS) {
			return;
		}

		FrameProducer frameProdObj = FrameProducer();

		bool resB = frameProdObj.openStreams();
		if (! resB) {
			return;
		}
		//
		std::unique_lock<std::mutex> thrLockCamStreamsOpened{fcapshared::gThrMtxCamStreamsOpened, std::defer_lock};

		thrLockCamStreamsOpened.lock();
		fcapshared::gThrVarCamStreamsOpened = true;
		thrLockCamStreamsOpened.unlock();
		fcapshared::gThrCondCamStreamsOpened.notify_all();
		//
		frameProdObj.runX1();
	}

	// -----------------------------------------------------------------------------

	void FrameProducer::log(const std::string &message) {
		std::cout << "FPROD: " << message << std::endl;
	}

	std::string FrameProducer::pipe_format_x_to_str(const unsigned int formatX) {
		std::string formatStr;
		switch (formatX) {
			case fcapconstants::PIPE_FMT_X_BGR:
				formatStr = fcapconstants::PIPE_FMT_S_BGR;
				break;
			case fcapconstants::PIPE_FMT_X_BGRX:
				formatStr = fcapconstants::PIPE_FMT_S_BGRX;
				break;
			default:
				formatStr = "n/a";
		}
		return formatStr;
	}

	std::string FrameProducer::build_gstreamer_pipeline(
				const unsigned int camNr,
				const unsigned int formatX1,
				const unsigned int formatX2,
				const unsigned int fps,
				const cv::Size captureSz,
				const cv::Size outputSz) {
		const std::string format1Str = pipe_format_x_to_str(formatX1);
		const std::string format2Str = pipe_format_x_to_str(formatX2);

		std::string pipeline = "libcamerasrc auto-focus-mode=AfModeContinuous camera-name=";
		if (camNr == 0) {
			pipeline += fcapconstants::GSTREAMER_CAMNAME_ZERO;
		} else {
			pipeline += fcapconstants::GSTREAMER_CAMNAME_ONE;
		}
		pipeline += " ! video/x-raw,format=" + format1Str + ",width=" + std::to_string(captureSz.width) + ",height=" + std::to_string(captureSz.height) + ",framerate=" + std::to_string(fps) + "/1";
		if (format1Str.compare(format2Str) != 0) {
			pipeline += " ! videoconvert";
		}
		pipeline += " ! videoscale";
		pipeline += " ! video/x-raw,format=" + format2Str + ",width=" + std::to_string(outputSz.width) + ",height=" + std::to_string(outputSz.height);
		pipeline += " ! appsink";
		return pipeline;
	}

	// -----------------------------------------------------------------------------

	bool FrameProducer::openStreams() {
		std::string pipeline = build_gstreamer_pipeline(
				fcapsettings::SETT_CAM_NR_LEFT,
				fcapsettings::SETT_PIPE_FMT1,
				fcapsettings::SETT_PIPE_FMT2,
				fcapsettings::SETT_FPS,
				fcapsettings::SETT_CAPTURE_SZ,
				fcapsettings::SETT_OUTPUT_SZ
			);
		log("CapL: Opening GStreamer '" + pipeline + "'...");
		gCapL.open(pipeline, cv::CAP_GSTREAMER);

		pipeline = build_gstreamer_pipeline(
				fcapsettings::SETT_CAM_NR_RIGHT,
				fcapsettings::SETT_PIPE_FMT1,
				fcapsettings::SETT_PIPE_FMT2,
				fcapsettings::SETT_FPS,
				fcapsettings::SETT_CAPTURE_SZ,
				fcapsettings::SETT_OUTPUT_SZ
			);
		log("CapR: Opening GStreamer '" + pipeline + "'...");
		gCapR.open(pipeline, cv::CAP_GSTREAMER);

		if (! gCapL.isOpened()) {
			log("CapL: Couldn't open device");
			if (gCapR.isOpened()) {
				gCapR.release();
			}
			return false;
		}
		if (! gCapR.isOpened()) {
			log("CapR: Couldn't open device");
			if (gCapL.isOpened()) {
				gCapL.release();
			}
			return false;
		}

		/**int inpCodecTypeInt = static_cast<int>(gCapL.get(cv::CAP_PROP_FOURCC));
		char inpCodecTypeCh[] = {
				(char)(inpCodecTypeInt & 0XFF),
				(char)((inpCodecTypeInt & 0XFF00) >> 8),
				(char)((inpCodecTypeInt & 0XFF0000) >> 16),
				(char)((inpCodecTypeInt & 0XFF000000) >> 24),
				0
			};
		log("Codec: " + std::to_string(inpCodecTypeInt) + " / '" + inpCodecTypeCh + "'");**/
		cv::Size inpVideoSz = cv::Size(
				(int)gCapL.get(cv::CAP_PROP_FRAME_WIDTH),
				(int)gCapL.get(cv::CAP_PROP_FRAME_HEIGHT)
			);
		log("Size: " + std::to_string(inpVideoSz.width) + "x" + std::to_string(inpVideoSz.height));
		int inpFps = gCapL.get(cv::CAP_PROP_FPS);
		log("FPS: " + std::to_string(inpFps));

		return true;
	}

	void FrameProducer::runX1(void) {
		const unsigned int _MAX_EMPTY_FRAMES = 0;
		std::unique_lock<std::mutex> thrLockInpQu{fcons::FrameConsumer::gThrMtxInpQu, std::defer_lock};
		std::unique_lock<std::mutex> thrLockRunningCltHnds{fcapshared::gThrMtxRunningCltHnds, std::defer_lock};

		try {
			cv::Mat frameL;  // Mat is a 'n-dimensional dense array class'
			cv::Mat frameR;
			unsigned int frameNr = 0;
			unsigned int emptyFrameCnt = 0;
			bool needToStop = false;
			bool haveClients = false;
			unsigned int toHaveClients = 1;
			unsigned int toNeedToStop = 100;
			unsigned int toOpts = 10;
			fcapshared::RuntimeOptionsStc opts = fcapshared::getRuntimeOptions();

			/**auto timeStart = std::chrono::steady_clock::now();**/
			/**log("Grabbing frames...");**/
			while (true) {
				if (--toNeedToStop == 0) {
					needToStop = fcapshared::getNeedToStop();
					if (needToStop) {
						/**log("Stopping to read frames");**/
						break;
					}
					//
					toNeedToStop = fcapsettings::SETT_FPS;  // check every FPS * 100ms
				}

				// only store frames if we have clients waiting for them
				if (--toHaveClients == 0) {
					thrLockRunningCltHnds.lock();
					haveClients = (fcapshared::gThrVarRunningCltsStc.runningStreamsCount > 0);
					thrLockRunningCltHnds.unlock();
					//
					toHaveClients = 10;
				}
				if (! haveClients) {
					std::this_thread::sleep_for(std::chrono::milliseconds(100));
					continue;
				}

				// update runtime options
				if (--toOpts == 0) {
					opts = fcapshared::getRuntimeOptions();
					//
					toOpts = 10;
				}

				// grab frames
				if (opts.outputCams != fcapconstants::OutputCamsEn::CAM_R) {
					gCapL >> frameL;
					if (frameL.empty()) {
						log("CapL: empty frame");
						if (++emptyFrameCnt > _MAX_EMPTY_FRAMES) {
							log("aborting");
							break;
						}
						continue;
					}
				}
				if (opts.outputCams != fcapconstants::OutputCamsEn::CAM_L) {
					gCapR >> frameR;
					if (frameR.empty()) {
						log("CapR: empty frame");
						if (++emptyFrameCnt > _MAX_EMPTY_FRAMES) {
							log("aborting");
							break;
						}
						continue;
					}
				}
				++frameNr;

				// store frames
				thrLockInpQu.lock();
				if (opts.outputCams != fcapconstants::OutputCamsEn::CAM_R) {
					fcons::FrameConsumer::gThrVarInpQueueL.push_back(frameL);
				}
				if (opts.outputCams != fcapconstants::OutputCamsEn::CAM_L) {
					fcons::FrameConsumer::gThrVarInpQueueR.push_back(frameR);
				}
				thrLockInpQu.unlock();
				fcons::FrameConsumer::gThrCondInpQu.notify_all();

				//
				std::this_thread::sleep_for(std::chrono::milliseconds((unsigned int)((1.0 / (float)fcapsettings::SETT_FPS) * 1000.0)) / 2);

				//
				/*if (frameNr == fcapsettings::SETT_FPS * 5) {
					log("Setting STOP flag...");
					fcapshared::setNeedToStop();
					//
					break;
				}*/
			}

			//
			/**auto timeEnd = std::chrono::steady_clock::now();
			const std::string pipeFormat1Str = pipe_format_x_to_str(fcapsettings::SETT_PIPE_FMT1);
			const std::string pipeFormat2Str = pipe_format_x_to_str(fcapsettings::SETT_PIPE_FMT2);
			log("PFMT(" + pipeFormat1Str + "/" + pipeFormat2Str + "): Elapsed time: " +
					std::to_string(std::chrono::duration_cast<std::chrono::milliseconds>(timeEnd - timeStart).count()) +
					" ms");**/
		} catch (std::exception& err) {
			log("ERROR: " + std::string(err.what()));
		}

		//
		log("Releasing cams...");
		std::this_thread::sleep_for(std::chrono::milliseconds(500));
		gCapL.release();
		gCapR.release();
		/**log("done.");**/

		//
		log("Wait for queues...");
		bool needToWait = true;
		unsigned int waitCnt = 0;
		while (needToWait) {
			if (waitCnt != 0) {
				log("still waiting for queues...");
			}
			std::this_thread::sleep_for(std::chrono::milliseconds(1000));
			thrLockInpQu.lock();
			needToWait = ! (fcons::FrameConsumer::gThrVarInpQueueL.empty() && fcons::FrameConsumer::gThrVarInpQueueR.empty());
			/*if (needToWait) {
				log("queueL size=" + std::to_string(gThrVarInpQueueL.size()) +
						", queueR size=" + std::to_string(gThrVarInpQueueR.size()));
			}*/
			thrLockInpQu.unlock();
			//
			if (needToWait) {
				thrLockRunningCltHnds.lock();
				needToWait = (fcapshared::gThrVarRunningCltsStc.runningStreamsCount > 0);
				thrLockRunningCltHnds.unlock();
			}
			//
			if (needToWait && ++waitCnt == 10) {
				log("dropping remaining frames");
				break;
			}
		}

		//
		fcapshared::setNeedToStop();
		//
		log("ENDED");
	}

}  // namespace fprod
