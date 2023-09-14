#include <stdio.h>
#include <thread>

#include "../settings.hpp"
#include "frame_consumer.hpp"

using namespace std::chrono_literals;

namespace frame {

	FrameProcessor::FrameProcessor() {
		gStaticOptionsStc = fcapcfgfile::CfgFile::getStaticOptions();
		//
		log("Output Framesize: " +
				std::to_string(gStaticOptionsStc.resolutionOutput.width) +
				"x" +
				std::to_string(gStaticOptionsStc.resolutionOutput.height));
	}

	FrameProcessor::~FrameProcessor() {
	}

	void FrameProcessor::processFrame(cv::Mat *pFrameL, cv::Mat *pFrameR, cv::Mat **ppFrameOut) {
		if (pFrameL != NULL && pFrameR == NULL) {
			*ppFrameOut = pFrameL;
		} else if (pFrameL == NULL && pFrameR != NULL) {
			*ppFrameOut = pFrameR;
		} else if (pFrameL != NULL && pFrameR != NULL) {
			cv::addWeighted(*pFrameL, /*alpha:*/0.5, *pFrameR, /*beta:*/0.5, /*gamma:*/0, **ppFrameOut, -1);
		}

		// resize frame
		bool needToResizeFrame = (
				(*ppFrameOut)->cols != gStaticOptionsStc.resolutionOutput.width ||
				(*ppFrameOut)->rows != gStaticOptionsStc.resolutionOutput.height
			);
		if (needToResizeFrame) {
			log("resizing image from " +
					std::to_string((*ppFrameOut)->cols) + "x" + std::to_string((*ppFrameOut)->rows) +
					" to " +
					std::to_string(gStaticOptionsStc.resolutionOutput.width) + "x" + std::to_string(gStaticOptionsStc.resolutionOutput.height) +
					" ...");
			cv::resize(**ppFrameOut, **ppFrameOut, gStaticOptionsStc.resolutionOutput, 0.0, 0.0, cv::INTER_LINEAR);
		}
	}

	// -----------------------------------------------------------------------------
	// -----------------------------------------------------------------------------

	void FrameProcessor::log(const std::string &message) {
		std::cout << "FPROC: " << message << std::endl;
	}

}  // namespace frame
