#include <stdio.h>
#include <chrono>

#include "../shared.hpp"
#include "../settings.hpp"
#include "../cfgfile.hpp"
#include "frame_consumer.hpp"
#include "frame_producer.hpp"

using namespace std::chrono_literals;

namespace frame {

	FrameQueueRawInput FrameConsumer::gFrameQueueInp;

	// -----------------------------------------------------------------------------
	// -----------------------------------------------------------------------------

	std::thread FrameConsumer::startThread(
			http::CbGetRunningHandlersCount cbGetRunningHandlersCount,
			http::CbBroadcastFrameToStreamingClients cbBroadcastFrameToStreamingClients) {
		std::thread threadObj(
				_startThread_internal,
				cbGetRunningHandlersCount,
				cbBroadcastFrameToStreamingClients
			);
		return threadObj;
	}

	// -----------------------------------------------------------------------------

	FrameConsumer::FrameConsumer(
			http::CbGetRunningHandlersCount cbGetRunningHandlersCount,
			http::CbBroadcastFrameToStreamingClients cbBroadcastFrameToStreamingClients) :
				gFrameProcessor(),
				gCbGetRunningHandlersCount(cbGetRunningHandlersCount),
				gCbBroadcastFrameToStreamingClients(cbBroadcastFrameToStreamingClients) {
		gStaticOptionsStc = fcapcfgfile::CfgFile::getStaticOptions();
		//
		gCompressionParams.push_back(cv::IMWRITE_JPEG_QUALITY);
		gCompressionParams.push_back(fcapsettings::JPEG_QUALITY);
	}

	FrameConsumer::~FrameConsumer() {
	}

	// -----------------------------------------------------------------------------
	// -----------------------------------------------------------------------------

	void FrameConsumer::_startThread_internal(
			http::CbGetRunningHandlersCount cbGetRunningHandlersCount,
			http::CbBroadcastFrameToStreamingClients cbBroadcastFrameToStreamingClients) {
		if (! fcapsettings::DBG_OPEN_CAM_STREAMS) {
			return;
		}

		FrameConsumer frameConsObj(cbGetRunningHandlersCount, cbBroadcastFrameToStreamingClients);
		frameConsObj.runX2();
	}

	// -----------------------------------------------------------------------------

	void FrameConsumer::log(const std::string &message) {
		std::cout << "FCONS: " << message << std::endl;
	}

