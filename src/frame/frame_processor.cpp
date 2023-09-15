#include <stdio.h>
#include <thread>

#include "../settings.hpp"
#include "frame_consumer.hpp"

using namespace std::chrono_literals;

namespace frame {

	const std::string TEXT_CAM_TXT_PREFIX = "CAM ";
	const std::string TEXT_CAM_TXT_SUFFIX_L = "L";
	const std::string TEXT_CAM_TXT_SUFFIX_R = "R";
	const std::string TEXT_CAM_TXT_SUFFIX_BOTH = "L+R";
	const cv::Point TEXT_CAM_COORD = cv::Point(5, 5);
	const cv::Scalar TEXT_CAM_COLOR = cv::Scalar(80.0, 80.0, 80.0);

	// -----------------------------------------------------------------------------
	// -----------------------------------------------------------------------------

	FrameProcessor::FrameProcessor() :
			gPOptsRt(NULL),
			gLastOutputCamsInt(-1) {
		gStaticOptionsStc = fcapcfgfile::CfgFile::getStaticOptions();

		//
		gSubProcsL.cal.setCamId(gStaticOptionsStc.camL);
		gSubProcsR.cal.setCamId(gStaticOptionsStc.camR);

		//
		gDisableProcessing = false;

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
				pCamDesc = &TEXT_CAM_TXT_SUFFIX_L;
			}
		} else if (outputCams == fcapconstants::OutputCamsEn::CAM_R) {
			*ppFrameOut = pFrameR;
			if (! gDisableProcessing) {
				pCamDesc = &TEXT_CAM_TXT_SUFFIX_R;
			}
		} else if (outputCams == fcapconstants::OutputCamsEn::CAM_BOTH) {
			if (! gDisableProcessing) {
				pCamDesc = &TEXT_CAM_TXT_SUFFIX_BOTH;
			}
			cv::addWeighted(*pFrameL, /*alpha:*/0.5, *pFrameR, /*beta:*/0.5, /*gamma:*/0, **ppFrameOut, -1);
		}

		// add text overlay
		if (! gDisableProcessing) {
			procAddTextOverlay(**ppFrameOut, *pCamDesc, outputCams);
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

	void FrameProcessor::procAddTextOverlay(
			cv::Mat &frameOut, const std::string &camDesc, const fcapconstants::OutputCamsEn outputCams) {
		if (gLastOutputCamsInt == -1 || gLastOutputCamsInt != (int)outputCams) {
			gOtherSubProcTextCams.setText(TEXT_CAM_TXT_PREFIX + camDesc, TEXT_CAM_COORD, TEXT_CAM_COLOR);
			gLastOutputCamsInt = (int)outputCams;
		}
		gOtherSubProcTextCams.processFrame(frameOut);
	}

}  // namespace frame
