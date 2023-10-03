#include <chrono>
#include <iostream>
#include <sstream>
#include <string>  // for stoi()
#include <opencv2/opencv.hpp>

#include "../settings.hpp"
#include "http_handleroute_post.hpp"

using namespace std::chrono_literals;

namespace http {

	HandleRoutePost::HandleRoutePost(httppriv::HandleClientDataStc *pHndCltData) :
				HandleRoute(pHndCltData, "POST") {
	}

	HandleRoutePost::~HandleRoutePost() {
	}

	// -----------------------------------------------------------------------------

	bool HandleRoutePost::handleRequest(const std::string &requUriPath, const std::string &requUriQuery) {
		if (! HandleRoute::handleRequest(requUriPath, requUriQuery)) {
			return false;
		}
		//
		try {
			const HandleRoutePostFnc fncPnt = HANDLEROUTE_LUT.at(gRequUriPath);

			return ((*this).*(fncPnt))();
		} catch (std::out_of_range &ex) {
			/**log("404 invalid path '" + gRequUriPath + "'");**/
			log("404 invalid path");
			gPHndCltData->respHttpStat = 404;
			return false;
		}
	}

	// -----------------------------------------------------------------------------
	// -----------------------------------------------------------------------------

	bool HandleRoutePost::_handleRoute_OUTPUT_CAMS_ENABLE() {
		bool resB;
		fcapconstants::OutputCamsEn tmpVal = fcapconstants::OutputCamsEn::CAM_BOTH;
		fcapshared::RuntimeOptionsStc *pRtOptsOut = &gPHndCltData->rtOptsNew;

		log("200 Path=" + gRequUriPath);
		resB = getOutputCamsFromQuery(tmpVal);
		if (resB) {
			if (tmpVal == fcapconstants::OutputCamsEn::CAM_L &&
					gPHndCltData->rtOptsCur.outputCams == fcapconstants::OutputCamsEn::CAM_R &&
					gPHndCltData->isCameraAvailabelL()) {
				pRtOptsOut->outputCams = fcapconstants::OutputCamsEn::CAM_BOTH;
			} else if (tmpVal == fcapconstants::OutputCamsEn::CAM_R &&
					gPHndCltData->rtOptsCur.outputCams == fcapconstants::OutputCamsEn::CAM_L &&
					gPHndCltData->isCameraAvailabelR()) {
				pRtOptsOut->outputCams = fcapconstants::OutputCamsEn::CAM_BOTH;
			} else if (tmpVal == fcapconstants::OutputCamsEn::CAM_BOTH &&
					gPHndCltData->isCameraAvailabelL() && gPHndCltData->isCameraAvailabelR()) {
				pRtOptsOut->outputCams = fcapconstants::OutputCamsEn::CAM_BOTH;
			} else {
				gPHndCltData->respErrMsg = "cannot enable camera";
				resB = false;
			}
		}
		if (resB) {
			fcapshared::Shared::setRtOpts_outputCams(pRtOptsOut->outputCams);
		}
		gPHndCltData->respReturnJson = true;
		return resB;
	}

	bool HandleRoutePost::_handleRoute_OUTPUT_CAMS_DISABLE() {
		bool resB;
		fcapconstants::OutputCamsEn tmpVal = fcapconstants::OutputCamsEn::CAM_BOTH;
		fcapshared::RuntimeOptionsStc *pRtOptsOut = &gPHndCltData->rtOptsNew;

		log("200 Path=" + gRequUriPath);
		resB = getOutputCamsFromQuery(tmpVal);
		if (resB) {
			if (tmpVal == fcapconstants::OutputCamsEn::CAM_L) {
				switch (gPHndCltData->rtOptsCur.outputCams) {
					case fcapconstants::OutputCamsEn::CAM_L:
						gPHndCltData->respErrMsg = "cannot disable only active camera";
						resB = false;
						break;
					case fcapconstants::OutputCamsEn::CAM_R:
						break;
					default:  // BOTH
						pRtOptsOut->outputCams = fcapconstants::OutputCamsEn::CAM_R;
				}
			} else if (tmpVal == fcapconstants::OutputCamsEn::CAM_R) {
				switch (gPHndCltData->rtOptsCur.outputCams) {
					case fcapconstants::OutputCamsEn::CAM_L:
						break;
					case fcapconstants::OutputCamsEn::CAM_R:
						gPHndCltData->respErrMsg = "cannot disable only active camera";
						resB = false;
						break;
					default:  // BOTH
						pRtOptsOut->outputCams = fcapconstants::OutputCamsEn::CAM_L;
				}
			} else {
				gPHndCltData->respErrMsg = "cannot disable both cameras";
				resB = false;
			}
		}
		if (resB) {
			fcapshared::Shared::setRtOpts_outputCams(pRtOptsOut->outputCams);
		}
		gPHndCltData->respReturnJson = true;
		return resB;
	}