	void FrameConsumer::runX2() {
		cv::Mat frameL(gStaticOptionsStc.resolutionInputStream, CV_8UC3);
		cv::Mat frameR(gStaticOptionsStc.resolutionInputStream, CV_8UC3);
		uint32_t frameNr = 0;
		char strBufPath[512];
		char strBufFn[1024];
		bool haveFrameL = false;
		bool haveFrameR = false;
		bool haveFrames = false;
		bool needToStop = false;
		bool haveExc = false;
		bool isFirstFrameDropTest = true;
		uint32_t toNeedToStop = 100;
		fcapshared::RuntimeOptionsStc optsRt = fcapshared::Shared::getRuntimeOptions();
		fcapconstants::OutputCamsEn optsLastOutputCams = optsRt.outputCams;
		uint32_t toCheckFps = optsRt.cameraFps * 2;

		// wait for camera streams to be open
		needToStop = ! FrameProducer::waitForCamStreams();
		if (needToStop) {
			log("camera streams not open. aborting.");
			fcapshared::Shared::setFlagNeedToStop();
			return;
		}

		//
		gFrameProcessor.setRuntimeOptionsPnt(&optsRt);

		//
		/**log("STARTED");**/
		snprintf(strBufPath, sizeof(strBufPath), "%s", gStaticOptionsStc.pngOutputPath.c_str());
		try {
			uint32_t toOpts = 10;
			bool reloadCamStreams = false;

			while (true) {
				// check if we need to stop
				if (--toNeedToStop == 0) {
					needToStop = fcapshared::Shared::getFlagNeedToStop();
					if (needToStop) {
						/**log("Stopping...");**/
						break;
					}
					//
					toNeedToStop = gStaticOptionsStc.cameraFps;  // check every FPS * 50ms
				}

				// update runtime options
				if (--toOpts == 0) {
					optsRt = fcapshared::Shared::getRuntimeOptions();
					gFrameProcessor.setRuntimeOptionsPnt(&optsRt);
					//
					if (optsLastOutputCams != optsRt.outputCams) {
						isFirstFrameDropTest = true;
						optsLastOutputCams = optsRt.outputCams;
					}
					//
					toOpts = 10;
				}

				// read frames from queues
				if (optsRt.outputCams == fcapconstants::OutputCamsEn::CAM_L) {
					/**log("get L");**/
					haveFrames = gFrameQueueInp.getFramesFromQueue(&frameL, nullptr);
				} else if (optsRt.outputCams == fcapconstants::OutputCamsEn::CAM_R) {
					/**log("get R");**/
					haveFrames = gFrameQueueInp.getFramesFromQueue(nullptr, &frameR);
				} else {
					/**log("get BOTH");**/
					haveFrameR = gFrameQueueInp.getFramesFromQueue(&frameL, &frameR);
					if (! haveFrameR || frameR.empty()) {
						std::this_thread::sleep_for(std::chrono::milliseconds(5));
						continue;
					}
					haveFrameL = (! frameL.empty());
					if (! haveFrameL) {
						continue;
					}
					haveFrames = true;
				}
				/**log("get done");**/

				// handle frames
				if (haveFrames && optsRt.outputCams == fcapconstants::OutputCamsEn::CAM_BOTH) {
					if (frameL.size() != frameR.size()) {
						haveFrames = false;
					}
				}
				if (haveFrames) {
					cv::Mat frameBlended;
					cv::Mat *pFrameOut;

					++frameNr;

					// write input frames to PNG files
					if (gStaticOptionsStc.outputPngs) {
						if (optsRt.outputCams != fcapconstants::OutputCamsEn::CAM_R) {
							snprintf(strBufFn, sizeof(strBufFn), "%s/testout-%03d-L.png", strBufPath, frameNr);
							cv::imwrite(std::string(strBufFn), frameL);
							/**log("CapL: wrote frame to '" << strBufFn << "'");**/
						}
						if (optsRt.outputCams != fcapconstants::OutputCamsEn::CAM_L) {
							snprintf(strBufFn, sizeof(strBufFn), "%s/testout-%03d-R.png", strBufPath, frameNr);
							cv::imwrite(std::string(strBufFn), frameR);
							/**log("CapR: wrote frame to '" << strBufFn << "'");**/
						}
					}

					// process frames
					if (optsRt.outputCams == fcapconstants::OutputCamsEn::CAM_L) {
						pFrameOut = &frameL;
						gFrameProcessor.processFrame(&frameL, NULL, pFrameOut, frameNr);
					} else if (optsRt.outputCams == fcapconstants::OutputCamsEn::CAM_R) {
						pFrameOut = &frameR;
						gFrameProcessor.processFrame(NULL, &frameR, pFrameOut, frameNr);
					} else {
						pFrameOut = &frameBlended;
						gFrameProcessor.processFrame(&frameL, &frameR, pFrameOut, frameNr);
					}

					// output the resulting frame
					///
					if (gStaticOptionsStc.outputPngs) {
						snprintf(strBufFn, sizeof(strBufFn), "%s/testout-%03d-OUT.png", strBufPath, frameNr);
						cv::imwrite(std::string(strBufFn), *pFrameOut);
						/**log("CapB: wrote resulting frame to '" << strBufFn << "'");**/
					}
					///
					outputFrameToQueue(*pFrameOut);
				} else {
					std::this_thread::sleep_for(std::chrono::milliseconds(10));
				}

				// check if we need to adjust the framerate
				if (gStaticOptionsStc.enableAdaptFps &&
						haveFrames &&
						gStaticOptionsStc.camSourceType == fcapconstants::CamSourceEn::GSTREAMER &&
						optsRt.cameraFps > 1 &&
						--toCheckFps == 0) {
					bool resetCounts = false;

					reloadCamStreams = false;
					if (gFrameQueueInp.getDroppedFramesCount() > 10) {
						log("dropped fames inp=" + std::to_string(gFrameQueueInp.getDroppedFramesCount()));
						reloadCamStreams = (! isFirstFrameDropTest);
						resetCounts = true;
					}
					if (reloadCamStreams) {
						uint32_t tmpDfX = gFrameQueueInp.getDroppedFramesCount();
						uint8_t lastCameraFps = fcapshared::Shared::getRuntimeOptions().cameraFps;
						optsRt.cameraFps = lastCameraFps - (
								tmpDfX > 100 && lastCameraFps > 10 ? 10 :
									(tmpDfX > 50 && lastCameraFps > 5 ? 5 :
										(tmpDfX > 30 && lastCameraFps > 3 ? 3 : 1))
							);
						log("Reducing FPS to " + std::to_string(optsRt.cameraFps) + "...");
						fcapshared::Shared::setRtOpts_cameraFps(optsRt.cameraFps);
						//
						FrameProducer::setFlagRestartCamStreams();
						// wait a little while for cam streams to have been reloaded
						std::this_thread::sleep_for(std::chrono::milliseconds(3000));
						//
						toCheckFps = optsRt.cameraFps * 5;
					} else {
						toCheckFps = optsRt.cameraFps * 2;
					}
					if (resetCounts) {
						isFirstFrameDropTest = false;
						gFrameQueueInp.resetDroppedFramesCount();
					}
				}
			}
		} catch (std::exception &err) {
			log("ERROR: " + std::string(err.what()));
			haveExc = true;
		}

		//
		if (haveExc) {
			fcapshared::Shared::setFlagNeedToStop();
		} /**else {
			log("dropped fames inpL=" + std::to_string(gFrameQueueInpL.getDroppedFramesCount()));
			log("dropped fames inpR=" + std::to_string(gFrameQueueInpR.getDroppedFramesCount()));
		}**/

		//
		log("ENDED");
	}

	void FrameConsumer::outputFrameToQueue(const cv::Mat &frame) {
		bool haveClients;

		//
		haveClients = (gCbGetRunningHandlersCount() > 0);
		if (! haveClients) {
			/**log("output no clients");**/
			return;
		}

		//
		std::vector<unsigned char> jpegFrame;
		cv::imencode(".jpg", frame, jpegFrame, gCompressionParams);
		/**log("put frame beg");**/
		gCbBroadcastFrameToStreamingClients(jpegFrame);
		/**log("put frame end");**/
	}

}  // namespace frame
