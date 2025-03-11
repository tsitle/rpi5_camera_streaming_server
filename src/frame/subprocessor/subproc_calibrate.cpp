#include "../../settings.hpp"
#include "../../shared.hpp"
#include "subproc_calibrate.hpp"

#include <sys/stat.h>

namespace framesubproc {

	FrameSubProcessorCalibrate::FrameSubProcessorCalibrate() :
			FrameSubProcessor("CAL"),
			gCalibrated(false),
			gWriteToFileFailed(false),
			gLoadedFromFile(false),
			gLoadFromFileFailed(false),
			gOptShowCalibChessboardPoints(false),
			gTries(0) {
		gOcvSettingsStc.useFisheye = false;
		if (gOcvSettingsStc.useFisheye) {
			/**
			 * ! untested !
			 */
			gOcvSettingsStc.flags |= cv::CALIB_FIX_S1_S2_S3_S4;
			gOcvSettingsStc.flags |= cv::CALIB_FIX_K1;
			//gOcvSettingsStc.flags |= cv::CALIB_FIX_K2;
			//gOcvSettingsStc.flags |= cv::CALIB_FIX_K3;
			//gOcvSettingsStc.flags |= cv::CALIB_FIX_K4;
		} else {
			/**
			 *  0.22  no flags -- image is terrible
			 *  0.29  CALIB_FIX_PRINCIPAL_POINT | CALIB_ZERO_TANGENT_DIST | CALIB_FIX_K1 -- promising
			 *  0.31  CALIB_FIX_PRINCIPAL_POINT | CALIB_ZERO_TANGENT_DIST | CALIB_FIX_K1 | CALIB_FIX_K2 -- promising
			 *  0.32  CALIB_FIX_PRINCIPAL_POINT | CALIB_ZERO_TANGENT_DIST | CALIB_FIX_K1 | CALIB_FIX_K2 | CALIB_FIX_K3 -- promising
			 *  0.28  CALIB_FIX_K1 -- promising
			 *  0.36  CALIB_FIX_K2 -- promising
			 *  0.25  CALIB_FIX_K3 -- promising
			 *  0.22  CALIB_FIX_K4 -- image is terrible
			 *  0.22  CALIB_FIX_K5 -- image is terrible
			 *  0.22  CALIB_FIX_K6 -- image is terrible
			 *  0.44  CALIB_USE_INTRINSIC_GUESS | CALIB_FIX_K1 -- promising
			 *  0.45  CALIB_FIX_PRINCIPAL_POINT | CALIB_FIX_K1 -- promising
			 *  0.45  CALIB_FIX_PRINCIPAL_POINT -- image is terrible
			 *  0.28  CALIB_ZERO_TANGENT_DIST | CALIB_FIX_K1 -- promising
			 *  0.31  CALIB_FIX_K1 | CALIB_FIX_K2 | CALIB_FIX_K3 -- promising
			 *  0.31  CALIB_FIX_PRINCIPAL_POINT | CALIB_FIX_K1 | CALIB_FIX_K2 | CALIB_FIX_K3 -- promising
			 *  2.20  CALIB_FIX_ASPECT_RATIO | x
			 *  0.81  CALIB_USE_INTRINSIC_GUESS | CALIB_FIX_PRINCIPAL_POINT | CALIB_FIX_K1
			 *  0.86  CALIB_USE_INTRINSIC_GUESS | CALIB_FIX_PRINCIPAL_POINT | CALIB_FIX_K3
			 */
			//gOcvSettingsStc.flags |= cv::CALIB_USE_INTRINSIC_GUESS;
			//gOcvSettingsStc.flags |= cv::CALIB_FIX_ASPECT_RATIO;
			gOcvSettingsStc.flags |= cv::CALIB_FIX_PRINCIPAL_POINT;
			gOcvSettingsStc.flags |= cv::CALIB_ZERO_TANGENT_DIST;
			gOcvSettingsStc.flags |= cv::CALIB_FIX_K1;
			gOcvSettingsStc.flags |= cv::CALIB_FIX_K2;
			gOcvSettingsStc.flags |= cv::CALIB_FIX_K3;
			//gOcvSettingsStc.flags |= cv::CALIB_FIX_K4;
			//gOcvSettingsStc.flags |= cv::CALIB_FIX_K5;
			//gOcvSettingsStc.flags |= cv::CALIB_FIX_K6;
		}
	}

