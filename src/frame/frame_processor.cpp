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

	const std::string TEXT_CAL_TXT_ISCAL = "CAL";
	const std::string TEXT_CAL_TXT_UNCAL = "UNCAL";
	const cv::Point TEXT_CAL_COORD = cv::Point(5, 0);
	const cv::Scalar TEXT_CAL_COLOR_ISCAL = cv::Scalar(20.0, 200.0, 20.0);
	const cv::Scalar TEXT_CAL_COLOR_UNCAL = cv::Scalar(200.0, 20.0, 20.0);

	// -----------------------------------------------------------------------------
	// -----------------------------------------------------------------------------

	FrameProcessor::FrameProcessor() :
			gPOptsRt(NULL),
			gLastOutputCamsInt(-1),
			gLastIsCalibratedInt(-1) {
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
				cv::Mat *pFrameOut) {
		// do the actual processing
		if (! gDisableProcessing) {
			if (pFrameL != NULL) {
				procDefaults(gSubProcsL, *pFrameL);
			}
			if (pFrameR != NULL) {
				procDefaults(gSubProcsR, *pFrameR);
			}
		}

		//
		const std::string* pCamDesc = NULL;
		if (outputCams == fcapconstants::OutputCamsEn::CAM_L) {
			if (! gDisableProcessing) {
				pCamDesc = &TEXT_CAM_TXT_SUFFIX_L;
			}
		} else if (outputCams == fcapconstants::OutputCamsEn::CAM_R) {
			if (! gDisableProcessing) {
				pCamDesc = &TEXT_CAM_TXT_SUFFIX_R;
			}
		} else if (outputCams == fcapconstants::OutputCamsEn::CAM_BOTH) {
			if (! gDisableProcessing) {
				pCamDesc = &TEXT_CAM_TXT_SUFFIX_BOTH;
			}
			cv::addWeighted(*pFrameL, /*alpha:*/0.5, *pFrameR, /*beta:*/0.5, /*gamma:*/0, *pFrameOut, -1);
		}

		// add text overlay
		if (! gDisableProcessing) {
			procAddTextOverlayCams(*pFrameOut, *pCamDesc, outputCams);
		}

		// resize frame
		bool needToResizeFrame = (
				pFrameOut->cols != gStaticOptionsStc.resolutionOutput.width ||
				pFrameOut->rows != gStaticOptionsStc.resolutionOutput.height
			);
		if (needToResizeFrame) {
			log("resizing image from " +
					std::to_string(pFrameOut->cols) + "x" + std::to_string(pFrameOut->rows) +
					" to " +
					std::to_string(gStaticOptionsStc.resolutionOutput.width) + "x" + std::to_string(gStaticOptionsStc.resolutionOutput.height) +
					" ...");
			cv::resize(*pFrameOut, *pFrameOut, gStaticOptionsStc.resolutionOutput, 0.0, 0.0, cv::INTER_LINEAR);
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
			procAddTextOverlayCal(frame, false);
			return;
		}
		procAddTextOverlayCal(frame, true);
	}

	void FrameProcessor::procAddTextOverlayCams(
			cv::Mat &frameOut, const std::string &camDesc, const fcapconstants::OutputCamsEn outputCams) {
		if (gLastOutputCamsInt == -1 || gLastOutputCamsInt != (int8_t)outputCams) {
			gOtherSubProcTextCams.setText(TEXT_CAM_TXT_PREFIX + camDesc, TEXT_CAM_COORD, TEXT_CAM_COLOR);
			gLastOutputCamsInt = (int)outputCams;
		}
		gOtherSubProcTextCams.processFrame(frameOut);
	}

	void FrameProcessor::procAddTextOverlayCal(cv::Mat &frameOut, const bool isCalibrated) {
		if (gLastOutputCamsInt == -1) {
			return;
		}
		if (gLastIsCalibratedInt == -1 || gLastIsCalibratedInt != (int8_t)isCalibrated) {
			gOtherSubProcTextCal.setText(
					isCalibrated ? TEXT_CAL_TXT_ISCAL : TEXT_CAL_TXT_UNCAL,
					TEXT_CAL_COORD + cv::Point(0, gOtherSubProcTextCams.getTextBottomY() + 5),
					isCalibrated ? TEXT_CAL_COLOR_ISCAL : TEXT_CAL_COLOR_UNCAL
				);
			gLastIsCalibratedInt = (int8_t)isCalibrated;
		}
		gOtherSubProcTextCal.processFrame(frameOut);
	}

}  // namespace frame