	bool HandleRoutePost::_handleRoute_OUTPUT_CAMS_SWAP() {
		bool resB = false;
		fcapshared::RuntimeOptionsStc *pRtOptsOut = &gPHndCltData->rtOptsNew;

		log("200 Path=" + gRequUriPath);
		if (gPHndCltData->rtOptsCur.outputCams == fcapconstants::OutputCamsEn::CAM_L &&
				gPHndCltData->isCameraAvailabelR()) {
			pRtOptsOut->outputCams = fcapconstants::OutputCamsEn::CAM_R;
			resB = true;
		} else if (gPHndCltData->rtOptsCur.outputCams == fcapconstants::OutputCamsEn::CAM_R &&
				gPHndCltData->isCameraAvailabelL()) {
			pRtOptsOut->outputCams = fcapconstants::OutputCamsEn::CAM_L;
			resB = true;
		} else {
			gPHndCltData->respErrMsg = "cannot swap cameras";
		}
		if (resB) {
			fcapshared::Shared::setRtOpts_outputCams(pRtOptsOut->outputCams);
		}
		gPHndCltData->respReturnJson = true;
		return resB;
	}

	bool HandleRoutePost::_handleRoute_PROC_BNC_BRIGHTN() {
		bool resB;
		fcapshared::RuntimeOptionsStc *pRtOptsOut = &gPHndCltData->rtOptsNew;

		log("200 Path=" + gRequUriPath);
		resB = getIntFromQuery(
				pRtOptsOut->procBncAdjBrightness,
				-100,
				100
			);
		if (resB && pRtOptsOut->procBncAdjBrightness != gPHndCltData->rtOptsCur.procBncAdjBrightness) {
			fcapshared::Shared::setRtOpts_procBncChanged(true);
			fcapshared::Shared::setRtOpts_procBncAdjBrightness(pRtOptsOut->procBncAdjBrightness);
		}
		gPHndCltData->respReturnJson = true;
		return resB;
	}

	bool HandleRoutePost::_handleRoute_PROC_BNC_CONTRAST() {
		bool resB;
		fcapshared::RuntimeOptionsStc *pRtOptsOut = &gPHndCltData->rtOptsNew;

		log("200 Path=" + gRequUriPath);
		resB = getIntFromQuery(
				pRtOptsOut->procBncAdjContrast,
				-100,
				100
			);
		if (resB && pRtOptsOut->procBncAdjContrast != gPHndCltData->rtOptsCur.procBncAdjContrast) {
			fcapshared::Shared::setRtOpts_procBncChanged(true);
			fcapshared::Shared::setRtOpts_procBncAdjContrast(pRtOptsOut->procBncAdjContrast);
		}
		gPHndCltData->respReturnJson = true;
		return resB;
	}

	bool HandleRoutePost::_handleRoute_PROC_BNC_GAMMA() {
		bool resB;
		fcapshared::RuntimeOptionsStc *pRtOptsOut = &gPHndCltData->rtOptsNew;

		log("200 Path=" + gRequUriPath);
		resB = getIntFromQuery(
				pRtOptsOut->procBncAdjGamma,
				-100,
				100
			);
		if (resB && pRtOptsOut->procBncAdjGamma != gPHndCltData->rtOptsCur.procBncAdjGamma) {
			fcapshared::Shared::setRtOpts_procBncChanged(true);
			fcapshared::Shared::setRtOpts_procBncAdjGamma(pRtOptsOut->procBncAdjGamma);
		}
		gPHndCltData->respReturnJson = true;
		return resB;
	}