	void FrameSubProcessorCalibrate::setShowCalibChessboardPoints(const bool val) {
		gOptShowCalibChessboardPoints = val;
	}

	bool FrameSubProcessorCalibrate::getIsCalibrated() const {
		return gCalibrated;
	}

	std::vector<cv::Point> FrameSubProcessorCalibrate::getRectCorners() {
		std::vector<cv::Point> resVect;

		if (! gCalibrated) {
			log("not calibrated");
			return resVect;
		}
		if (gCalibrationDataStc.imagePoints.empty()) {
			log("missing imagePoints");
			return resVect;
		}

		constexpr uint8_t rowCnt = fcapsettings::PROC_CAL_CHESS_SQUARES_INNERCORNERS_ROW;
		constexpr uint8_t colCnt = fcapsettings::PROC_CAL_CHESS_SQUARES_INNERCORNERS_COL;
		const std::vector<cv::Point2f> lastPnts = gCalibrationDataStc.imagePoints.back();

		if (lastPnts.size() < rowCnt * colCnt) {
			log("invalid vector size (is:" +
					std::to_string(lastPnts.size()) +
					", exp=" +
					std::to_string(rowCnt * colCnt) + ")");
			return resVect;
		}
		cv::Point tmpPnt1 = cv::Point(
				(int32_t)lastPnts[0].x,
				(int32_t)lastPnts[0].y
			);
		cv::Point tmpPnt2 = cv::Point(
				(int32_t)lastPnts[rowCnt * (colCnt - 1)].x,
				(int32_t)lastPnts[rowCnt * (colCnt - 1)].y
			);
		cv::Point tmpPnt3 = cv::Point(
				(int32_t)lastPnts[rowCnt - 1].x,
				(int32_t)lastPnts[rowCnt - 1].y
			);
		cv::Point tmpPnt4 = cv::Point(
				(int32_t)lastPnts[(rowCnt * colCnt) - 1].x,
				(int32_t)lastPnts[(rowCnt * colCnt) - 1].y
			);
		resVect.push_back(tmpPnt1);
		resVect.push_back(tmpPnt2);
		resVect.push_back(tmpPnt3);
		resVect.push_back(tmpPnt4);
		return resVect;
	}

	void FrameSubProcessorCalibrate::resetData() {
		gCalibrated = false;
		if (gLoadedFromFile) {
			deleteCalibrationDataFile();
		} else {
			gCalibrationDataStc.reset();
		}
		gTries = 0;
	}

	void FrameSubProcessorCalibrate::loadData() {
		gLoadedFromFile = loadCalibrationDataFromFile();
		if (gLoadedFromFile) {
			gCalibrated = true;
		}
	}

	void FrameSubProcessorCalibrate::processFrame(cv::Mat &frame, const uint32_t frameNr) {
		if (gTries == MAX_TRIES || gWriteToFileFailed) {
			/**if (gTries == MAX_TRIES) {
				log("exit cal - MAXTRIES");
			} else {
				log("exit cal - gWriteToFileFailed");
			}**/
			return;
		}
		//
		gFrameNr = frameNr;
		//
		if (! gCalibrated) {
			std::vector<cv::Point2f> corners;
			bool foundCorners = false;

			// find the corners
			findCorners(frame, foundCorners, corners);
			if (foundCorners) {
				gCalibrationDataStc.imagePoints.push_back(corners);
				// draw the corners
				cv::drawChessboardCorners(frame, gOcvSettingsStc.boardSize, corners, true);
				//
				if (gCalibrationDataStc.imagePoints.size() >= 10) {
					gCalibrated = calibrate(frame);
					if (! gCalibrated && gCalibrationDataStc.imagePoints.size() >= 30 && ! gWriteToFileFailed) {
						log("giving up trying to calibrate camera");
						gTries = MAX_TRIES;
					}
				}
			}
		} else {
			// draw the last found corners
			if (gOptShowCalibChessboardPoints && ! gCalibrationDataStc.imagePoints.empty()) {
				cv::drawChessboardCorners(
						frame,
						gOcvSettingsStc.boardSize,
						gCalibrationDataStc.imagePoints.back(),
						true
					);
			}
			//
			if (fcapsettings::PROC_CAL_UNDISTORT) {
				renderUndistorted(frame);
			}
		}
	}

