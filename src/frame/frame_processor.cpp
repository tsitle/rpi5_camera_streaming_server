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
			gLastOverlCamsOutputCamsInt(-1),
			gLastOverlCalIsCalibratedInt(-1),
			gLastOverlCamsResolutionOutpW(-1),
			gLastOverlCalResolutionOutpW(-1) {
		gStaticOptionsStc = fcapcfgfile::CfgFile::getStaticOptions();

		//
		initSubProcs();

		//
		log("Input Stream Framesize: " +
				std::to_string(gStaticOptionsStc.resolutionInputStream.width) +
				"x" +
				std::to_string(gStaticOptionsStc.resolutionInputStream.height));
	}

	FrameProcessor::~FrameProcessor() {
	}

	void FrameProcessor::setRuntimeOptionsPnt(fcapshared::RuntimeOptionsStc *pOptsRt) {
		gPOptsRt = pOptsRt;
		updateSubProcsSettings();
	}

	void FrameProcessor::processFrame(cv::Mat *pFrameL, cv::Mat *pFrameR, cv::Mat *pFrameOut) {
		// check frame size
		if (pFrameL != NULL &&
				(pFrameL->size().width != gStaticOptionsStc.resolutionInputStream.width ||
					pFrameL->size().height != gStaticOptionsStc.resolutionInputStream.height)) {
			log("input frame size CAM_L doesn't match resolutionInputStream");
			return;
		}
		if (pFrameR != NULL &&
				(pFrameR->size().width != gStaticOptionsStc.resolutionInputStream.width ||
					pFrameR->size().height != gStaticOptionsStc.resolutionInputStream.height)) {
			log("input frame size CAM_R doesn't match resolutionInputStream");
			return;
		}

		// do the actual processing
		if (! fcapsettings::PROC_DISABLE_ALL_PROCESSING) {
			if (pFrameL != NULL) {
				procDefaults(gSubProcsL, *pFrameL);
			}
			if (pFrameR != NULL) {
				procDefaults(gSubProcsR, *pFrameR);
			}
			// adjust color channels
			if (gPOptsRt->outputCams == fcapconstants::OutputCamsEn::CAM_BOTH &&
					pFrameR->channels() != pFrameL->channels()) {
				int channL = pFrameL->channels();
				int channR = pFrameR->channels();
				if (channL > channR) {
					cv::cvtColor(*pFrameL, *pFrameL, cv::COLOR_BGR2GRAY);
				} else {
					cv::cvtColor(*pFrameR, *pFrameR, cv::COLOR_BGR2GRAY);
				}
			}
		}

		//
		const std::string* pCamDesc = NULL;
		if (gPOptsRt->outputCams == fcapconstants::OutputCamsEn::CAM_L) {
			pCamDesc = &TEXT_CAM_TXT_SUFFIX_L;
		} else if (gPOptsRt->outputCams == fcapconstants::OutputCamsEn::CAM_R) {
			pCamDesc = &TEXT_CAM_TXT_SUFFIX_R;
		} else if (gPOptsRt->outputCams == fcapconstants::OutputCamsEn::CAM_BOTH) {
			pCamDesc = &TEXT_CAM_TXT_SUFFIX_BOTH;
			cv::addWeighted(*pFrameL, /*alpha:*/0.5, *pFrameR, /*beta:*/0.5, /*gamma:*/0, *pFrameOut, -1);
		}

		// add text overlays
		if (! fcapsettings::PROC_DISABLE_ALL_PROCESSING) {
			// text overlay "CAM x"
			if (gStaticOptionsStc.procEnabled.overlCam) {
				procAddTextOverlayCams(*pFrameOut, *pCamDesc, gPOptsRt->outputCams);
			}
			// text overlay "CAL"
			if (gStaticOptionsStc.procEnabled.overlCal) {
				bool tmpBool;
				switch (gPOptsRt->outputCams) {
					case fcapconstants::OutputCamsEn::CAM_L:
						tmpBool = gPOptsRt->procCalDone[gStaticOptionsStc.camL];
						break;
					case fcapconstants::OutputCamsEn::CAM_R:
						tmpBool = gPOptsRt->procCalDone[gStaticOptionsStc.camR];
						break;
					default:
						tmpBool = (gPOptsRt->procCalDone[fcapconstants::CamIdEn::CAM_0] &&
								gPOptsRt->procCalDone[fcapconstants::CamIdEn::CAM_1]);
				}
				procAddTextOverlayCal(*pFrameOut, tmpBool);
			}
		}
	}

	// -----------------------------------------------------------------------------
	// -----------------------------------------------------------------------------

	void FrameProcessor::log(const std::string &message) {
		std::cout << "FPROC: " << message << std::endl;
	}

	void FrameProcessor::initSubProcs() {
		// other FrameSubProcs for the master output
		///
		_initSubProcs_fspObj(fcapconstants::CamIdEn::CAM_0, fcapconstants::OutputCamsEn::CAM_BOTH, gOtherSubProcRoi);
		if (gStaticOptionsStc.procEnabled.roi) {
			gOtherSubProcRoi.loadData();
			gRoiOutputSz = gOtherSubProcRoi.getOutputSz();
		} else {
			gRoiOutputSz = gStaticOptionsStc.resolutionInputStream;
		}
		///
		_initSubProcs_fspObj(fcapconstants::CamIdEn::CAM_0, fcapconstants::OutputCamsEn::CAM_BOTH, gOtherSubProcTextCams);
		_initSubProcs_fspObj(fcapconstants::CamIdEn::CAM_0, fcapconstants::OutputCamsEn::CAM_BOTH, gOtherSubProcTextCal);

		// per camera FrameSubProcs
		_initSubProcs_stc(gStaticOptionsStc.camL, fcapconstants::OutputCamsEn::CAM_L, gSubProcsL);
		_initSubProcs_stc(gStaticOptionsStc.camR, fcapconstants::OutputCamsEn::CAM_R, gSubProcsR);
	}

	void FrameProcessor::_initSubProcs_stc(
			fcapconstants::CamIdEn camId, fcapconstants::OutputCamsEn outputCams, SubProcsStc &subProcsStc) {
		subProcsStc.camId = camId;
		subProcsStc.outputCams = outputCams;
		//
		_initSubProcs_fspObj(camId, outputCams, subProcsStc.bnc);
		//
		_initSubProcs_fspObj(camId, outputCams, subProcsStc.cal);
		if (gStaticOptionsStc.procEnabled.cal) {
			subProcsStc.cal.loadData();
		}
		//
		subProcsStc.flip.setData(gStaticOptionsStc.flip[camId].hor, gStaticOptionsStc.flip[camId].ver);
		_initSubProcs_fspObj(camId, outputCams, subProcsStc.flip);
		//
		_initSubProcs_fspObj(camId, outputCams, subProcsStc.grid);
		//
		_initSubProcs_fspObj(camId, outputCams, subProcsStc.pt);
		if (gStaticOptionsStc.procEnabled.pt) {
			subProcsStc.pt.setRoiOutputSz(gRoiOutputSz);
			subProcsStc.pt.loadData();
		}
		//
		_initSubProcs_fspObj(camId, outputCams, subProcsStc.tr);
	}

	void FrameProcessor::_initSubProcs_fspObj(
			fcapconstants::CamIdEn camId,
			fcapconstants::OutputCamsEn outputCams,
			framesubproc::FrameSubProcessor &fsp) {
		fsp.setCamIdAndOutputCams(camId, outputCams);
		fsp.setInputFrameSize(gStaticOptionsStc.resolutionInputStream);
	}

	void FrameProcessor::updateSubProcsSettings() {
		if (gStaticOptionsStc.procEnabled.roi && gPOptsRt->procRoiChanged) {
			gOtherSubProcRoi.setData(gPOptsRt->procRoiSizePerc);
			gRoiOutputSz = gOtherSubProcRoi.getOutputSz();
			//
			fcapshared::Shared::setRtOpts_procRoiChanged(false);
			gPOptsRt->procRoiChanged = false;
		}
		//
		_updateSubProcsSettings_stc(gSubProcsL);
		_updateSubProcsSettings_stc(gSubProcsR);
	}

	void FrameProcessor::_updateSubProcsSettings_stc(SubProcsStc &subProcsStc) {
		if (gStaticOptionsStc.procEnabled.bnc && gPOptsRt->procBncChanged[subProcsStc.camId]) {
			subProcsStc.bnc.setBrightness(gPOptsRt->procBncAdjBrightness);
			subProcsStc.bnc.setContrast(gPOptsRt->procBncAdjContrast);
			//
			fcapshared::Shared::setRtOpts_procBncChanged(subProcsStc.camId, false);
			gPOptsRt->procBncChanged[subProcsStc.camId] = false;
		}
		//
		if (gStaticOptionsStc.procEnabled.cal && gPOptsRt->procCalChanged[subProcsStc.camId]) {
			subProcsStc.cal.setShowCalibChessboardPoints(gPOptsRt->procCalShowCalibChessboardPoints);
			//
			fcapshared::Shared::setRtOpts_procCalChanged(subProcsStc.camId, false);
			gPOptsRt->procCalChanged[subProcsStc.camId] = false;
		}
		//
		if (gStaticOptionsStc.procEnabled.pt && gPOptsRt->procPtChanged[subProcsStc.camId]) {
			subProcsStc.pt.setRectCorners(gPOptsRt->procPtRectCorners[subProcsStc.camId]);
			subProcsStc.pt.setRoiOutputSz(gRoiOutputSz);
			//
			fcapshared::Shared::setRtOpts_procPtChanged(subProcsStc.camId, false);
			gPOptsRt->procPtChanged[subProcsStc.camId] = false;
		}
		//
		if (gStaticOptionsStc.procEnabled.tr && gPOptsRt->procTrChanged[subProcsStc.camId]) {
			subProcsStc.tr.setDelta(gPOptsRt->procTrDelta[subProcsStc.camId].x, gPOptsRt->procTrDelta[subProcsStc.camId].y);
			//
			fcapshared::Shared::setRtOpts_procTrChanged(subProcsStc.camId, false);
			gPOptsRt->procTrChanged[subProcsStc.camId] = false;
		}
	}

	void FrameProcessor::procDefaults(SubProcsStc &subProcsStc, cv::Mat &frame) {
		bool tmpBool;
		bool skipPt = false;
		bool skipTr = false;

		// adjust brightness and contrast
		if (gStaticOptionsStc.procEnabled.bnc) {
			subProcsStc.bnc.processFrame(frame);
		}

		// calibrate camera
		if (gStaticOptionsStc.procEnabled.cal) {
			if (gPOptsRt->procCalDoReset[subProcsStc.camId]) {
				fcapshared::Shared::setRtOpts_procCalDoReset(subProcsStc.camId, false);
				gPOptsRt->procCalDoReset[subProcsStc.camId] = false;
				//
				fcapshared::Shared::setRtOpts_procCalDone(subProcsStc.camId, false);
				gPOptsRt->procCalDone[subProcsStc.camId] = false;
				//
				subProcsStc.cal.resetData();
				// also reset subsequent SubProcs' data
				///
				fcapshared::Shared::setRtOpts_procPtDoReset(subProcsStc.camId, true);
				gPOptsRt->procPtDoReset[subProcsStc.camId] = true;
				///
				fcapshared::Shared::setRtOpts_procTrDoReset(subProcsStc.camId, true);
				gPOptsRt->procTrDoReset[subProcsStc.camId] = true;
			}
			//
			if (gPOptsRt->procCalDoStart[subProcsStc.camId] || gPOptsRt->procCalDone[subProcsStc.camId]) {
				subProcsStc.cal.processFrame(frame);
			}
			//
			tmpBool = subProcsStc.cal.getIsCalibrated();
			if (tmpBool != gPOptsRt->procCalDone[subProcsStc.camId]) {
				fcapshared::Shared::setRtOpts_procCalDone(subProcsStc.camId, tmpBool);
				gPOptsRt->procCalDone[subProcsStc.camId] = tmpBool;
				//
				if (tmpBool && gPOptsRt->procCalDoStart[subProcsStc.camId]) {
					fcapshared::Shared::setRtOpts_procCalDoStart(subProcsStc.camId, false);
					gPOptsRt->procCalDoStart[subProcsStc.camId] = false;
				}
			}
			if (! tmpBool) {
				skipPt = true;
			}
		}

		// perspective transformation
		if (gStaticOptionsStc.procEnabled.pt) {
			if (gPOptsRt->procPtDoReset[subProcsStc.camId]) {
				fcapshared::Shared::setRtOpts_procPtDoReset(subProcsStc.camId, false);
				gPOptsRt->procPtDoReset[subProcsStc.camId] = false;
				//
				fcapshared::Shared::setRtOpts_procPtDone(subProcsStc.camId, false);
				gPOptsRt->procPtDone[subProcsStc.camId] = false;
				//
				fcapshared::Shared::setRtOpts_procPtRectCorners(subProcsStc.camId, std::vector<cv::Point>());
				gPOptsRt->procPtRectCorners[subProcsStc.camId] = std::vector<cv::Point>();
				//
				subProcsStc.pt.resetData();
				// also reset subsequent SubProcs' data
				///
				fcapshared::Shared::setRtOpts_procTrDoReset(subProcsStc.camId, true);
				gPOptsRt->procTrDoReset[subProcsStc.camId] = true;
			} else {
				tmpBool = ! subProcsStc.pt.getNeedRectCorners();
				if (tmpBool != gPOptsRt->procPtDone[subProcsStc.camId]) {
					fcapshared::Shared::setRtOpts_procPtDone(subProcsStc.camId, tmpBool);
					gPOptsRt->procPtDone[subProcsStc.camId] = tmpBool;
					//
					std::vector<cv::Point> tmpCorners = subProcsStc.pt.getRectCorners();
					fcapshared::Shared::setRtOpts_procPtRectCorners(subProcsStc.camId, tmpCorners);
					gPOptsRt->procPtRectCorners[subProcsStc.camId] = tmpCorners;
				}
			}
			//
			if (! skipPt) {
				subProcsStc.pt.processFrame(frame);
			}
			//
			if (subProcsStc.pt.getNeedRectCorners()) {
				skipTr = true;
			}
		}

		// flip
		if (gStaticOptionsStc.procEnabled.flip) {
			subProcsStc.flip.processFrame(frame);
		}

		// translation
		if (gStaticOptionsStc.procEnabled.tr) {
			if (gPOptsRt->procTrDoReset[subProcsStc.camId]) {
				fcapshared::Shared::setRtOpts_procTrDoReset(subProcsStc.camId, false);
				gPOptsRt->procTrDoReset[subProcsStc.camId] = false;
				//
				subProcsStc.tr.resetData();
			}
			//
			if (! skipTr) {
				subProcsStc.tr.processFrame(frame);
			}
			//
			cv::Point tmpPnt;
			subProcsStc.tr.getDelta(tmpPnt.x, tmpPnt.y);
			if (tmpPnt.x != gPOptsRt->procTrDelta[subProcsStc.camId].x ||
					tmpPnt.y != gPOptsRt->procTrDelta[subProcsStc.camId].y) {
				fcapshared::Shared::setRtOpts_procTrDelta(subProcsStc.camId, tmpPnt);
				gPOptsRt->procTrDelta[subProcsStc.camId] = tmpPnt;
			}
		}

		// region of interest
		if (gStaticOptionsStc.procEnabled.roi) {
			gOtherSubProcRoi.processFrame(frame);
		}

		// update output frame size
		if (frame.size().width != gPOptsRt->resolutionOutput.width ||
				frame.size().height != gPOptsRt->resolutionOutput.height) {
			fcapshared::Shared::setRtOpts_resolutionOutput(frame.size());
			gPOptsRt->resolutionOutput.width = frame.size().width;
			gPOptsRt->resolutionOutput.height = frame.size().height;
		}

		// grid
		if (gStaticOptionsStc.procEnabled.grid) {
			subProcsStc.grid.processFrame(frame);
		}

		// set Camera-Ready flag
		if (! gPOptsRt->cameraReady[subProcsStc.camId]) {
			fcapshared::Shared::setRtOpts_cameraReady(subProcsStc.camId, true);
			gPOptsRt->cameraReady[subProcsStc.camId] = true;
		}
	}

	void FrameProcessor::procAddTextOverlayCams(
			cv::Mat &frameOut, const std::string &camDesc, const fcapconstants::OutputCamsEn outputCams) {
		if (gLastOverlCamsOutputCamsInt == -1 || gLastOverlCamsOutputCamsInt != (int8_t)outputCams ||
				gLastOverlCamsResolutionOutpW != gPOptsRt->resolutionOutput.width) {
			double scale = (double)gPOptsRt->resolutionOutput.width / 1280.0;
			gOtherSubProcTextCams.setText(TEXT_CAM_TXT_PREFIX + camDesc, TEXT_CAM_COORD, TEXT_CAM_COLOR, scale);
			gLastOverlCamsOutputCamsInt = (int)outputCams;
			gLastOverlCamsResolutionOutpW = gPOptsRt->resolutionOutput.width;
		}
		gOtherSubProcTextCams.processFrame(frameOut);
	}

	void FrameProcessor::procAddTextOverlayCal(cv::Mat &frameOut, const bool isCalibrated) {
		if (gLastOverlCamsOutputCamsInt == -1) {
			return;
		}
		if (gLastOverlCalIsCalibratedInt == -1 || gLastOverlCalIsCalibratedInt != (int8_t)isCalibrated ||
				gLastOverlCalResolutionOutpW != gPOptsRt->resolutionOutput.width) {
			double scale = (double)gPOptsRt->resolutionOutput.width / 1280.0;
			gOtherSubProcTextCal.setText(
					isCalibrated ? TEXT_CAL_TXT_ISCAL : TEXT_CAL_TXT_UNCAL,
					TEXT_CAL_COORD + cv::Point(0, gOtherSubProcTextCams.getTextBottomY() + 5),
					isCalibrated ? TEXT_CAL_COLOR_ISCAL : TEXT_CAL_COLOR_UNCAL,
					scale
				);
			gLastOverlCalIsCalibratedInt = (int8_t)isCalibrated;
			gLastOverlCalResolutionOutpW = gPOptsRt->resolutionOutput.width;
		}
		gOtherSubProcTextCal.processFrame(frameOut);
	}

}  // namespace frame