	void HandleRoutePost::__handleRoute_PROC_CAL_SHOWCHESSCORNERS_x(fcapconstants::CamIdEn camId, bool newVal) {
		fcapshared::RuntimeOptionsStc *pRtOptsOut = &gPHndCltData->rtOptsNew;

		if (newVal != gPHndCltData->rtOptsCur.procCalShowCalibChessboardPoints[camId]) {
			fcapshared::Shared::setRtOpts_procCalChanged(camId, true);
			fcapshared::Shared::setRtOpts_procCalShowCalibChessboardPoints(camId, newVal);
			pRtOptsOut->procCalShowCalibChessboardPoints[camId] = newVal;
		}
	}

	bool HandleRoutePost::_handleRoute_PROC_CAL_SHOWCHESSCORNERS() {
		bool resB;
		bool tmpBool = false;

		log("200 Path=" + gRequUriPath);
		resB = getBoolFromQuery(tmpBool);
		if (resB) {
			// set flag for one or both cameras
			if (gPHndCltData->curCamId() != NULL) {
				__handleRoute_PROC_CAL_SHOWCHESSCORNERS_x(*gPHndCltData->curCamId(), tmpBool);
			} else {
				__handleRoute_PROC_CAL_SHOWCHESSCORNERS_x(fcapconstants::CamIdEn::CAM_0, tmpBool);
				__handleRoute_PROC_CAL_SHOWCHESSCORNERS_x(fcapconstants::CamIdEn::CAM_1, tmpBool);
			}
		}
		gPHndCltData->respReturnJson = true;
		return resB;
	}

	bool HandleRoutePost::_handleRoute_PROC_CAL_START() {
		bool resB = false;

		log("200 Path=" + gRequUriPath);
		if (gPHndCltData->curCamId() != NULL) {
			fcapshared::RuntimeOptionsStc *pRtOptsOut = &gPHndCltData->rtOptsNew;

			resB = true;
			pRtOptsOut->procCalDoStart[*gPHndCltData->curCamId()] = true;
			pRtOptsOut->procCalDoReset[*gPHndCltData->curCamId()] = true;
			pRtOptsOut->procCalDone[*gPHndCltData->curCamId()] = false;
			fcapshared::Shared::setRtOpts_procCalDoStart(*gPHndCltData->curCamId(), true);
			fcapshared::Shared::setRtOpts_procCalDoReset(*gPHndCltData->curCamId(), true);
			fcapshared::Shared::setRtOpts_procCalDone(*gPHndCltData->curCamId(), false);
		} else {
			gPHndCltData->respErrMsg = "cannot perform reset on both cameras";
		}
		gPHndCltData->respReturnJson = true;
		return resB;
	}

	bool HandleRoutePost::_handleRoute_PROC_CAL_RESET() {
		bool resB = false;

		log("200 Path=" + gRequUriPath);
		if (gPHndCltData->curCamId() != NULL) {
			fcapshared::RuntimeOptionsStc *pRtOptsOut = &gPHndCltData->rtOptsNew;

			resB = true;
			pRtOptsOut->procCalDoStart[*gPHndCltData->curCamId()] = false;
			pRtOptsOut->procCalDoReset[*gPHndCltData->curCamId()] = true;
			pRtOptsOut->procCalDone[*gPHndCltData->curCamId()] = false;
			fcapshared::Shared::setRtOpts_procCalDoStart(*gPHndCltData->curCamId(), false);
			fcapshared::Shared::setRtOpts_procCalDoReset(*gPHndCltData->curCamId(), true);
			fcapshared::Shared::setRtOpts_procCalDone(*gPHndCltData->curCamId(), false);
		} else {
			gPHndCltData->respErrMsg = "cannot perform reset on both cameras";
		}
		gPHndCltData->respReturnJson = true;
		return resB;
	}