	// -----------------------------------------------------------------------------
	// -----------------------------------------------------------------------------

	void FrameSubProcessorCalibrate::findCorners(cv::Mat &frame, bool &foundCorners, std::vector<cv::Point2f> &corners) {
		cv::cvtColor(frame, frame, cv::COLOR_BGR2GRAY);
		//
		foundCorners = cv::findChessboardCorners(
				frame,
				gOcvSettingsStc.boardSize,
				corners,
				cv::CALIB_CB_ADAPTIVE_THRESH + cv::CALIB_CB_NORMALIZE_IMAGE + cv::CALIB_CB_FAST_CHECK
			);
		if (foundCorners) {
			/**log("found chessboard pattern");**/
			cv::cornerSubPix(
					frame,
					corners,
					cv::Size(11, 11),
					cv::Size(-1, -1),
					cv::TermCriteria(cv::TermCriteria::EPS + cv::TermCriteria::MAX_ITER, 30, 0.1)
				);
		}
		//
		if (! foundCorners && ++gTries == MAX_TRIES) {
			log("giving up trying to find chessboard pattern");
		}
	}

	bool FrameSubProcessorCalibrate::calibrate(cv::Mat &frame) {
		bool resB;
		cv::Mat outpCameraMatrix, outpDistCoeffs;

		/**log("_calibrateAndSaveToFile");**/
		resB = _calibrateAndSaveToFile(
				gOcvSettingsStc,
				frame.size(),
				outpCameraMatrix,
				outpDistCoeffs,
				gCalibrationDataStc.imagePoints
			);
		if (! resB) {
			log("calibration failed");
			return resB;
		}
		return resB;
	}

	void FrameSubProcessorCalibrate::renderUndistorted(cv::Mat &frame) const {
		const cv::Mat frameOrg = frame.clone();
		frame = cv::Mat();
		if (gOcvSettingsStc.useFisheye) {
			/**log("render undistorted fisheye");**/
			cv::fisheye::undistortImage(
					frameOrg,
					frame,
					gCalibrationDataStc.cameraMatrix,
					gCalibrationDataStc.distCoeffs,
					gCalibrationDataStc.fisheyeNewCamMat
				);
		} else {
			/**log("render undistorted default");**/
			cv::undistort(
					frameOrg,
					frame,
					gCalibrationDataStc.cameraMatrix,
					gCalibrationDataStc.distCoeffs
				);
		}
	}

