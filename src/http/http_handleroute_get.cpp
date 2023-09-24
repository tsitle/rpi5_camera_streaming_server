#include <chrono>
#include <iostream>
#include <sstream>
#include <string>  // for stoi()
#include <opencv2/opencv.hpp>

#include "../settings.hpp"
#include "http_handleroute_get.hpp"

using namespace std::chrono_literals;

namespace http {

	HandleRouteGet::HandleRouteGet(HandleClientDataStc *pHndCltData) :
				gPHndCltData(pHndCltData) {
	}

	HandleRouteGet::~HandleRouteGet() {
	}

	// -----------------------------------------------------------------------------

	bool HandleRouteGet::handleRequest(const std::string &requUriPath, const std::string &requUriQuery) {
		gRequUriPath = requUriPath;
		gRequUriQuery = requUriQuery;
		//
		try {
			const HandleRouteGetFnc fncPnt = HANDLEROUTE_LUT.at(gRequUriPath);

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

	void HandleRouteGet::log(const std::string &message) {
		std::cout << "CLIENT#" << std::to_string(gPHndCltData->thrIx)
				<< " [GET]: " << message << std::endl;
	}

	// -----------------------------------------------------------------------------

	bool HandleRouteGet::_handleRoute_ROOT() {
		log("200 Path=" + gRequUriPath);
		gPHndCltData->respHttpMsgString = buildWebsite();
		return true;
	}

	bool HandleRouteGet::_handleRoute_STREAM() {
		bool resB = false;

		gPHndCltData->isNewStreamingClientAccepted = gPHndCltData->cbIncStreamingClientCount();
		if (! gPHndCltData->isNewStreamingClientAccepted) {
			log("500 Path=" + gRequUriPath);
			log("__cannot accept more streaming clients at the moment");
			gPHndCltData->respHttpMsgString = "too many clients";
		} else {
			log("200 Path=" + gRequUriPath);
			resB = true;
		}
		return resB;
	}

	bool HandleRouteGet::_handleRoute_FAVICON() {
		log("404 Path=" + gRequUriPath);
		gPHndCltData->respHttpStat = 404;
		return false;
	}

	bool HandleRouteGet::_handleRoute_OUTPUT_CAMS_ENABLE() {
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

	bool HandleRouteGet::_handleRoute_OUTPUT_CAMS_DISABLE() {
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

	bool HandleRouteGet::_handleRoute_OUTPUT_CAMS_SWAP() {
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

	bool HandleRouteGet::_handleRoute_PROC_BNC_BRIGHTN() {
		bool resB;
		fcapshared::RuntimeOptionsStc *pRtOptsOut = &gPHndCltData->rtOptsNew;

		log("200 Path=" + gRequUriPath);
		resB = getIntFromQuery(
				pRtOptsOut->procBncAdjBrightness,
				fcapconstants::PROC_BNC_MIN_ADJ_BRIGHTNESS,
				fcapconstants::PROC_BNC_MAX_ADJ_BRIGHTNESS
			);
		if (resB && pRtOptsOut->procBncAdjBrightness != gPHndCltData->rtOptsCur.procBncAdjBrightness) {
			fcapshared::Shared::setRtOpts_procBncChanged(*gPHndCltData->curCamId(), true);
			fcapshared::Shared::setRtOpts_procBncAdjBrightness(pRtOptsOut->procBncAdjBrightness);
		}
		gPHndCltData->respReturnJson = true;
		return resB;
	}

	bool HandleRouteGet::_handleRoute_PROC_BNC_CONTRAST() {
		bool resB;
		fcapshared::RuntimeOptionsStc *pRtOptsOut = &gPHndCltData->rtOptsNew;

		log("200 Path=" + gRequUriPath);
		resB = getIntFromQuery(
				pRtOptsOut->procBncAdjContrast,
				fcapconstants::PROC_BNC_MIN_ADJ_CONTRAST,
				fcapconstants::PROC_BNC_MAX_ADJ_CONTRAST
			);
		if (resB && pRtOptsOut->procBncAdjContrast != gPHndCltData->rtOptsCur.procBncAdjContrast) {
			fcapshared::Shared::setRtOpts_procBncChanged(*gPHndCltData->curCamId(), true);
			fcapshared::Shared::setRtOpts_procBncAdjContrast(pRtOptsOut->procBncAdjContrast);
		}
		gPHndCltData->respReturnJson = true;
		return resB;
	}

	bool HandleRouteGet::_handleRoute_PROC_CAL_SHOWCHESSCORNERS() {
		bool resB;
		fcapshared::RuntimeOptionsStc *pRtOptsOut = &gPHndCltData->rtOptsNew;

		log("200 Path=" + gRequUriPath);
		resB = getBoolFromQuery(pRtOptsOut->procCalShowCalibChessboardPoints);
		if (resB && pRtOptsOut->procCalShowCalibChessboardPoints != gPHndCltData->rtOptsCur.procCalShowCalibChessboardPoints) {
			fcapshared::Shared::setRtOpts_procCalChanged(*gPHndCltData->curCamId(), true);
			fcapshared::Shared::setRtOpts_procCalShowCalibChessboardPoints(pRtOptsOut->procCalShowCalibChessboardPoints);
		}
		gPHndCltData->respReturnJson = true;
		return resB;
	}

	bool HandleRouteGet::_handleRoute_PROC_CAL_START() {
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

	bool HandleRouteGet::_handleRoute_PROC_CAL_RESET() {
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

	bool HandleRouteGet::_handleRoute_PROC_PT_RECTCORNER() {
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

	bool HandleRouteGet::_handleRoute_PROC_PT_RESET() {
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

	bool HandleRouteGet::_handleRoute_PROC_ROI_SIZE() {
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

	bool HandleRouteGet::__handleRoute_PROC_TR_DELTA_x(fcapconstants::CamIdEn camId) {
		bool resB = false;
		fcapshared::RuntimeOptionsStc *pRtOptsOut = &gPHndCltData->rtOptsNew;
		cv::Point tmpPoint;

		log("200 Path=" + gRequUriPath);
		resB = getCoordsFromQuery(
				tmpPoint,
				cv::Point(-10000, -10000),
				cv::Point(10000, 10000)
			);
		if (resB && tmpPoint != gPHndCltData->rtOptsCur.procTrDelta[camId]) {
			/**log("__set to x=" + std::to_string(tmpPoint.x) + ", y=" + std::to_string(tmpPoint.y));**/
			pRtOptsOut->procTrDelta[camId] = tmpPoint;
			pRtOptsOut->procTrChanged[camId] = true;
			fcapshared::Shared::setRtOpts_procTrDelta(camId, tmpPoint);
			fcapshared::Shared::setRtOpts_procTrChanged(camId, true);
		}
		gPHndCltData->respReturnJson = true;
		return resB;
	}

	bool HandleRouteGet::_handleRoute_PROC_TR_DELTA_L() {
		return __handleRoute_PROC_TR_DELTA_x(gPHndCltData->staticOptionsStc.camL);
	}

	bool HandleRouteGet::_handleRoute_PROC_TR_DELTA_R() {
		return __handleRoute_PROC_TR_DELTA_x(gPHndCltData->staticOptionsStc.camR);
	}

	bool HandleRouteGet::_handleRoute_PROC_TR_RESET() {
		bool resB = false;

		log("200 Path=" + gRequUriPath);
		if (gPHndCltData->curCamId() != NULL) {
			fcapshared::RuntimeOptionsStc *pRtOptsOut = &gPHndCltData->rtOptsNew;

			resB = true;
			pRtOptsOut->procTrDelta[*gPHndCltData->curCamId()] = cv::Point();
			pRtOptsOut->procTrDoReset[*gPHndCltData->curCamId()] = true;
			fcapshared::Shared::setRtOpts_procTrDoReset(*gPHndCltData->curCamId(), true);
		} else {
			gPHndCltData->respErrMsg = "cannot perform reset on both cameras";
		}
		gPHndCltData->respReturnJson = true;
		return resB;
	}

	bool HandleRouteGet::_handleRoute_STATUS() {
		/**log("200 Path=" + gRequUriPath);**/
		gPHndCltData->respReturnJson = true;

		return true;
	}

	// -----------------------------------------------------------------------------

	bool HandleRouteGet::getBoolFromQuery(bool &valOut) {
		bool resB = true;

		try {
			if (gRequUriQuery.empty()) {
				throw std::exception();
			}
			if (gRequUriQuery.compare("0") != 0 && gRequUriQuery.compare("1") != 0) {
				throw std::exception();
			}
			valOut = (gRequUriQuery.compare("1") == 0);
		} catch (std::exception &err) {
			gPHndCltData->respErrMsg = "invalid boolean value (allowed: 0 and 1)";
			resB = false;
		}
		return resB;
	}

	bool HandleRouteGet::getIntFromQuery(int16_t &valOut, const int16_t valMin, const int16_t valMax) {
		bool resB = true;

		try {
			if (gRequUriQuery.empty()) {
				throw std::exception();
			}
			int16_t tmpInt = stoi(gRequUriQuery);
			if (tmpInt < valMin || tmpInt > valMax) {
				throw std::exception();
			}
			valOut = tmpInt;
		} catch (std::exception &err) {
			gPHndCltData->respErrMsg = "invalid integer value (allowed: " + std::to_string(valMin) + ".." + std::to_string(valMax) + ")";
			resB = false;
		}
		return resB;
	}

	bool HandleRouteGet::getOutputCamsFromQuery(fcapconstants::OutputCamsEn &valOut) {
		bool resB = true;

		try {
			if (gRequUriQuery.empty()) {
				throw std::exception();
			}
			if (gRequUriQuery.compare("L") == 0) {
				valOut = fcapconstants::OutputCamsEn::CAM_L;
			} else if (gRequUriQuery.compare("R") == 0) {
				valOut = fcapconstants::OutputCamsEn::CAM_R;
			} else if (gRequUriQuery.compare("BOTH") == 0)  {
				valOut = fcapconstants::OutputCamsEn::CAM_BOTH;
			} else {
				throw std::exception();
			}
		} catch (std::exception &err) {
			gPHndCltData->respErrMsg = "invalid output camera (allowed: 'L', 'R', 'BOTH')";
			resB = false;
		}
		return resB;
	}

	void HandleRouteGet::_stringSplit(const std::string &valIn, const std::string &split, std::string &valOut1, std::string &valOut2) {
		if (valIn.empty()) {
			throw std::exception();
		}
		size_t posIx = valIn.find(split);
		if (posIx == std::string::npos) {
			throw std::exception();
		}
		valOut1 = valIn.substr(0, posIx);
		valOut2 = valIn.substr(posIx + 1);
	}

	bool HandleRouteGet::getCoordsFromQuery(cv::Point &valOut, const cv::Point &valMin, const cv::Point &valMax) {
		bool resB = true;

		try {
			std::string strA1;
			std::string strA1k;
			std::string strA1v;
			std::string strA2;
			std::string strA2k;
			std::string strA2v;

			_stringSplit(gRequUriQuery, "&", strA1, strA2);
			_stringSplit(strA1, "=", strA1k, strA1v);
			_stringSplit(strA2, "=", strA2k, strA2v);
			if (strA1k.compare("x") == 0) {
				valOut.x = stoi(strA1v);
			} else if (strA1k.compare("y") == 0) {
				valOut.y = stoi(strA1v);
			} else {
				throw std::exception();
			}
			if (strA2k.compare("x") == 0) {
				valOut.x = stoi(strA2v);
			} else if (strA2k.compare("y") == 0) {
				valOut.y = stoi(strA2v);
			} else {
				throw std::exception();
			}
			if (valOut.x < valMin.x || valOut.x > valMax.x) {
				throw std::exception();
			}
			if (valOut.y < valMin.y || valOut.y > valMax.y) {
				throw std::exception();
			}
		} catch (std::exception &err) {
			gPHndCltData->respErrMsg = "invalid coordinates (example: 'x=1&y=2')";
			resB = false;
		}
		return resB;
	}

	// -----------------------------------------------------------------------------

	std::string HandleRouteGet::buildWebsite() {
		std::ostringstream resS;

		resS
				<< "<!DOCTYPE html>"
				<< "<html lang=\"en\">"
				<< "<head>"
					<< "<title>" << fcapconstants::HTTP_SERVER_NAME << "</title>"
				<< "</head>"
				<< "<body>"
					<< "<h1>" << fcapconstants::HTTP_SERVER_NAME << "</h1>"
					<< "<p>"
						<< "<a href='" << URL_PATH_STREAM << "'>MJPEG Stream</a>"
					<< "</p>";
		if (gPHndCltData->rtOptsCur.outputCams == fcapconstants::OutputCamsEn::CAM_R) {
			resS
					<< "<p>"
						<< "<a href='" << URL_PATH_OUTPUT_CAMS_ENABLE << "?L'>Enable left camera</a>"
					<< "</p>";
		} else {
			resS
					<< "<p>"
						<< "<a href='" << URL_PATH_OUTPUT_CAMS_DISABLE << "?L'>Disable left camera</a>"
					<< "</p>";
		}
		if (gPHndCltData->rtOptsCur.outputCams == fcapconstants::OutputCamsEn::CAM_L) {
			resS
					<< "<p>"
						<< "<a href='" << URL_PATH_OUTPUT_CAMS_ENABLE << "?R'>Enable right camera</a>"
					<< "</p>";
		} else {
			resS
					<< "<p>"
						<< "<a href='" << URL_PATH_OUTPUT_CAMS_DISABLE << "?R'>Disable right camera</a>"
					<< "</p>";
		}
		resS
				<< "<p>"
					<< "<a href='" << URL_PATH_OUTPUT_CAMS_SWAP << "'>Swap cameras</a>"
				<< "</p>";
		resS
					<< "<div style='margin-top:20px'>"
						<< "<img src='" << URL_PATH_STREAM << "' width='800' height='450' />"
					<< "</div>"
				<< "</body></html>";
		return resS.str();
	}

}  // namespace http
