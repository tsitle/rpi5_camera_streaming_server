#include <stdio.h>
#include <chrono>

#include "../shared.hpp"
#include "../settings.hpp"
#include "../cfgfile.hpp"
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

	//
	bool FrameProducer::gThrVarCamStreamsOpened = false;
	std::mutex FrameProducer::gThrMtxCamStreamsOpened;
	std::condition_variable FrameProducer::gThrCondCamStreamsOpened;

	//
	bool FrameProducer::gThrVarRestartCamStreams = false;
	std::mutex FrameProducer::gThrMtxRestartCamStreams;
	std::condition_variable FrameProducer::gThrCondRestartCamStreams;

	// -----------------------------------------------------------------------------

	std::thread FrameProducer::startThread(http::CbGetRunningHandlersCount cbGetRunningHandlersCount) {
		std::thread threadObj(_startThread_internal, cbGetRunningHandlersCount);
		return threadObj;
	}

	// -----------------------------------------------------------------------------

	FrameProducer::FrameProducer(http::CbGetRunningHandlersCount cbGetRunningHandlersCount) :
			gCbGetRunningHandlersCount(cbGetRunningHandlersCount) {
		gStaticOptionsStc = fcapcfgfile::CfgFile::getStaticOptions();
	}

	FrameProducer::~FrameProducer() {
	}

	bool FrameProducer::waitForCamStreams() {
		return _getFlagCamStreamsOpened(std::chrono::milliseconds(30000));
	}

	bool FrameProducer::getFlagCamStreamsOpened() {
		return _getFlagCamStreamsOpened(std::chrono::milliseconds(1));
	}

	void FrameProducer::setFlagRestartCamStreams() {
		std::unique_lock<std::mutex> thrLock{gThrMtxRestartCamStreams, std::defer_lock};

		thrLock.lock();
		gThrVarRestartCamStreams = true;
		thrLock.unlock();
		gThrCondRestartCamStreams.notify_all();
	}

	// -----------------------------------------------------------------------------
	// -----------------------------------------------------------------------------

	void FrameProducer::_startThread_internal(http::CbGetRunningHandlersCount cbGetRunningHandlersCount) {
		if (! fcapsettings::DBG_OPEN_CAM_STREAMS) {
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
		if (gThrCondCamStreamsOpened.wait_for(thrLock, dur, []{ return gThrVarCamStreamsOpened; })) {
			resB = true;
		}
		thrLock.unlock();
		return resB;
	}

	bool FrameProducer::getFlagRestartCamStreams() {
		bool resB = false;
		std::unique_lock<std::mutex> thrLock{gThrMtxRestartCamStreams, std::defer_lock};

		thrLock.lock();
		if (gThrCondRestartCamStreams.wait_for(thrLock, 1ms, []{ return gThrVarRestartCamStreams; })) {
			resB = true;
			gThrVarRestartCamStreams = false;
		}
		thrLock.unlock();
		return resB;
	}

	// -----------------------------------------------------------------------------

	void FrameProducer::log(const std::string &message) {
		std::cout << "FPROD: " << message << std::endl;
	}

	std::string FrameProducer::pipe_format_en_to_str(const fcapconstants::GstreamerPipeFmtEn formatEn) {
		std::string formatStr;
		switch (formatEn) {
			case fcapconstants::GstreamerPipeFmtEn::BGR:
				formatStr = fcapconstants::GSTREAMER_PIPE_FMT_S_BGR;
				break;
			case fcapconstants::GstreamerPipeFmtEn::BGRX:
				formatStr = fcapconstants::GSTREAMER_PIPE_FMT_S_BGRX;
				break;
			default:
				formatStr = "n/a";
		}
		return formatStr;
	}

	std::string FrameProducer::build_gstreamer_pipeline(const std::string camSource, const uint8_t cameraFps) {
		const std::string format1Str = pipe_format_en_to_str(fcapconstants::GSTREAMER_PIPE_FMT1);
		const std::string format2Str = pipe_format_en_to_str(fcapconstants::GSTREAMER_PIPE_FMT2);

		std::string pipeline = "libcamerasrc auto-focus-mode=AfModeContinuous camera-name=";
		pipeline += camSource;
		pipeline += " ! video/x-raw,format=" + format1Str +
				",width=" + std::to_string(gStaticOptionsStc.gstreamerResolutionCapture.width) +
				",height=" + std::to_string(gStaticOptionsStc.gstreamerResolutionCapture.height) +
				",framerate=" + std::to_string(cameraFps) + "/1";
		if (format1Str.compare(format2Str) != 0) {
			pipeline += " ! videoconvert";
		}
		pipeline += " ! videoscale";
		pipeline += " ! video/x-raw,format=" + format2Str +
				",width=" + std::to_string(gStaticOptionsStc.resolutionInputStream.width) +
				",height=" + std::to_string(gStaticOptionsStc.resolutionInputStream.height);
		pipeline += " ! appsink";
		return pipeline;
	}

	// -----------------------------------------------------------------------------

	bool FrameProducer::openStreams() {
		std::string camSourceL = (gStaticOptionsStc.camL == fcapconstants::CamIdEn::CAM_0 ?
				gStaticOptionsStc.camSource0 : gStaticOptionsStc.camSource1);
		std::string camSourceR = (gStaticOptionsStc.camR == fcapconstants::CamIdEn::CAM_0 ?
				gStaticOptionsStc.camSource0 : gStaticOptionsStc.camSource1);
		cv::VideoCapture* pCapInfo;
		fcapshared::RuntimeOptionsStc optsRt = fcapshared::Shared::getRuntimeOptions();

		if (gCapL.isOpened()) {
			gCapL.release();
		}
		if (gCapR.isOpened()) {
			gCapR.release();
		}
		//
		if (gStaticOptionsStc.camSourceType == fcapconstants::CamSourceEn::GSTREAMER) {
			std::string pipeline;

			if (camSourceL.length() != 0) {
				pipeline = build_gstreamer_pipeline(camSourceL, optsRt.cameraFps);
				log("CapL: Opening type=GStreamer '" + pipeline + "'...");
				gCapL.open(pipeline, cv::CAP_GSTREAMER);
			}

			if (camSourceR.length() != 0) {
				pipeline = build_gstreamer_pipeline(camSourceR, optsRt.cameraFps);
				log("CapR: Opening type=GStreamer '" + pipeline + "'...");
				gCapR.open(pipeline, cv::CAP_GSTREAMER);
			}
		} else if (gStaticOptionsStc.camSourceType == fcapconstants::CamSourceEn::MJPEG) {
			if (camSourceL.length() != 0) {
				log("CapL: Opening type=MJPEG '" + camSourceL + "'...");
				gCapL.open(camSourceL, cv::CAP_ANY);
			}

			if (camSourceR.length() != 0) {
				log("CapR: Opening type=MJPEG '" + camSourceR + "'...");
				gCapR.open(camSourceR, cv::CAP_ANY);
			}
		} else {
			if (camSourceL.length() != 0) {
				log("CapL: Opening type=Unspecified '" + camSourceL + "'...");
				if (camSourceL.compare("0") == 0) {
					gCapL.open(0, cv::CAP_V4L);
				} else if (camSourceL.compare("1") == 0) {
					gCapL.open(1, cv::CAP_V4L);
				} else {
					gCapL.open(camSourceL, cv::CAP_ANY);
				}
			}

			if (camSourceR.length() != 0) {
				log("CapR: Opening type=Unspecified '" + camSourceR + "'...");
				if (camSourceR.compare("0") == 0) {
					gCapR.open(0, cv::CAP_V4L);
				} else if (camSourceR.compare("1") == 0) {
					gCapR.open(1, cv::CAP_V4L);
				} else {
					gCapR.open(camSourceR, cv::CAP_ANY);
				}
			}
		}

		if (camSourceL.length() != 0 && ! gCapL.isOpened()) {
			log("CapL: Couldn't open device");
			if (gCapR.isOpened()) {
				gCapR.release();
			}
			return false;
		}
		if (camSourceR.length() != 0 && ! gCapR.isOpened()) {
			log("CapR: Couldn't open device");
			if (gCapL.isOpened()) {
				gCapL.release();
			}
			return false;
		}

		//
		if (camSourceL.length() == 0 && optsRt.outputCams == fcapconstants::OutputCamsEn::CAM_L) {
			optsRt.outputCams = fcapconstants::OutputCamsEn::CAM_R;
			fcapshared::Shared::setRtOpts_outputCams(optsRt.outputCams);
		}

		//
		///
		pCapInfo = (camSourceL.length() != 0 ? &gCapL : &gCapR);
		///
		/**int inpCodecTypeInt = static_cast<int>(pCapInfo->get(cv::CAP_PROP_FOURCC));
		char inpCodecTypeCh[] = {
				(char)(inpCodecTypeInt & 0XFF),
				(char)((inpCodecTypeInt & 0XFF00) >> 8),
				(char)((inpCodecTypeInt & 0XFF0000) >> 16),
				(char)((inpCodecTypeInt & 0XFF000000) >> 24),
				0
			};
		log("Input Codec: " + std::to_string(inpCodecTypeInt) + " / '" + inpCodecTypeCh + "'");**/
		///
		cv::Size inpVideoSz = cv::Size(
				(int)pCapInfo->get(cv::CAP_PROP_FRAME_WIDTH),
				(int)pCapInfo->get(cv::CAP_PROP_FRAME_HEIGHT)
			);
		log("Input Framesize: " + std::to_string(inpVideoSz.width) + "x" + std::to_string(inpVideoSz.height));
		///
		int inpFps = pCapInfo->get(cv::CAP_PROP_FPS);
		log("Input FPS: " + std::to_string(inpFps) +
				(gStaticOptionsStc.camSourceType != fcapconstants::CamSourceEn::GSTREAMER ?
					" (expecting " + std::to_string(optsRt.cameraFps) + ")" : ""));

		return true;
	}

	void FrameProducer::runX1(void) {
		const uint32_t _MAX_EMPTY_FRAMES = 3;

		try {
			uint32_t frameNr = 0;
			uint32_t emptyFrameCnt = 0;
			bool needToStop = false;
			bool haveClients = false;
			uint32_t toHaveClients = 1;
			uint32_t toNeedToStop = 100;
			uint32_t toOpts = 10;
			fcapshared::RuntimeOptionsStc optsRt = fcapshared::Shared::getRuntimeOptions();
			uint32_t toRestartCamStreams = optsRt.cameraFps;
			bool grabOkL;
			bool grabOkR;
			cv::Mat frameL = cv::Mat();  // Mat is a 'n-dimensional dense array class'
			cv::Mat frameR = cv::Mat();
			cv::Mat *pFrameL = nullptr;
			cv::Mat *pFrameR = nullptr;
			fcapconstants::OutputCamsEn lastOutputCams = optsRt.outputCams;

			switch (optsRt.outputCams) {
				case fcapconstants::OutputCamsEn::CAM_L:
					pFrameL = &frameL;
					break;
				case fcapconstants::OutputCamsEn::CAM_R:
					pFrameR = &frameR;
					break;
				default:
					pFrameL = &frameL;
					pFrameR = &frameR;
			}

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
					toNeedToStop = optsRt.cameraFps;  // check every FPS * 100ms
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
					optsRt = fcapshared::Shared::getRuntimeOptions();
					//
					if (lastOutputCams != optsRt.outputCams) {
						switch (optsRt.outputCams) {
							case fcapconstants::OutputCamsEn::CAM_L:
								pFrameL = &frameL;
								pFrameR = nullptr;
								break;
							case fcapconstants::OutputCamsEn::CAM_R:
								pFrameL = nullptr;
								pFrameR = &frameR;
								break;
							default:
								pFrameL = &frameL;
								pFrameR = &frameR;
						}
						//
						FrameConsumer::gFrameQueueInp.flushQueue();
						//
						lastOutputCams = optsRt.outputCams;
					}
					//
					toOpts = (optsRt.cameraFps > 5 ? optsRt.cameraFps / 5 : 1);
				}

				// grab frames in a synchronized manner
				///
				if (optsRt.outputCams != fcapconstants::OutputCamsEn::CAM_R) {
					/**log("grab L");**/
					grabOkL = gCapL.grab();
				} else {
					grabOkL = true;
				}
				if (optsRt.outputCams != fcapconstants::OutputCamsEn::CAM_L) {
					/**log("grab R");**/
					grabOkR = gCapR.grab();
				} else {
					grabOkR = true;
				}
				if (! (grabOkL && grabOkR)) {
					if (! grabOkL) {
						log("CapL: empty frame");
					}
					if (! grabOkR) {
						log("CapR: empty frame");
					}
					if (++emptyFrameCnt > _MAX_EMPTY_FRAMES) {
						log("aborting");
						break;
					}
					//
					openStreams();
					//
					std::this_thread::sleep_for(std::chrono::milliseconds(250));
					continue;
				}
				emptyFrameCnt = 0;
				///
				if (optsRt.outputCams != fcapconstants::OutputCamsEn::CAM_R) {
					/**log("retrieve L");**/
					gCapL.retrieve(frameL);
				}
				if (optsRt.outputCams != fcapconstants::OutputCamsEn::CAM_L) {
					/**log("retrieve R");**/
					gCapR.retrieve(frameR);
				}
				++frameNr;
				/**log("retrieve done");**/

				// store frames
				FrameConsumer::gFrameQueueInp.appendFramesToQueue(pFrameL, pFrameR);

				// restart camera streams if that has been requested
				if (gStaticOptionsStc.enableAdaptFps &&
						gStaticOptionsStc.camSourceType == fcapconstants::CamSourceEn::GSTREAMER &&
						--toRestartCamStreams == 0) {
					if (getFlagRestartCamStreams()) {
						openStreams();
					}
					optsRt = fcapshared::Shared::getRuntimeOptions();
					toRestartCamStreams = optsRt.cameraFps;
				}

				//
				std::this_thread::sleep_for(std::chrono::milliseconds((uint32_t)((1.0 / (float)(optsRt.cameraFps * 5)) * 1000.0)));

				//
				/*if (frameNr == optsRt.cameraFps * 5) {
					log("Setting STOP flag...");
					fcapshared::Shared::setFlagNeedToStop();
					//
					break;
				}*/
			}

			//
			/**auto timeEnd = std::chrono::steady_clock::now();
			const std::string pipeFormat1Str = pipe_format_en_to_str(fcapconstants::GSTREAMER_PIPE_FMT1);
			const std::string pipeFormat2Str = pipe_format_en_to_str(fcapconstants::GSTREAMER_PIPE_FMT2);
			log("PFMT(" + pipeFormat1Str + "/" + pipeFormat2Str + "): Elapsed time: " +
					std::to_string(std::chrono::duration_cast<std::chrono::milliseconds>(timeEnd - timeStart).count()) +
					" ms");**/
		} catch (std::exception &err) {
			log("ERROR: " + std::string(err.what()));
		}

		//
		log("Releasing cams...");
		std::this_thread::sleep_for(std::chrono::milliseconds(250));
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
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
			needToWait = ! (FrameConsumer::gFrameQueueInp.isQueueEmpty() &&
					FrameConsumer::gFrameQueueInp.isQueueEmpty());
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