	bool FrameSubProcessorCalibrate::loadCalibrationDataFromFile() {
		if (gLoadedFromFile || gLoadFromFileFailed) {
			return false;
		}

		std::string inpFn = buildDataFilename("");

		if (! fcapshared::Shared::fileExists(inpFn)) {
			gLoadFromFileFailed = true;
			return false;
		}

		log("loading Calibration data from file '" + inpFn + "'");

		cv::FileStorage fs(inpFn, cv::FileStorage::READ | cv::FileStorage::FORMAT_YAML);
		int cfileInt;

		//
		gLoadFromFileFailed = (! loadDataFromFile_header(fs));
		if (gLoadFromFileFailed) {
			return false;
		}

		//
		///
		fs["ocvSettings_useFisheye"] >> cfileInt;
		gOcvSettingsStc.useFisheye = (cfileInt == 1);
		///
		gOcvSettingsStc.flags = 0;
		if (gOcvSettingsStc.useFisheye) {
			_readFlagFromCalibrationDataFile(fs, "ocvSettings_fisheye_flags_CALIB_FIX_S1_S2_S3_S4", cv::CALIB_FIX_S1_S2_S3_S4);
			_readFlagFromCalibrationDataFile(fs, "ocvSettings_fisheye_flags_CALIB_FIX_K1", cv::CALIB_FIX_K1);
			_readFlagFromCalibrationDataFile(fs, "ocvSettings_fisheye_flags_CALIB_FIX_K2", cv::CALIB_FIX_K2);
			_readFlagFromCalibrationDataFile(fs, "ocvSettings_fisheye_flags_CALIB_FIX_K3", cv::CALIB_FIX_K3);
			_readFlagFromCalibrationDataFile(fs, "ocvSettings_fisheye_flags_CALIB_FIX_K4", cv::CALIB_FIX_K4);
		} else {
			_readFlagFromCalibrationDataFile(fs, "ocvSettings_default_flags_CALIB_USE_INTRINSIC_GUESS", cv::CALIB_USE_INTRINSIC_GUESS);
			_readFlagFromCalibrationDataFile(fs, "ocvSettings_default_flags_CALIB_FIX_ASPECT_RATIO", cv::CALIB_FIX_ASPECT_RATIO);
			_readFlagFromCalibrationDataFile(fs, "ocvSettings_default_flags_CALIB_FIX_PRINCIPAL_POINT", cv::CALIB_FIX_PRINCIPAL_POINT);
			_readFlagFromCalibrationDataFile(fs, "ocvSettings_default_flags_CALIB_ZERO_TANGENT_DIST", cv::CALIB_ZERO_TANGENT_DIST);
			_readFlagFromCalibrationDataFile(fs, "ocvSettings_default_flags_CALIB_FIX_K1", cv::CALIB_FIX_K1);
			_readFlagFromCalibrationDataFile(fs, "ocvSettings_default_flags_CALIB_FIX_K2", cv::CALIB_FIX_K2);
			_readFlagFromCalibrationDataFile(fs, "ocvSettings_default_flags_CALIB_FIX_K3", cv::CALIB_FIX_K3);
			_readFlagFromCalibrationDataFile(fs, "ocvSettings_default_flags_CALIB_FIX_K4", cv::CALIB_FIX_K4);
			_readFlagFromCalibrationDataFile(fs, "ocvSettings_default_flags_CALIB_FIX_K5", cv::CALIB_FIX_K5);
			_readFlagFromCalibrationDataFile(fs, "ocvSettings_default_flags_CALIB_FIX_K6", cv::CALIB_FIX_K6);
		}
		///
		fs["ocvSettings_aspectRatio"] >> gOcvSettingsStc.aspectRatio;
		fs["ocvSettings_patternEn"] >> cfileInt;
		gOcvSettingsStc.patternEn = (PatternEn)cfileInt;
		fs["ocvSettings_boardSize"] >> gOcvSettingsStc.boardSize;
		fs["ocvSettings_gridWidth"] >> gOcvSettingsStc.gridWidth;
		//
		fs["calib_cameraMatrix"] >> gCalibrationDataStc.cameraMatrix;
		fs["calib_distCoeffs"] >> gCalibrationDataStc.distCoeffs;
		if (gOcvSettingsStc.useFisheye) {
			fs["calib_fisheyeNewCamMat"] >> gCalibrationDataStc.fisheyeNewCamMat;
		}
		fs["calib_imagePoints"] >> gCalibrationDataStc.imagePoints;

		log("__reading done");
		return true;
	}

	void FrameSubProcessorCalibrate::deleteCalibrationDataFile() {
		const std::string outpFn = buildDataFilename("");

		deleteDataFile(outpFn);
		gLoadedFromFile = false;
		gCalibrationDataStc.reset();
	}

	// -----------------------------------------------------------------------------

	bool FrameSubProcessorCalibrate::_calibrateAndSaveToFile(
			OcvSettingsStc &s, cv::Size imageSize, cv::Mat &outpCameraMatrix, cv::Mat &outpDistCoeffs,
			std::vector<std::vector<cv::Point2f>> imagePoints) {
		const bool _DO_RELEASE_OBJECT = true;
		bool resB;
		std::vector<cv::Mat> outpRvecs, outpTvecs;
		std::vector<float> outpReprojErrs;
		double outpTotalAvgErr = 0.0;
		std::vector<cv::Point3f> outpNewObjPoints;

		resB = _ocvRunCalibration(
				s,
				imageSize,
				outpCameraMatrix,
				outpDistCoeffs,
				imagePoints,
				outpRvecs,
				outpTvecs,
				outpReprojErrs,
				outpTotalAvgErr,
				outpNewObjPoints,
				_DO_RELEASE_OBJECT
			);
		if (resB && outpTotalAvgErr >= fcapsettings::PROC_CAL_MAX_PROJECTION_ERROR) {
			log("avg re projection error too high (is=" + std::to_string(outpTotalAvgErr) +
					", max=" + std::to_string(fcapsettings::PROC_CAL_MAX_PROJECTION_ERROR) + ")");
			resB = false;
		}

		if (resB) {
			gCalibrationDataStc.cameraMatrix = outpCameraMatrix;
			gCalibrationDataStc.distCoeffs = outpDistCoeffs;
			//
			if (gOcvSettingsStc.useFisheye) {
				// get fisheyeNewCamMat
				cv::fisheye::estimateNewCameraMatrixForUndistortRectify(
						outpCameraMatrix,
						outpDistCoeffs,
						imageSize,
						cv::Matx33d::eye(),
						gCalibrationDataStc.fisheyeNewCamMat,
						1
					);
			}
			//
			log("Calibration succeeded, avg re projection error=" + std::to_string(outpTotalAvgErr));
		}

		/**if (resB) {
			std::cout << "New board corners: " << std::endl;
			std::cout << outpNewObjPoints[0] << std::endl;
			std::cout << outpNewObjPoints[gOcvSettingsStc.boardSize.width - 1] << std::endl;
			std::cout << outpNewObjPoints[gOcvSettingsStc.boardSize.width * (gOcvSettingsStc.boardSize.height - 1)] << std::endl;
			std::cout << outpNewObjPoints.back() << std::endl;
		}**/

		// save Calibration data to file
		if (resB) {
			_saveCalibrationDataToFile();
			resB = (! gWriteToFileFailed);
		}
		return resB;
	}

