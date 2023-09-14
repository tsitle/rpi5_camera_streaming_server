#include <stdio.h>
#include <thread>

#include "../settings.hpp"
#include "frame_consumer.hpp"

using namespace std::chrono_literals;

namespace frame {

	FrameProcessor::FrameProcessor() {
		gStaticOptionsStc = fcapcfgfile::CfgFile::getStaticOptions();
		//
		gDisableProcessing = false;
		//
		///
		gTextOverlayPropsStc.textFontScale = 1.0;
		gTextOverlayPropsStc.textThickness = 2;
		gTextOverlayPropsStc.textFontId = cv::FONT_HERSHEY_SIMPLEX;
		gTextOverlayPropsStc.textPrefix = TEXT_CAM_PREFIX;
		///
		cv::Size tmpSzOne = getTextSize(gTextOverlayPropsStc.textPrefix + TEXT_CAM_SUFFIX_L);
		cv::Size tmpSzTwo = getTextSize(gTextOverlayPropsStc.textPrefix + TEXT_CAM_SUFFIX_BOTH);
		///
		const uint8_t _BORDER = 5;
		gTextOverlayPropsStc.rectStartPoint = cv::Point(5, 5);
		gTextOverlayPropsStc.textCoordinates = cv::Point(
				gTextOverlayPropsStc.rectStartPoint.x + _BORDER,
				gTextOverlayPropsStc.rectStartPoint.y + _BORDER + tmpSzOne.height
			);
		gTextOverlayPropsStc.textColor = cv::Scalar(80.0, 80.0, 80.0);
		///
		gTextOverlayPropsStc.rectEndPointOne = cv::Point(
				gTextOverlayPropsStc.rectStartPoint.x + _BORDER + tmpSzOne.width + _BORDER,
				gTextOverlayPropsStc.rectStartPoint.x + _BORDER + tmpSzOne.height + _BORDER
			);
		gTextOverlayPropsStc.rectEndPointTwo = cv::Point(
				gTextOverlayPropsStc.rectStartPoint.x + _BORDER + tmpSzTwo.width + _BORDER,
				gTextOverlayPropsStc.rectStartPoint.x + _BORDER + tmpSzOne.height + _BORDER
			);
		gTextOverlayPropsStc.rectColor = cv::Scalar(255.0, 255.0, 255.0);
		gTextOverlayPropsStc.rectThickness = cv::FILLED;
		//
		log("Output Framesize: " +
				std::to_string(gStaticOptionsStc.resolutionOutput.width) +
				"x" +
				std::to_string(gStaticOptionsStc.resolutionOutput.height));
	}

	FrameProcessor::~FrameProcessor() {
	}

	void FrameProcessor::processFrame(cv::Mat *pFrameL, cv::Mat *pFrameR, cv::Mat **ppFrameOut) {
		if (ppFrameOut == NULL) {
			return;
		}
		const std::string* pCamDesc = NULL;
		bool isOneCam = true;
		if (pFrameL != NULL && pFrameR == NULL) {
			*ppFrameOut = pFrameL;
			if (! gDisableProcessing) {
				pCamDesc = &TEXT_CAM_SUFFIX_L;
			}
		} else if (pFrameL == NULL && pFrameR != NULL) {
			*ppFrameOut = pFrameR;
			if (! gDisableProcessing) {
				pCamDesc = &TEXT_CAM_SUFFIX_R;
			}
		} else if (pFrameL != NULL && pFrameR != NULL) {
			if (*ppFrameOut == NULL) {
				return;
			}
			if (! gDisableProcessing) {
				pCamDesc = &TEXT_CAM_SUFFIX_BOTH;
				isOneCam = false;
			}
			cv::addWeighted(*pFrameL, /*alpha:*/0.5, *pFrameR, /*beta:*/0.5, /*gamma:*/0, **ppFrameOut, -1);
		}

		// add text overlay
		if (! gDisableProcessing) {
			procAddTextOverlay(**ppFrameOut, *pCamDesc, isOneCam);
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

	void FrameProcessor::procAddTextOverlay(cv::Mat &frameOut, const std::string &camDesc, const bool isOneCam) {
		cv::rectangle(
				frameOut,
				gTextOverlayPropsStc.rectStartPoint,
				isOneCam ? gTextOverlayPropsStc.rectEndPointOne : gTextOverlayPropsStc.rectEndPointTwo,
				gTextOverlayPropsStc.rectColor,
				gTextOverlayPropsStc.rectThickness
			);
		cv::putText(
				frameOut,
				gTextOverlayPropsStc.textPrefix + camDesc,
				gTextOverlayPropsStc.textCoordinates,
				gTextOverlayPropsStc.textFontId,
				gTextOverlayPropsStc.textFontScale,
				gTextOverlayPropsStc.textColor,
				gTextOverlayPropsStc.textThickness,
				cv::LINE_AA,
				false
			);
	}

	cv::Size FrameProcessor::getTextSize(const std::string &text) {
		int baseline = 0;
		cv::Size resSz = cv::getTextSize(
				text,
				gTextOverlayPropsStc.textFontId,
				gTextOverlayPropsStc.textFontScale,
				gTextOverlayPropsStc.textThickness,
				&baseline
			);
		baseline += gTextOverlayPropsStc.textThickness;
		return resSz + cv::Size(0, baseline) - cv::Size(0, 11);
	}

}  // namespace frame