	bool HandleRoutePost::_handleRoute_PROC_GRID_SHOW() {
		bool resB;
		fcapshared::RuntimeOptionsStc *pRtOptsOut = &gPHndCltData->rtOptsNew;

		log("200 Path=" + gRequUriPath);
		resB = getBoolFromQuery(pRtOptsOut->procGridShow);
		if (resB && pRtOptsOut->procGridShow != gPHndCltData->rtOptsCur.procGridShow) {
			fcapshared::Shared::setRtOpts_procGridChanged(true);
			fcapshared::Shared::setRtOpts_procGridShow(pRtOptsOut->procGridShow);
		}
		gPHndCltData->respReturnJson = true;
		return resB;
	}

	bool HandleRoutePost::_handleRoute_PROC_PT_RECTCORNER() {
		bool resB;
		cv::Point tmpPoint;

		log("200 Path=" + gRequUriPath);
		resB = getCoordsFromQuery(
				tmpPoint,
				cv::Point(0, 0),
				cv::Point(
						gPHndCltData->rtOptsCur.resolutionOutput.width - 1,
						gPHndCltData->rtOptsCur.resolutionOutput.height - 1
					)
			);
		if (resB && gPHndCltData->curCamId() == NULL) {
			gPHndCltData->respErrMsg = "cannot set corners on both cameras";
			resB = false;
		}
		if (resB) {
			fcapshared::RuntimeOptionsStc *pRtOptsOut = &gPHndCltData->rtOptsNew;

			pRtOptsOut->procPtRectCorners[*gPHndCltData->curCamId()].push_back(tmpPoint);
			pRtOptsOut->procPtDone[*gPHndCltData->curCamId()] =
					(pRtOptsOut->procPtRectCorners[*gPHndCltData->curCamId()].size() == fcapconstants::PROC_PT_RECTCORNERS_MAX);
			fcapshared::Shared::setRtOpts_procPtChanged(*gPHndCltData->curCamId(), true);
			fcapshared::Shared::setRtOpts_procPtRectCorners(
					*gPHndCltData->curCamId(),
					pRtOptsOut->procPtRectCorners[*gPHndCltData->curCamId()]
				);
		}
		gPHndCltData->respReturnJson = true;
		return resB;
	}

	bool HandleRoutePost::_handleRoute_PROC_PT_RESET() {
		bool resB = false;

		log("200 Path=" + gRequUriPath);
		if (gPHndCltData->curCamId() != NULL) {
			fcapshared::RuntimeOptionsStc *pRtOptsOut = &gPHndCltData->rtOptsNew;

			resB = true;
			pRtOptsOut->procPtRectCorners[*gPHndCltData->curCamId()].clear();
			pRtOptsOut->procPtDone[*gPHndCltData->curCamId()] = false;
			pRtOptsOut->procPtDoReset[*gPHndCltData->curCamId()] = true;
			fcapshared::Shared::setRtOpts_procPtDoReset(*gPHndCltData->curCamId(), true);
		} else {
			gPHndCltData->respErrMsg = "cannot perform reset on both cameras";
		}
		gPHndCltData->respReturnJson = true;
		return resB;
	}

	bool HandleRoutePost::_handleRoute_PROC_ROI_SIZE() {
		bool resB;
		fcapshared::RuntimeOptionsStc *pRtOptsOut = &gPHndCltData->rtOptsNew;
		int16_t tmpInt = 0;

		log("200 Path=" + gRequUriPath);
		resB = getIntFromQuery(tmpInt, 10, 100);
		if (resB && (uint8_t)tmpInt != gPHndCltData->rtOptsCur.procRoiSizePerc) {
			pRtOptsOut->procRoiSizePerc = (uint8_t)tmpInt;
			fcapshared::Shared::setRtOpts_procRoiChanged(true);
			fcapshared::Shared::setRtOpts_procRoiSizePerc(pRtOptsOut->procRoiSizePerc);
		}
		gPHndCltData->respReturnJson = true;
		return resB;
	}