	bool FrameSubProcessorCalibrate::_ocvRunCalibration(
			OcvSettingsStc &s, cv::Size &imageSize, cv::Mat &outpCameraMatrix, cv::Mat &outpDistCoeffs,
			std::vector<std::vector<cv::Point2f>> imagePoints, std::vector<cv::Mat> &outpRvecs, std::vector<cv::Mat> &outpTvecs,
			std::vector<float> &outpReprojErrs, double &outpTotalAvgErr, std::vector<cv::Point3f> &outpNewObjPoints,
			bool doReleaseObject) {
		//! [fixed_aspect]
		outpCameraMatrix = cv::Mat::eye(3, 3, CV_64F);
		if (! s.useFisheye && (s.flags & cv::CALIB_FIX_ASPECT_RATIO)) {
			outpCameraMatrix.at<double>(0,0) = s.aspectRatio;
		}
		//! [fixed_aspect]
		if (s.useFisheye) {
			outpDistCoeffs = cv::Mat::zeros(4, 1, CV_64F);
		} else {
			outpDistCoeffs = cv::Mat::zeros(8, 1, CV_64F);
		}

		/**log("_ocvCalcBoardCornerPositions");**/
		std::vector<std::vector<cv::Point3f>> objectPoints(1);
		_ocvCalcBoardCornerPositions(
				fcapsettings::PROC_CAL_CHESS_SQUARES_WIDTH_MM,
				objectPoints[0],
				s.patternEn
			);
		if (s.patternEn == PatternEn::CHARUCOBOARD) {
			objectPoints[0][gOcvSettingsStc.boardSize.width - 2].x = objectPoints[0][0].x + gOcvSettingsStc.gridWidth;
		} else {
			objectPoints[0][gOcvSettingsStc.boardSize.width - 1].x = objectPoints[0][0].x + gOcvSettingsStc.gridWidth;
		}
		outpNewObjPoints = objectPoints[0];

		objectPoints.resize(imagePoints.size(), objectPoints[0]);

		// Find intrinsic and extrinsic camera parameters
		/**double rms;**/

		if (s.useFisheye) {
			/**log("cv::fisheye::calibrate");**/
			cv::Mat _rvecs, _tvecs;
			/**rms =**/ cv::fisheye::calibrate(
					objectPoints,
					imagePoints,
					imageSize,
					outpCameraMatrix,
					outpDistCoeffs,
					_rvecs,
					_tvecs,
					s.flags
				);

			outpRvecs.reserve(_rvecs.rows);
			outpTvecs.reserve(_tvecs.rows);
			for (int i = 0; i < int(objectPoints.size()); i++) {
				outpRvecs.push_back(_rvecs.row(i));
				outpTvecs.push_back(_tvecs.row(i));
			}
		} else {
			/**log("cv::calibrateCameraRO");**/
			int iFixedPoint = -1;
			if (doReleaseObject) {
				iFixedPoint = gOcvSettingsStc.boardSize.width - 1;
			}
			/**rms =**/ cv::calibrateCameraRO(
					objectPoints,
					imagePoints,
					imageSize,
					iFixedPoint,
					outpCameraMatrix,
					outpDistCoeffs,
					outpRvecs,
					outpTvecs,
					outpNewObjPoints,
					s.flags | cv::CALIB_USE_LU
				);
		}

		/**log("Re-projection error reported by calibrateCamera: " + std::to_string(rms));**/

		bool ok = (cv::checkRange(outpCameraMatrix) && cv::checkRange(outpDistCoeffs));

		/**log("_ocvComputeReprojectionErrors");**/
		objectPoints.clear();
		objectPoints.resize(imagePoints.size(), outpNewObjPoints);
		outpTotalAvgErr = _ocvComputeReprojectionErrors(
				objectPoints,
				imagePoints,
				outpRvecs,
				outpTvecs,
				outpCameraMatrix,
				outpDistCoeffs,
				outpReprojErrs,
				s.useFisheye
			);

		return ok;
	}

