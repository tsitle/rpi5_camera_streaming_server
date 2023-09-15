#include <stdio.h>
#include <thread>

#include "../settings.hpp"
#include "frame_consumer.hpp"

using namespace std::chrono_literals;

namespace frame {

	FrameProcessor::FrameProcessor() :
			gPOptsRt(NULL) {
		gStaticOptionsStc = fcapcfgfile::CfgFile::getStaticOptions();

		//
		gSubProcsL.cal.setCamId(gStaticOptionsStc.camL);
		gSubProcsR.cal.setCamId(gStaticOptionsStc.camR);

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

	void FrameProcessor::setRuntimeOptionsPnt(fcapshared::RuntimeOptionsStc *pOptsRt) {
		gPOptsRt = pOptsRt;
		updateSubProcsSettings();
	}

	void FrameProcessor::processFrame(
				fcapconstants::OutputCamsEn outputCams,
				cv::Mat *pFrameL,
				cv::Mat *pFrameR,
				cv::Mat **ppFrameOut) {
		// do the actual processing
		if (! gDisableProcessing && pFrameL != NULL) {
			procDefaults(gSubProcsL, *pFrameL);
		}
		if (! gDisableProcessing && pFrameR != NULL) {
			procDefaults(gSubProcsR, *pFrameR);
		}

		//
		const std::string* pCamDesc = NULL;
		if (outputCams == fcapconstants::OutputCamsEn::CAM_L) {
			*ppFrameOut = pFrameL;
			if (! gDisableProcessing) {
				pCamDesc = &TEXT_CAM_SUFFIX_L;
			}
		} else if (outputCams == fcapconstants::OutputCamsEn::CAM_R) {
			*ppFrameOut = pFrameR;
			if (! gDisableProcessing) {
				pCamDesc = &TEXT_CAM_SUFFIX_R;
			}
		} else if (outputCams == fcapconstants::OutputCamsEn::CAM_BOTH) {
			if (! gDisableProcessing) {
				pCamDesc = &TEXT_CAM_SUFFIX_BOTH;
			}
			cv::addWeighted(*pFrameL, /*alpha:*/0.5, *pFrameR, /*beta:*/0.5, /*gamma:*/0, **ppFrameOut, -1);
		}

		// add text overlay
		if (! gDisableProcessing) {
			procAddTextOverlay(**ppFrameOut, *pCamDesc, outputCams != fcapconstants::OutputCamsEn::CAM_BOTH);
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

	void FrameProcessor::updateSubProcsSettings() {
		_updateSubProcsSettings_stc(gSubProcsL);
		_updateSubProcsSettings_stc(gSubProcsR);
	}

	void FrameProcessor::_updateSubProcsSettings_stc(SubProcsStc &subProcsStc) {
		subProcsStc.bnc.setBrightness(gPOptsRt->procBncAdjBrightness);
		subProcsStc.bnc.setContrast(gPOptsRt->procBncAdjContrast);
		//
		subProcsStc.cal.setShowCalibChessboardPoints(gPOptsRt->procCalShowCalibChessboardPoints);
	}

	void FrameProcessor::procDefaults(SubProcsStc &subProcsStc, cv::Mat &frame) {
		// adjust brightness and contrast
		subProcsStc.bnc.processFrame(frame);
		// calibrate camera
		subProcsStc.cal.processFrame(frame);
		if (! subProcsStc.cal.getIsCalibrated()) {
			return;
		}
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
