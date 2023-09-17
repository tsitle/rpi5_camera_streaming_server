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
		initSubProcs();

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

	void FrameProcessor::initSubProcs() {
		// other FrameSubProcs for the master output
		_initSubProcs_fspObj(fcapconstants::CamIdEn::CAM_0, fcapconstants::OutputCamsEn::CAM_BOTH, gOtherSubProcTextCams);
		_initSubProcs_fspObj(fcapconstants::CamIdEn::CAM_0, fcapconstants::OutputCamsEn::CAM_BOTH, gOtherSubProcTextCal);
		//
		_initSubProcs_stc(gStaticOptionsStc.camL, fcapconstants::OutputCamsEn::CAM_L, gSubProcsL);
		_initSubProcs_stc(gStaticOptionsStc.camR, fcapconstants::OutputCamsEn::CAM_R, gSubProcsR);
	}

	void FrameProcessor::_initSubProcs_stc(
			fcapconstants::CamIdEn camId, fcapconstants::OutputCamsEn outputCams, SubProcsStc &subProcsStc) {
		subProcsStc.camId = camId;
		subProcsStc.outputCams = outputCams;
		_initSubProcs_fspObj(camId, outputCams, subProcsStc.bnc);
		_initSubProcs_fspObj(camId, outputCams, subProcsStc.cal);
		_initSubProcs_fspObj(camId, outputCams, subProcsStc.pt);
	}

	void FrameProcessor::_initSubProcs_fspObj(fcapconstants::CamIdEn camId, fcapconstants::OutputCamsEn outputCams, framesubproc::FrameSubProcessor &fsp) {
		fsp.setCamIdAndOutputCams(camId, outputCams);
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
		//
		if (gPOptsRt->procPtChangedRectCorners[subProcsStc.camId]) {
			subProcsStc.pt.setRectCorners(gPOptsRt->procPtRectCorners[subProcsStc.camId]);
			fcapshared::Shared::setRuntimeOptions_procPtChangedRectCorners(subProcsStc.camId, false);
			gPOptsRt->procPtChangedRectCorners[subProcsStc.camId] = false;
		}
	}

	void FrameProcessor::procDefaults(SubProcsStc &subProcsStc, cv::Mat &frame) {
		bool tmpBool;

		// adjust brightness and contrast
		subProcsStc.bnc.processFrame(frame);
		// calibrate camera
		subProcsStc.cal.processFrame(frame);
		tmpBool = subProcsStc.cal.getIsCalibrated();
		if (tmpBool != gPOptsRt->procCalDone[subProcsStc.camId]) {
			fcapshared::Shared::setRuntimeOptions_procCalDone(subProcsStc.camId, tmpBool);
			gPOptsRt->procCalDone[subProcsStc.camId] = tmpBool;
		}
		if (! tmpBool) {
			// text overlay "UNCAL"
			procAddTextOverlayCal(frame, false);
			return;
		}
		// perspective transformation
		///
		tmpBool = ! subProcsStc.pt.getNeedRectCorners();
		if (tmpBool != gPOptsRt->procPtDone[subProcsStc.camId]) {
			fcapshared::Shared::setRuntimeOptions_procPtDone(subProcsStc.camId, tmpBool);
			gPOptsRt->procPtDone[subProcsStc.camId] = tmpBool;
			//
			std::vector<cv::Point> tmpCorners = subProcsStc.pt.getRectCorners();
			fcapshared::Shared::setRuntimeOptions_procPtRectCorners(subProcsStc.camId, tmpCorners);
			gPOptsRt->procPtRectCorners[subProcsStc.camId] = tmpCorners;
		}
		///
		subProcsStc.pt.processFrame(frame);
		// text overlay "CAL"
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