	void FrameSubProcessorCalibrate::_ocvCalcBoardCornerPositions(
			float squareSize, std::vector<cv::Point3f> &corners, PatternEn patternType) const {
		corners.clear();

		switch(patternType) {
			case PatternEn::CHESSBOARD:
			case PatternEn::CIRCLES_GRID:
				for (int i = 0; i < gOcvSettingsStc.boardSize.height; ++i) {
					for (int j = 0; j < gOcvSettingsStc.boardSize.width; ++j) {
						corners.push_back(cv::Point3f(j * squareSize, i * squareSize, 0));
					}
				}
				break;
			case PatternEn::CHARUCOBOARD:
				for (int i = 0; i < gOcvSettingsStc.boardSize.height - 1; ++i) {
					for (int j = 0; j < gOcvSettingsStc.boardSize.width - 1; ++j) {
						corners.push_back(cv::Point3f(j * squareSize, i * squareSize, 0));
					}
				}
				break;
			case PatternEn::ASYMMETRIC_CIRCLES_GRID:
				for (int i = 0; i < gOcvSettingsStc.boardSize.height; i++) {
					for (int j = 0; j < gOcvSettingsStc.boardSize.width; j++) {
						corners.push_back(cv::Point3f((2 * j + i % 2) * squareSize, i * squareSize, 0));
					}
				}
				break;
			default:
				break;
		}
	}

	double FrameSubProcessorCalibrate::_ocvComputeReprojectionErrors(
			const std::vector<std::vector<cv::Point3f>> &objectPoints,
			const std::vector<std::vector<cv::Point2f>> &imagePoints,
			const std::vector<cv::Mat> &rvecs, const std::vector<cv::Mat> &tvecs,
			const cv::Mat &cameraMatrix, const cv::Mat &distCoeffs,
			std::vector<float> &outpPerViewErrors, bool fisheye) {
		std::vector<cv::Point2f> imagePoints2;
		size_t totalPoints = 0;
		double totalErr = 0;
		outpPerViewErrors.resize(objectPoints.size());

		for (size_t i = 0; i < objectPoints.size(); ++i) {
			if (fisheye) {
				cv::fisheye::projectPoints(
						objectPoints[i],
						imagePoints2,
						rvecs[i],
						tvecs[i],
						cameraMatrix,
						distCoeffs
					);
			} else {
				cv::projectPoints(objectPoints[i], rvecs[i], tvecs[i], cameraMatrix, distCoeffs, imagePoints2);
			}

			const double err = cv::norm(imagePoints[i], imagePoints2, cv::NORM_L2);

			size_t n = objectPoints[i].size();
			outpPerViewErrors[i] = static_cast<float>(std::sqrt(err * err / static_cast<double>(n)));
			totalErr += err * err;
			totalPoints += n;
		}

		return std::sqrt(totalErr / static_cast<double>(totalPoints));
	}

	// -----------------------------------------------------------------------------