	bool HandleRoutePost::__handleRoute_PROC_TR_FIXDELTA_x(fcapconstants::CamIdEn camId) {
		bool resB = false;
		fcapshared::RuntimeOptionsStc *pRtOptsOut = &gPHndCltData->rtOptsNew;
		cv::Point tmpPoint;

		log("200 Path=" + gRequUriPath);
		resB = getCoordsFromQuery(
				tmpPoint,
				cv::Point(-10000, -10000),
				cv::Point(10000, 10000)
			);
		if (resB && tmpPoint != gPHndCltData->rtOptsCur.procTrFixDelta[camId]) {
			/**log("__set to x=" + std::to_string(tmpPoint.x) + ", y=" + std::to_string(tmpPoint.y));**/
			pRtOptsOut->procTrFixDelta[camId] = tmpPoint;
			pRtOptsOut->procTrChanged[camId] = true;
			fcapshared::Shared::setRtOpts_procTrFixDelta(camId, tmpPoint);
			fcapshared::Shared::setRtOpts_procTrChanged(camId, true);
		}
		gPHndCltData->respReturnJson = true;
		return resB;
	}

	bool HandleRoutePost::_handleRoute_PROC_TR_FIXDELTA_L() {
		return __handleRoute_PROC_TR_FIXDELTA_x(gPHndCltData->staticOptionsStc.camL);
	}

	bool HandleRoutePost::_handleRoute_PROC_TR_FIXDELTA_R() {
		return __handleRoute_PROC_TR_FIXDELTA_x(gPHndCltData->staticOptionsStc.camR);
	}

	bool HandleRoutePost::_handleRoute_PROC_TR_DYNDELTA() {
		bool resB = false;
		fcapshared::RuntimeOptionsStc *pRtOptsOut = &gPHndCltData->rtOptsNew;
		cv::Point tmpPointL;
		cv::Point tmpPointR;

		log("200 Path=" + gRequUriPath);
		resB = getDualCoordsFromQuery(
				tmpPointL,
				tmpPointR,
				cv::Point(-10000, -10000),
				cv::Point(10000, 10000)
			);
		if (resB &&
				(tmpPointL != gPHndCltData->rtOptsCur.procTrDynDelta[gPHndCltData->staticOptionsStc.camL] ||
					tmpPointR != gPHndCltData->rtOptsCur.procTrDynDelta[gPHndCltData->staticOptionsStc.camR])) {
			fcapconstants::CamIdEn camId;
			//
			/**log("__set to Lx=" + std::to_string(tmpPointL.x) + ", Ly=" + std::to_string(tmpPointL.y));**/
			camId = gPHndCltData->staticOptionsStc.camL;
			pRtOptsOut->procTrDynDelta[camId] = tmpPointL;
			pRtOptsOut->procTrChanged[camId] = true;
			fcapshared::Shared::setRtOpts_procTrDynDelta(camId, tmpPointL);
			fcapshared::Shared::setRtOpts_procTrChanged(camId, true);
			//
			/**log("__set to Rx=" + std::to_string(tmpPointR.x) + ", Ry=" + std::to_string(tmpPointR.y));**/
			camId = gPHndCltData->staticOptionsStc.camR;
			pRtOptsOut->procTrDynDelta[camId] = tmpPointR;
			pRtOptsOut->procTrChanged[camId] = true;
			fcapshared::Shared::setRtOpts_procTrDynDelta(camId, tmpPointR);
			fcapshared::Shared::setRtOpts_procTrChanged(camId, true);
		}
		gPHndCltData->respReturnJson = true;
		return resB;
	}

	bool HandleRoutePost::_handleRoute_PROC_TR_RESET() {
		bool resB = false;

		log("200 Path=" + gRequUriPath);
		if (gPHndCltData->curCamId() != NULL) {
			fcapshared::RuntimeOptionsStc *pRtOptsOut = &gPHndCltData->rtOptsNew;

			resB = true;
			pRtOptsOut->procTrFixDelta[*gPHndCltData->curCamId()] = cv::Point();
			pRtOptsOut->procTrDynDelta[*gPHndCltData->curCamId()] = cv::Point();
			pRtOptsOut->procTrDoReset[*gPHndCltData->curCamId()] = true;
			fcapshared::Shared::setRtOpts_procTrDoReset(*gPHndCltData->curCamId(), true);
		} else {
			gPHndCltData->respErrMsg = "cannot perform reset on both cameras";
		}
		gPHndCltData->respReturnJson = true;
		return resB;
	}

}  // namespace http
