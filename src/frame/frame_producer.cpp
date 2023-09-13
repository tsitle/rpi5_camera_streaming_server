#include <stdio.h>
#include <chrono>

#include "../shared.hpp"
#include "../settings.hpp"
#include "../http/http_tcp_server.hpp"
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

namespace frame {

	bool FrameProducer::gThrVarCamStreamsOpened = false;
	std::mutex FrameProducer::gThrMtxCamStreamsOpened;
	std::condition_variable FrameProducer::gThrCondCamStreamsOpened;

	// -----------------------------------------------------------------------------

	std::thread FrameProducer::startThread(http::CbGetRunningHandlersCount cbGetRunningHandlersCount) {
		std::thread threadObj(_startThread_internal, cbGetRunningHandlersCount);
		return threadObj;
	}

	// -----------------------------------------------------------------------------

	FrameProducer::FrameProducer(http::CbGetRunningHandlersCount cbGetRunningHandlersCount) :
			gCbGetRunningHandlersCount(cbGetRunningHandlersCount) {
		gStaticOptionsStc = fcapshared::Shared::getStaticOptions();
	}

	FrameProducer::~FrameProducer() {
	}

	bool FrameProducer::waitForCamStreams() {
		return _getFlagCamStreamsOpened(std::chrono::milliseconds(30000));
	}

	bool FrameProducer::getFlagCamStreamsOpened() {
		return _getFlagCamStreamsOpened(std::chrono::milliseconds(1));
	}

	// -----------------------------------------------------------------------------
	// -----------------------------------------------------------------------------

	void FrameProducer::_startThread_internal(http::CbGetRunningHandlersCount cbGetRunningHandlersCount) {
		if (! fcapsettings::SETT_OPEN_CAM_STREAMS) {
			return;
		}

		FrameProducer frameProdObj(cbGetRunningHandlersCount);

		bool resB = frameProdObj.openStreams();
		if (! resB) {
			return;
		}
		//
		setFlagCamStreamsOpened(true);
		//
		frameProdObj.runX1();
	}

	void FrameProducer::setFlagCamStreamsOpened(const bool state) {
		std::unique_lock<std::mutex> thrLock{gThrMtxCamStreamsOpened, std::defer_lock};

		thrLock.lock();
		gThrVarCamStreamsOpened = state;
		thrLock.unlock();
		gThrCondCamStreamsOpened.notify_all();
	}

	bool FrameProducer::_getFlagCamStreamsOpened(const std::chrono::milliseconds dur) {
		bool resB = false;
		std::unique_lock<std::mutex> thrLock{gThrMtxCamStreamsOpened, std::defer_lock};

		thrLock.lock();
		if (gThrCondCamStreamsOpened.wait_for(thrLock, dur, []{return gThrVarCamStreamsOpened;})) {
			resB = true;
		}
		thrLock.unlock();
		return resB;
	}

	// -----------------------------------------------------------------------------

	void FrameProducer::log(const std::string &message) {
		std::cout << "FPROD: " << message << std::endl;
	}

