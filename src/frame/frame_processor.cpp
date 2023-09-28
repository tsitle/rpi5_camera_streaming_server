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

	void FrameProcessor::processFrame(cv::Mat *pFrameL, cv::Mat *pFrameR, cv::Mat *pFrameOut, const uint32_t frameNr) {
		const std::string* pCamDesc = NULL;

		if (gPOptsRt->outputCams == fcapconstants::OutputCamsEn::CAM_L) {
			pCamDesc = &TEXT_CAM_TXT_SUFFIX_L;
		} else if (gPOptsRt->outputCams == fcapconstants::OutputCamsEn::CAM_R) {
			pCamDesc = &TEXT_CAM_TXT_SUFFIX_R;
		} else {
			pCamDesc = &TEXT_CAM_TXT_SUFFIX_BOTH;
		}

		// check frame size
		if (pFrameL != NULL && ! checkFrameSize(pFrameL, TEXT_CAM_TXT_PREFIX + TEXT_CAM_TXT_SUFFIX_L)) {
			return;
		}
		if (pFrameR != NULL && ! checkFrameSize(pFrameR, TEXT_CAM_TXT_PREFIX + TEXT_CAM_TXT_SUFFIX_R)) {
			return;
		}

		// do the actual processing
		if (! fcapsettings::DBG_PROC_DISABLE_ALL_PROCESSING) {
			if (pFrameL != NULL) {
				procDefaults(gSubProcsL, *pFrameL);
			}
			if (pFrameR != NULL) {
				procDefaults(gSubProcsR, *pFrameR);
			}
		}

		//
		if (gPOptsRt->outputCams == fcapconstants::OutputCamsEn::CAM_BOTH) {
			// adjust color channels
			if (pFrameR->channels() != pFrameL->channels()) {
				int channL = pFrameL->channels();
				int channR = pFrameR->channels();
				if (channL > channR) {
					cv::cvtColor(*pFrameL, *pFrameL, cv::COLOR_BGR2GRAY);
				} else {
					cv::cvtColor(*pFrameR, *pFrameR, cv::COLOR_BGR2GRAY);
				}
			}
			// render split view (or blended view)
			/**log("processFrame renderMasterOutput");**/
			renderMasterOutput(pFrameL, pFrameR, pFrameOut, frameNr);
		}

		// add grid
		if (gPOptsRt->procGridShow) {
			/**log("processFrame GRID");**/
			gOtherSubProcGrid.processFrame(*pFrameOut);
		}

		// add text overlays
		if (! fcapsettings::DBG_PROC_DISABLE_ALL_PROCESSING &&
				(!gStaticOptionsStc.procEnabled.roi || gPOptsRt->procRoiSizePerc >= 40) &&
				gPOptsRt->resolutionOutput.width >= 350) {
			// text overlay "CAM x"
			if (gStaticOptionsStc.procEnabled.overlCam) {
				/**log("processFrame OVERLCAMS");**/
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
				/**log("processFrame OVERLCAL");**/
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
		// other FrameSubProcs for the individual channels/cameras
		///
		_initSubProcs_fspObj(fcapconstants::CamIdEn::CAM_0, fcapconstants::OutputCamsEn::CAM_BOTH, gOtherSubProcBnc);
		if (gStaticOptionsStc.procEnabled.bnc) {
			gOtherSubProcBnc.loadData();
			//
			int16_t tmpB = 0;
			int16_t tmpC = 0;
			gOtherSubProcBnc.getData(tmpB, tmpC);
			fcapshared::Shared::setRtOpts_procBncAdjBrightness(tmpB);
			fcapshared::Shared::setRtOpts_procBncAdjContrast(tmpC);
		}
		///
		_initSubProcs_fspObj(fcapconstants::CamIdEn::CAM_0, fcapconstants::OutputCamsEn::CAM_BOTH, gOtherSubProcRoi);
		if (gStaticOptionsStc.procEnabled.roi) {
			gOtherSubProcRoi.loadData();
			gRoiOutputSz = gOtherSubProcRoi.getOutputSz();
			fcapshared::Shared::setRtOpts_procRoiSizePerc(
					gOtherSubProcRoi.getSizePercent()
				);
		} else {
			gRoiOutputSz = gStaticOptionsStc.resolutionInputStream;
		}

		// other FrameSubProcs for the master output
		///
		_initSubProcs_fspObj(fcapconstants::CamIdEn::CAM_0, fcapconstants::OutputCamsEn::CAM_BOTH, gOtherSubProcGrid);
		///
		_initSubProcs_fspObj(fcapconstants::CamIdEn::CAM_0, fcapconstants::OutputCamsEn::CAM_BOTH, gOtherSubProcTextCams);
		_initSubProcs_fspObj(fcapconstants::CamIdEn::CAM_0, fcapconstants::OutputCamsEn::CAM_BOTH, gOtherSubProcTextCal);

		// per channel/camera FrameSubProcs
		_initSubProcs_stc(gStaticOptionsStc.camL, fcapconstants::OutputCamsEn::CAM_L, gSubProcsL);
		_initSubProcs_stc(gStaticOptionsStc.camR, fcapconstants::OutputCamsEn::CAM_R, gSubProcsR);
	}

	void FrameProcessor::_initSubProcs_stc(
			fcapconstants::CamIdEn camId, fcapconstants::OutputCamsEn outputCams, SubProcsStc &subProcsStc) {
		subProcsStc.camId = camId;
		subProcsStc.outputCams = outputCams;
		//
		_initSubProcs_fspObj(camId, outputCams, subProcsStc.cal);
		if (gStaticOptionsStc.procEnabled.cal) {
			subProcsStc.cal.loadData();
		}
		//
		subProcsStc.flip.setData(gStaticOptionsStc.flip[camId].hor, gStaticOptionsStc.flip[camId].ver);
		_initSubProcs_fspObj(camId, outputCams, subProcsStc.flip);
		//
		_initSubProcs_fspObj(camId, outputCams, subProcsStc.pt);
		if (gStaticOptionsStc.procEnabled.pt) {
			subProcsStc.pt.setRoiOutputSz(gRoiOutputSz);
			subProcsStc.pt.loadData();
		}
		//
		_initSubProcs_fspObj(camId, outputCams, subProcsStc.tr);
		if (gStaticOptionsStc.procEnabled.tr) {
			subProcsStc.tr.loadData();
			//
			cv::Point tmpPnt;
			subProcsStc.tr.getFixDelta(tmpPnt.x, tmpPnt.y);
			fcapshared::Shared::setRtOpts_procTrFixDelta(subProcsStc.camId, tmpPnt);
		}
	}

	void FrameProcessor::_initSubProcs_fspObj(
			fcapconstants::CamIdEn camId,
			fcapconstants::OutputCamsEn outputCams,
			framesubproc::FrameSubProcessor &fsp) {
		fsp.setCamIdAndOutputCams(camId, outputCams);
		fsp.setInputFrameSize(gStaticOptionsStc.resolutionInputStream);
	}

	void FrameProcessor::updateSubProcsSettings() {
		if (gStaticOptionsStc.procEnabled.bnc && gPOptsRt->procBncChanged) {
			gOtherSubProcBnc.setBrightness(gPOptsRt->procBncAdjBrightness);
			gOtherSubProcBnc.setContrast(gPOptsRt->procBncAdjContrast);
			//
			fcapshared::Shared::setRtOpts_procBncChanged(false);
			gPOptsRt->procBncChanged = false;
		}
		//
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
		if (gStaticOptionsStc.procEnabled.cal && gPOptsRt->procCalChanged[subProcsStc.camId]) {
			subProcsStc.cal.setShowCalibChessboardPoints(gPOptsRt->procCalShowCalibChessboardPoints[subProcsStc.camId]);
			//
			fcapshared::Shared::setRtOpts_procCalChanged(subProcsStc.camId, false);
			gPOptsRt->procCalChanged[subProcsStc.camId] = false;
		}
		//
		if (gStaticOptionsStc.procEnabled.pt && gPOptsRt->procPtChanged[subProcsStc.camId]) {
			subProcsStc.pt.setManualRectCorners(gPOptsRt->procPtRectCorners[subProcsStc.camId]);
			subProcsStc.pt.setRoiOutputSz(gRoiOutputSz);
			//
			fcapshared::Shared::setRtOpts_procPtChanged(subProcsStc.camId, false);
			gPOptsRt->procPtChanged[subProcsStc.camId] = false;
		}
		//
		if (gStaticOptionsStc.procEnabled.tr && gPOptsRt->procTrChanged[subProcsStc.camId]) {
			subProcsStc.tr.setFixDelta(gPOptsRt->procTrFixDelta[subProcsStc.camId].x, gPOptsRt->procTrFixDelta[subProcsStc.camId].y);
			subProcsStc.tr.setDynDelta(gPOptsRt->procTrDynDelta[subProcsStc.camId].x, gPOptsRt->procTrDynDelta[subProcsStc.camId].y);
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
			/**log("procDefaults BNC CAM" + std::to_string((int)subProcsStc.camId));**/
			gOtherSubProcBnc.processFrame(frame);
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
			bool tmpDoStart = gPOptsRt->procCalDoStart[subProcsStc.camId];
			if (tmpDoStart || gPOptsRt->procCalDone[subProcsStc.camId]) {
				/**log("procDefaults CAL CAM" + std::to_string((int)subProcsStc.camId));**/
				subProcsStc.cal.processFrame(frame);
			}
			//
			tmpBool = subProcsStc.cal.getIsCalibrated();
			if (tmpBool != gPOptsRt->procCalDone[subProcsStc.camId]) {
				fcapshared::Shared::setRtOpts_procCalDone(subProcsStc.camId, tmpBool);
				gPOptsRt->procCalDone[subProcsStc.camId] = tmpBool;
				//
				if (tmpBool && tmpDoStart) {
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
				if (! tmpBool && gStaticOptionsStc.procEnabled.cal && subProcsStc.cal.getIsCalibrated()) {
					std::vector<cv::Point> tmpCalCorners = subProcsStc.cal.getRectCorners();

					subProcsStc.pt.setCalRectCorners(tmpCalCorners);
					tmpBool = true;
					gPOptsRt->procPtDone[subProcsStc.camId] = ! tmpBool;
				}
				if (tmpBool != gPOptsRt->procPtDone[subProcsStc.camId]) {
					fcapshared::Shared::setRtOpts_procPtDone(subProcsStc.camId, tmpBool);
					gPOptsRt->procPtDone[subProcsStc.camId] = tmpBool;
					//
					std::vector<cv::Point> tmpCorners = subProcsStc.pt.getManualRectCorners();
					fcapshared::Shared::setRtOpts_procPtRectCorners(subProcsStc.camId, tmpCorners);
					gPOptsRt->procPtRectCorners[subProcsStc.camId] = tmpCorners;
				}
			}
			//
			if (! skipPt) {
				/**log("procDefaults PT CAM" + std::to_string((int)subProcsStc.camId));**/
				subProcsStc.pt.processFrame(frame);
			}
			//
			if (subProcsStc.pt.getNeedRectCorners()) {
				skipTr = true;
			}
		}

		// flip
		if (gStaticOptionsStc.procEnabled.flip) {
			/**log("procDefaults FLIP CAM" + std::to_string((int)subProcsStc.camId));**/
			subProcsStc.flip.processFrame(frame);
		}

		// translation
		if (gStaticOptionsStc.procEnabled.tr) {
			bool tmpDoReset = gPOptsRt->procTrDoReset[subProcsStc.camId];
			if (tmpDoReset) {
				fcapshared::Shared::setRtOpts_procTrDoReset(subProcsStc.camId, false);
				gPOptsRt->procTrDoReset[subProcsStc.camId] = false;
				//
				subProcsStc.tr.resetData();
			}
			//
			if (! skipTr) {
				/**log("procDefaults TR CAM" + std::to_string((int)subProcsStc.camId));**/
				subProcsStc.tr.processFrame(frame);
			}
			//
			if (tmpDoReset) {
				cv::Point tmpPnt;
				subProcsStc.tr.getFixDelta(tmpPnt.x, tmpPnt.y);
				if (tmpPnt.x != gPOptsRt->procTrFixDelta[subProcsStc.camId].x ||
						tmpPnt.y != gPOptsRt->procTrFixDelta[subProcsStc.camId].y) {
					fcapshared::Shared::setRtOpts_procTrFixDelta(subProcsStc.camId, tmpPnt);
					gPOptsRt->procTrFixDelta[subProcsStc.camId] = tmpPnt;
				}
			}
		}

		// region of interest
		if (gStaticOptionsStc.procEnabled.roi) {
			/**log("procDefaults ROI CAM" + std::to_string((int)subProcsStc.camId));**/
			gOtherSubProcRoi.processFrame(frame);
		}

		// update output frame size
		if (frame.size().width != gPOptsRt->resolutionOutput.width ||
				frame.size().height != gPOptsRt->resolutionOutput.height) {
			fcapshared::Shared::setRtOpts_resolutionOutput(frame.size());
			gPOptsRt->resolutionOutput.width = frame.size().width;
			gPOptsRt->resolutionOutput.height = frame.size().height;
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

	bool FrameProcessor::checkFrameSize(const cv::Mat *pFrame, const std::string camName) {
		if (pFrame->size().width == gStaticOptionsStc.resolutionInputStream.width &&
				pFrame->size().height == gStaticOptionsStc.resolutionInputStream.height) {
			return true;
		}
		log("Error: [" + camName + "] input frame size doesn't match resolutionInputStream (is:" +
				std::to_string(pFrame->size().width) + "x" +
				std::to_string(pFrame->size().height) +
				", exp:" +
				std::to_string(gStaticOptionsStc.resolutionInputStream.width) + "x" +
				std::to_string(gStaticOptionsStc.resolutionInputStream.height) +
				")");
		fcapshared::Shared::setFlagNeedToStop();
		return false;
	}

	void FrameProcessor::renderMasterOutput(
			cv::Mat *pFrameL,
			cv::Mat *pFrameR,
			cv::Mat *pFrameOut,
			__attribute__((unused)) const uint32_t frameNr) {
		/**auto timeStart = std::chrono::steady_clock::now();**/

		if (fcapsettings::PROC_DEFAULT_SPLITVIEW_FOR_CAMBOTH) {
			/**
			 * split view rendering is faster than blended view rendering
			 */
			const int32_t centerX = gPOptsRt->resolutionOutput.width / 2;
			const cv::Range rowRange(0, gPOptsRt->resolutionOutput.height);
			const cv::Range colRangeL(0, centerX);
			const cv::Range colRangeR = cv::Range(centerX, gPOptsRt->resolutionOutput.width);

			*pFrameOut = cv::Mat(gPOptsRt->resolutionOutput.height, gPOptsRt->resolutionOutput.width, CV_8UC3);
			//
			cv::Mat insetImageForL(
					*pFrameOut,
					cv::Rect(0, 0, centerX, gPOptsRt->resolutionOutput.height)
				);
			(*pFrameL)(rowRange, colRangeL).copyTo(insetImageForL);
			//
			cv::Mat insetImageForR = cv::Mat(
					*pFrameOut,
					cv::Rect(centerX, 0, gPOptsRt->resolutionOutput.width - centerX, gPOptsRt->resolutionOutput.height)
				);
			(*pFrameR)(rowRange, colRangeR).copyTo(insetImageForR);
			//
			cv::line(
					*pFrameOut,
					cv::Point(centerX - 1, 0),
					cv::Point(centerX - 1, gPOptsRt->resolutionOutput.height - 1),
					cv::Scalar(0, 0, 0),
					1
				);
			cv::line(
					*pFrameOut,
					cv::Point(centerX, 0),
					cv::Point(centerX, gPOptsRt->resolutionOutput.height - 1),
					cv::Scalar(255, 255, 255),
					1
				);
			cv::line(
					*pFrameOut,
					cv::Point(centerX + 1, 0),
					cv::Point(centerX + 1, gPOptsRt->resolutionOutput.height - 1),
					cv::Scalar(0, 0, 0),
					1
				);
		} else {
			/**
			 * blended view rendering takes about 2.5 times longer than split view rendering
			 */
			cv::addWeighted(*pFrameL, /*alpha:*/0.5, *pFrameR, /*beta:*/0.5, /*gamma:*/0, *pFrameOut, -1);
		}

		/**auto timeEnd = std::chrono::steady_clock::now();
		if ((frameNr % 10) == 0) {
			log("blend: Elapsed time: " +
					std::to_string(std::chrono::duration_cast<std::chrono::microseconds>(timeEnd - timeStart).count()) +
					" us");
		}**/
	}

}  // namespace frame