	void FrameSubProcessorCalibrate::_saveCalibrationDataToFile() {
		if (gWriteToFileFailed) {
			return;
		}

		std::string outpFn = buildDataFilename("");

		log("writing Calibration data to file '" + outpFn + "'");
		cv::FileStorage fs(outpFn, cv::FileStorage::WRITE | cv::FileStorage::FORMAT_YAML);

		//
		saveDataToFile_header(fs);

		//
		fs << "ocvSettings_useFisheye" << gOcvSettingsStc.useFisheye;
		if (gOcvSettingsStc.useFisheye) {
			_writeFlagFromCalibrationDataFile(fs, "ocvSettings_fisheye_flags_CALIB_FIX_S1_S2_S3_S4", cv::CALIB_FIX_S1_S2_S3_S4);
			_writeFlagFromCalibrationDataFile(fs, "ocvSettings_fisheye_flags_CALIB_FIX_K1", cv::CALIB_FIX_K1);
			_writeFlagFromCalibrationDataFile(fs, "ocvSettings_fisheye_flags_CALIB_FIX_K2", cv::CALIB_FIX_K2);
			_writeFlagFromCalibrationDataFile(fs, "ocvSettings_fisheye_flags_CALIB_FIX_K3", cv::CALIB_FIX_K3);
			_writeFlagFromCalibrationDataFile(fs, "ocvSettings_fisheye_flags_CALIB_FIX_K4", cv::CALIB_FIX_K4);
		} else {
			_writeFlagFromCalibrationDataFile(fs, "ocvSettings_default_flags_CALIB_USE_INTRINSIC_GUESS", cv::CALIB_USE_INTRINSIC_GUESS);
			_writeFlagFromCalibrationDataFile(fs, "ocvSettings_default_flags_CALIB_FIX_ASPECT_RATIO", cv::CALIB_FIX_ASPECT_RATIO);
			_writeFlagFromCalibrationDataFile(fs, "ocvSettings_default_flags_CALIB_FIX_PRINCIPAL_POINT", cv::CALIB_FIX_PRINCIPAL_POINT);
			_writeFlagFromCalibrationDataFile(fs, "ocvSettings_default_flags_CALIB_ZERO_TANGENT_DIST", cv::CALIB_ZERO_TANGENT_DIST);
			_writeFlagFromCalibrationDataFile(fs, "ocvSettings_default_flags_CALIB_FIX_K1", cv::CALIB_FIX_K1);
			_writeFlagFromCalibrationDataFile(fs, "ocvSettings_default_flags_CALIB_FIX_K2", cv::CALIB_FIX_K2);
			_writeFlagFromCalibrationDataFile(fs, "ocvSettings_default_flags_CALIB_FIX_K3", cv::CALIB_FIX_K3);
			_writeFlagFromCalibrationDataFile(fs, "ocvSettings_default_flags_CALIB_FIX_K4", cv::CALIB_FIX_K4);
			_writeFlagFromCalibrationDataFile(fs, "ocvSettings_default_flags_CALIB_FIX_K5", cv::CALIB_FIX_K5);
			_writeFlagFromCalibrationDataFile(fs, "ocvSettings_default_flags_CALIB_FIX_K6", cv::CALIB_FIX_K6);
		}
		fs << "ocvSettings_aspectRatio" << gOcvSettingsStc.aspectRatio;
		fs << "ocvSettings_patternEn" << static_cast<int>(gOcvSettingsStc.patternEn);
		fs << "ocvSettings_boardSize" << gOcvSettingsStc.boardSize;
		fs << "ocvSettings_gridWidth" << gOcvSettingsStc.gridWidth;
		//
		fs << "calib_cameraMatrix" << gCalibrationDataStc.cameraMatrix;
		fs << "calib_distCoeffs" << gCalibrationDataStc.distCoeffs;
		if (gOcvSettingsStc.useFisheye) {
			fs << "calib_fisheyeNewCamMat" << gCalibrationDataStc.fisheyeNewCamMat;
		}
		fs << "calib_imagePoints" << gCalibrationDataStc.imagePoints;

		gWriteToFileFailed = (! fcapshared::Shared::fileExists(outpFn));
	}

	void FrameSubProcessorCalibrate::_writeFlagFromCalibrationDataFile(
			cv::FileStorage &fs, const std::string &flagName, const int flagValue) const {
		fs << flagName << ((gOcvSettingsStc.flags & flagValue) != 0);
	}

	void FrameSubProcessorCalibrate::_readFlagFromCalibrationDataFile(
			cv::FileStorage &fs, const std::string &flagName, const int flagValue) {
		bool cfileBool;

		fs[flagName] >> cfileBool;
		if (cfileBool) {
			gOcvSettingsStc.flags |= flagValue;
		}
	}

}  // namespace framesubproc
