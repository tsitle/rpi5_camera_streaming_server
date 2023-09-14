#include <stdio.h>
#include <chrono>

#include "../shared.hpp"
#include "../settings.hpp"
#include "../cfgfile.hpp"
#include "frame_consumer.hpp"
#include "frame_producer.hpp"

using namespace std::chrono_literals;

namespace frame {

	FrameQueueRaw FrameConsumer::gFrameQueueInpL;
	FrameQueueRaw FrameConsumer::gFrameQueueInpR;

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
		gCompressionParams.push_back(fcapsettings::SETT_JPEG_QUALITY);
	}

	FrameConsumer::~FrameConsumer() {
	}

	// -----------------------------------------------------------------------------
	// -----------------------------------------------------------------------------

	void FrameConsumer::_startThread_internal(
			http::CbGetRunningHandlersCount cbGetRunningHandlersCount,
			http::CbBroadcastFrameToStreamingClients cbBroadcastFrameToStreamingClients) {
		if (! fcapsettings::SETT_OPEN_CAM_STREAMS) {
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
		cv::Mat frameL(gStaticOptionsStc.resolutionOutput, CV_8UC3);
		cv::Mat frameR(gStaticOptionsStc.resolutionOutput, CV_8UC3);
		cv::Mat blendedImg;
		cv::Mat *pFrameOut = NULL;
		uint32_t frameNr = 0;
		char strBufPath[512];
		char strBufFn[1024];
		bool haveFrameL = false;
		bool haveFrameR = false;
		bool haveFrames = false;
		bool needToStop = false;
		bool willDiscard = false;
		uint32_t toNeedToStop = 100;
		uint32_t toFrameL = 100;

		// wait for camera streams to be open
		needToStop = ! FrameProducer::waitForCamStreams();
		if (needToStop) {
			log("camera streams not open. aborting.");
			fcapshared::Shared::setFlagNeedToStop();
			return;
		}

		//
		/**log("STARTED");**/
		snprintf(strBufPath, sizeof(strBufPath), "%s", gStaticOptionsStc.pngOutputPath.c_str());
		try {
			uint32_t toOpts = 10;
			fcapshared::RuntimeOptionsStc opts = fcapshared::Shared::getRuntimeOptions();

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
					opts = fcapshared::Shared::getRuntimeOptions();
					//
					toOpts = 10;
				}

				// read frames from queues
				if (opts.outputCams == fcapconstants::OutputCamsEn::CAM_L) {
					/**log("get L");**/
					haveFrames = gFrameQueueInpL.getFrameFromQueue(frameL);
				} else if (opts.outputCams == fcapconstants::OutputCamsEn::CAM_R) {
					/**log("get R");**/
					haveFrames = gFrameQueueInpR.getFrameFromQueue(frameR);
				} else {
					/**log("get B R");**/
					haveFrameR = gFrameQueueInpR.getFrameFromQueue(frameR);
					if (! haveFrameR) {
						std::this_thread::sleep_for(std::chrono::milliseconds(10));
						continue;
					}
					/**log("get B L");**/
					haveFrameL = false;
					while (! haveFrameL && toFrameL != 0) {
						haveFrameL = gFrameQueueInpL.getFrameFromQueue(frameL);
						if (! haveFrameL) {
							--toFrameL;
							std::this_thread::sleep_for(std::chrono::milliseconds((uint32_t)((1.0 / (float)(gStaticOptionsStc.cameraFps * 5)) * 1000.0)));
						}
					}
					toFrameL = 100;
					if (! haveFrameL) {
						/**log("get B L giving up");**/
						continue;
					}
					haveFrames = true;
				}
				/**log("get done");**/

				// handle frames
				if (haveFrames && opts.outputCams == fcapconstants::OutputCamsEn::CAM_BOTH) {
					if (frameL.size() != frameR.size()) {
						haveFrames = false;
					}
				}
				if (haveFrames) {
					++frameNr;
					willDiscard = true;
					//
					if (opts.outputCams != fcapconstants::OutputCamsEn::CAM_R && gStaticOptionsStc.outputPngs) {
						snprintf(strBufFn, sizeof(strBufFn), "%s/testout-%03d-L.png", strBufPath, frameNr);
						cv::imwrite(std::string(strBufFn), frameL);
						/**log("CapL: wrote frame to '" << strBufFn << "'");**/
					}
					if (opts.outputCams != fcapconstants::OutputCamsEn::CAM_L && gStaticOptionsStc.outputPngs) {
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
						if (gStaticOptionsStc.outputPngs) {
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
				} else {
					std::this_thread::sleep_for(std::chrono::milliseconds(50));
				}
			}
		} catch (std::exception& err) {
			log("ERROR: " + std::string(err.what()));
		}

		//
		log("dropped fames inpL=" + std::to_string(gFrameQueueInpL.getDroppedFramesCount()));
		log("dropped fames inpR=" + std::to_string(gFrameQueueInpR.getDroppedFramesCount()));

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
