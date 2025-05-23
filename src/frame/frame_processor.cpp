#include <cstdio>
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
			gPOptsRt(nullptr),
			gLastOverlCalIsCalibratedInt(-1),
			gLastOverlCalResolutionOutpW(-1),
			gLastOverlCamsOutputCamsInt(-1),
			gLastOverlCamsResolutionOutpW(-1) {
		gStaticOptionsStc = fcapcfgfile::CfgFile::getStaticOptions();

		//
		initSubProcs();

		//
		log("Input Stream Framesize: " +
				std::to_string(gStaticOptionsStc.resolutionInputStream.width) +
				"x" +
				std::to_string(gStaticOptionsStc.resolutionInputStream.height));
	}

	void FrameProcessor::setRuntimeOptionsPnt(fcapshared::RuntimeOptionsStc *pOptsRt) {
		gPOptsRt = pOptsRt;
		updateSubProcsSettings();
	}

	void FrameProcessor::processFrame(cv::Mat *pFrameL, cv::Mat *pFrameR, cv::Mat *pFrameOut, const uint32_t frameNr) {
		const std::string* pCamDesc = nullptr;

		if (gPOptsRt->outputCams == fcapconstants::OutputCamsEn::CAM_L) {
			pCamDesc = &TEXT_CAM_TXT_SUFFIX_L;
		} else if (gPOptsRt->outputCams == fcapconstants::OutputCamsEn::CAM_R) {
			pCamDesc = &TEXT_CAM_TXT_SUFFIX_R;
		} else {
			pCamDesc = &TEXT_CAM_TXT_SUFFIX_BOTH;
		}

		// check frame size
		if (pFrameL != nullptr && ! checkFrameSize(pFrameL, TEXT_CAM_TXT_PREFIX + TEXT_CAM_TXT_SUFFIX_L)) {
			return;
		}
		if (pFrameR != nullptr && ! checkFrameSize(pFrameR, TEXT_CAM_TXT_PREFIX + TEXT_CAM_TXT_SUFFIX_R)) {
			return;
		}

		// do the actual processing
		if (! fcapsettings::DBG_PROC_DISABLE_ALL_PROCESSING) {
			if (pFrameL != nullptr) {
				procDefaults(gSubProcsL, *pFrameL, frameNr);
			}
			if (pFrameR != nullptr) {
				procDefaults(gSubProcsR, *pFrameR, frameNr);
			}
		}

		//
		if (gPOptsRt->outputCams == fcapconstants::OutputCamsEn::CAM_BOTH) {
			// adjust color channels
			if (pFrameR != nullptr && pFrameL != nullptr && pFrameR->channels() != pFrameL->channels()) {
				const int channL = pFrameL->channels();
				const int channR = pFrameR->channels();
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

		// scale the image
		if (gStaticOptionsStc.procEnabled.scale) {
			gOtherSubProcScale.processFrame(*pFrameOut, frameNr);
		}

		// update output frame size
		if (pFrameOut->size().width != gPOptsRt->resolutionOutput.width ||
				pFrameOut->size().height != gPOptsRt->resolutionOutput.height) {
			fcapshared::Shared::setRtOpts_resolutionOutput(pFrameOut->size());
			gPOptsRt->resolutionOutput.width = pFrameOut->size().width;
			gPOptsRt->resolutionOutput.height = pFrameOut->size().height;
		}

		// add grid
		if (gPOptsRt->procGridShow) {
			/**log("processFrame GRID");**/
			gOtherSubProcGrid.processFrame(*pFrameOut, frameNr);
		}

		// add text overlays
		if (! fcapsettings::DBG_PROC_DISABLE_ALL_PROCESSING &&
				(! gStaticOptionsStc.procEnabled.roi || gPOptsRt->procRoiSizePerc >= 40 || gStaticOptionsStc.procEnabled.scale) &&
				gPOptsRt->resolutionOutput.width >= 440) {
			// text overlay "CAM x"
			if (gStaticOptionsStc.procEnabled.overlCam) {
				/**log("processFrame OVERLCAMS");**/
				procAddTextOverlayCams(*pFrameOut, frameNr, *pCamDesc, gPOptsRt->outputCams);
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
				procAddTextOverlayCal(*pFrameOut, frameNr, tmpBool);
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
			int16_t tmpG = 0;
			gOtherSubProcBnc.getData(tmpB, tmpC, tmpG);
			fcapshared::Shared::setRtOpts_procBncAdjBrightness(tmpB);
			fcapshared::Shared::setRtOpts_procBncAdjContrast(tmpC);
			fcapshared::Shared::setRtOpts_procBncAdjGamma(tmpG);
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
		_initSubProcs_fspObj(fcapconstants::CamIdEn::CAM_0, fcapconstants::OutputCamsEn::CAM_BOTH, gOtherSubProcScale);
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
			framesubproc::FrameSubProcessor &fsp) const {
		fsp.setCamIdAndOutputCams(camId, outputCams);
		fsp.setInputFrameSize(gStaticOptionsStc.resolutionInputStream);
	}

	void FrameProcessor::updateSubProcsSettings() {
		if (gStaticOptionsStc.procEnabled.bnc && gPOptsRt->procBncChanged) {
			gOtherSubProcBnc.setBrightness(gPOptsRt->procBncAdjBrightness);
			gOtherSubProcBnc.setContrast(gPOptsRt->procBncAdjContrast);
			gOtherSubProcBnc.setGamma(gPOptsRt->procBncAdjGamma);
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

	void FrameProcessor::_updateSubProcsSettings_stc(SubProcsStc &subProcsStc) const {
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

	void FrameProcessor::procDefaults(SubProcsStc &subProcsStc, cv::Mat &frame, const uint32_t frameNr) {
		bool tmpBool;
		bool skipPt = false;
		bool skipTr = false;

		// adjust brightness and contrast
		if (gStaticOptionsStc.procEnabled.bnc) {
			/**log("procDefaults BNC CAM" + std::to_string((int)subProcsStc.camId));**/
			gOtherSubProcBnc.processFrame(frame, frameNr);
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
				subProcsStc.cal.processFrame(frame, frameNr);
			} /**else {
				log("procDefaults no CAL CAM" + std::to_string((int)subProcsStc.camId));
			}**/
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
					gPOptsRt->procPtDone[subProcsStc.camId] = false;
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
				subProcsStc.pt.processFrame(frame, frameNr);
			}
			//
			if (subProcsStc.pt.getNeedRectCorners()) {
				skipTr = true;
			}
		}

		// flip
		if (gStaticOptionsStc.procEnabled.flip) {
			/**log("procDefaults FLIP CAM" + std::to_string((int)subProcsStc.camId));**/
			subProcsStc.flip.processFrame(frame, frameNr);
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
				subProcsStc.tr.processFrame(frame, frameNr);
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
			gOtherSubProcRoi.processFrame(frame, frameNr);
		}

		// set Camera-Ready flag
		if (! gPOptsRt->cameraReady[subProcsStc.camId]) {
			fcapshared::Shared::setRtOpts_cameraReady(subProcsStc.camId, true);
			gPOptsRt->cameraReady[subProcsStc.camId] = true;
		}
	}

	void FrameProcessor::procAddTextOverlayCams(
			cv::Mat &frameOut,
			const uint32_t frameNr,
			const std::string &camDesc,
			const fcapconstants::OutputCamsEn outputCams) {
		if (gLastOverlCamsOutputCamsInt == -1 || gLastOverlCamsOutputCamsInt != (int8_t)outputCams ||
				gLastOverlCamsResolutionOutpW != gPOptsRt->resolutionOutput.width) {
			const double scale = static_cast<double>(gPOptsRt->resolutionOutput.width) / 1280.0;
			gOtherSubProcTextCams.setText(TEXT_CAM_TXT_PREFIX + camDesc, TEXT_CAM_COORD, TEXT_CAM_COLOR, scale);
			gLastOverlCamsOutputCamsInt = static_cast<int8_t>(outputCams);
			gLastOverlCamsResolutionOutpW = gPOptsRt->resolutionOutput.width;
		}
		gOtherSubProcTextCams.processFrame(frameOut, frameNr);
	}

	void FrameProcessor::procAddTextOverlayCal(cv::Mat &frameOut, const uint32_t frameNr, const bool isCalibrated) {
		if (gLastOverlCamsOutputCamsInt == -1) {
			return;
		}
		if (gLastOverlCalIsCalibratedInt == -1 || gLastOverlCalIsCalibratedInt != (int8_t)isCalibrated ||
				gLastOverlCalResolutionOutpW != gPOptsRt->resolutionOutput.width) {
			const double scale = static_cast<double>(gPOptsRt->resolutionOutput.width) / 1280.0;
			gOtherSubProcTextCal.setText(
					isCalibrated ? TEXT_CAL_TXT_ISCAL : TEXT_CAL_TXT_UNCAL,
					TEXT_CAL_COORD + cv::Point(0, gOtherSubProcTextCams.getTextBottomY() + 5),
					isCalibrated ? TEXT_CAL_COLOR_ISCAL : TEXT_CAL_COLOR_UNCAL,
					scale
				);
			gLastOverlCalIsCalibratedInt = static_cast<int8_t>(isCalibrated);
			gLastOverlCalResolutionOutpW = gPOptsRt->resolutionOutput.width;
		}
		gOtherSubProcTextCal.processFrame(frameOut, frameNr);
	}

	bool FrameProcessor::checkFrameSize(const cv::Mat *pFrame, const std::string &camName) const {
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
			const cv::Mat *pFrameL,
			const cv::Mat *pFrameR,
			cv::Mat *pFrameOut,
			__attribute__((unused)) const uint32_t frameNr) {
		/**auto timeStart = std::chrono::steady_clock::now();**/
		const auto targetWidth = static_cast<uint32_t>(pFrameL->size().width);
		const auto targetHeight = static_cast<uint32_t>(pFrameL->size().height);

		if (fcapsettings::PROC_DEFAULT_SPLITVIEW_FOR_CAMBOTH) {
			/**
			 * split view rendering is faster than blended view rendering
			 */
			const int32_t centerX = static_cast<int32_t>(targetWidth) / 2;
			const cv::Range rowRange(0, static_cast<int32_t>(targetHeight));
			const cv::Range colRangeL(0, centerX);
			const cv::Range colRangeR = cv::Range(centerX, static_cast<int32_t>(targetWidth));

			*pFrameOut = cv::Mat(static_cast<int32_t>(targetHeight), static_cast<int32_t>(targetWidth), CV_8UC3);
			//
			cv::Mat insetImageForL(
					*pFrameOut,
					cv::Rect(0, 0, centerX, static_cast<int32_t>(targetHeight))
				);
			(*pFrameL)(rowRange, colRangeL).copyTo(insetImageForL);
			//
			cv::Mat insetImageForR = cv::Mat(
					*pFrameOut,
					cv::Rect(centerX, 0, static_cast<int32_t>(targetWidth) - centerX, static_cast<int32_t>(targetHeight))
				);
			(*pFrameR)(rowRange, colRangeR).copyTo(insetImageForR);
			//
			cv::line(
					*pFrameOut,
					cv::Point(centerX - 1, 0),
					cv::Point(centerX - 1, static_cast<int32_t>(targetHeight) - 1),
					cv::Scalar(0, 0, 0),
					1
				);
			cv::line(
					*pFrameOut,
					cv::Point(centerX, 0),
					cv::Point(centerX, static_cast<int32_t>(targetHeight) - 1),
					cv::Scalar(255, 255, 255),
					1
				);
			cv::line(
					*pFrameOut,
					cv::Point(centerX + 1, 0),
					cv::Point(centerX + 1, static_cast<int32_t>(targetHeight) - 1),
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