	std::string FrameProducer::pipe_format_x_to_str(const uint8_t formatX) {
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

	std::string FrameProducer::build_gstreamer_pipeline(const std::string camSource) {
		const std::string format1Str = pipe_format_x_to_str(fcapsettings::SETT_PIPE_FMT1);
		const std::string format2Str = pipe_format_x_to_str(fcapsettings::SETT_PIPE_FMT2);

		std::string pipeline = "libcamerasrc auto-focus-mode=AfModeContinuous camera-name=";
		pipeline += camSource;
		pipeline += " ! video/x-raw,format=" + format1Str +
				",width=" + std::to_string(gStaticOptionsStc.resolutionCapture.width) +
				",height=" + std::to_string(gStaticOptionsStc.resolutionCapture.height) +
				",framerate=" + std::to_string(gStaticOptionsStc.fps) + "/1";
		if (format1Str.compare(format2Str) != 0) {
			pipeline += " ! videoconvert";
		}
		pipeline += " ! videoscale";
		pipeline += " ! video/x-raw,format=" + format2Str +
				",width=" + std::to_string(gStaticOptionsStc.resolutionOutput.width) +
				",height=" + std::to_string(gStaticOptionsStc.resolutionOutput.height);
		pipeline += " ! appsink";
		return pipeline;
	}

	// -----------------------------------------------------------------------------

	bool FrameProducer::openStreams() {
		std::string camSource = (gStaticOptionsStc.camL == fcapconstants::CamIdEn::CAM_0 ? gStaticOptionsStc.camSource0 : gStaticOptionsStc.camSource1);
		std::string pipeline = build_gstreamer_pipeline(camSource);
		log("CapL: Opening GStreamer '" + pipeline + "'...");
		gCapL.open(pipeline, cv::CAP_GSTREAMER);

		camSource = (gStaticOptionsStc.camR == fcapconstants::CamIdEn::CAM_0 ? gStaticOptionsStc.camSource0 : gStaticOptionsStc.camSource1);
		pipeline = build_gstreamer_pipeline(camSource);
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
		const uint32_t _MAX_EMPTY_FRAMES = 0;

		try {
			cv::Mat frameL;  // Mat is a 'n-dimensional dense array class'
			cv::Mat frameR;
			uint32_t frameNr = 0;
			uint32_t emptyFrameCnt = 0;
			bool needToStop = false;
			bool haveClients = false;
			uint32_t toHaveClients = 1;
			uint32_t toNeedToStop = 100;
			uint32_t toOpts = 10;
			fcapshared::RuntimeOptionsStc opts = fcapshared::Shared::getRuntimeOptions();

			/**auto timeStart = std::chrono::steady_clock::now();**/
			/**log("Grabbing frames...");**/
			while (true) {
				if (--toNeedToStop == 0) {
					needToStop = fcapshared::Shared::getFlagNeedToStop();
					if (needToStop) {
						/**log("Stopping to read frames");**/
						break;
					}
					//
					toNeedToStop = gStaticOptionsStc.fps;  // check every FPS * 100ms
				}

				// only store frames if we have clients waiting for them
				if (--toHaveClients == 0) {
					haveClients = (gCbGetRunningHandlersCount() > 0);
					//
					toHaveClients = 10;
				}
				if (! haveClients) {
					std::this_thread::sleep_for(std::chrono::milliseconds(100));
					continue;
				}

				// update runtime options
				if (--toOpts == 0) {
					opts = fcapshared::Shared::getRuntimeOptions();
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
				if (opts.outputCams != fcapconstants::OutputCamsEn::CAM_R) {
					/**log("app L");**/
					FrameConsumer::gFrameQueueInpL.appendFrameToQueue(frameL);
				}
				if (opts.outputCams != fcapconstants::OutputCamsEn::CAM_L) {
					/**log("app R");**/
					FrameConsumer::gFrameQueueInpR.appendFrameToQueue(frameR);
				}

				//
				std::this_thread::sleep_for(std::chrono::milliseconds((uint32_t)((1.0 / (float)(gStaticOptionsStc.fps * 5)) * 1000.0)));

				//
				/*if (frameNr == gStaticOptionsStc.fps * 5) {
					log("Setting STOP flag...");
					fcapshared::Shared::setFlagNeedToStop();
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
		uint32_t waitCnt = 0;
		while (needToWait) {
			if (waitCnt != 0) {
				log("still waiting for queues...");
			}
			std::this_thread::sleep_for(std::chrono::milliseconds(1000));
			needToWait = ! (FrameConsumer::gFrameQueueInpL.isQueueEmpty() &&
					FrameConsumer::gFrameQueueInpR.isQueueEmpty());
			//
			if (needToWait) {
				needToWait = (gCbGetRunningHandlersCount() > 0);
			}
			//
			if (needToWait && ++waitCnt == 10) {
				log("dropping remaining frames");
				break;
			}
		}

		//
		fcapshared::Shared::setFlagNeedToStop();
		//
		log("ENDED");
	}

}  // namespace frame
