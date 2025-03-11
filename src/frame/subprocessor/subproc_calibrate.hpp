#ifndef SUBPROC_CALIBRATE_HPP_
#define SUBPROC_CALIBRATE_HPP_

#include "subproc.hpp"

namespace framesubproc {

	enum class PatternEn {
		CHESSBOARD = 0,
		CHARUCOBOARD = 1,
		CIRCLES_GRID = 2,
		ASYMMETRIC_CIRCLES_GRID = 3
	};

	struct OcvSettingsStc {
		bool useFisheye;
		int flags;
		double aspectRatio;
		PatternEn patternEn;
		cv::Size boardSize;
		float gridWidth;

		OcvSettingsStc() {
			useFisheye = false;
			flags = 0;
			aspectRatio = 16.0 / 9.0;
			patternEn = PatternEn::CHESSBOARD;
			boardSize = cv::Size(fcapsettings::PROC_CAL_CHESS_SQUARES_INNERCORNERS_ROW, fcapsettings::PROC_CAL_CHESS_SQUARES_INNERCORNERS_COL);
			if (patternEn == PatternEn::CHARUCOBOARD) {
				gridWidth = fcapsettings::PROC_CAL_CHESS_SQUARES_WIDTH_MM * static_cast<float>(boardSize.width - 2);
			} else {
				gridWidth = fcapsettings::PROC_CAL_CHESS_SQUARES_WIDTH_MM * static_cast<float>(boardSize.width - 1);
			}
		}
	};

	struct CalibrationDataStc {
		cv::Mat cameraMatrix;
		cv::Mat distCoeffs;
		cv::Mat fisheyeNewCamMat;
		std::vector<std::vector<cv::Point2f>> imagePoints;

		void reset() {
			cameraMatrix = cv::Mat();
			distCoeffs = cv::Mat();
			fisheyeNewCamMat = cv::Mat();
			imagePoints = std::vector<std::vector<cv::Point2f>>();
		}
	};

	class FrameSubProcessorCalibrate final : public FrameSubProcessor {
		public:
			FrameSubProcessorCalibrate();
			void setShowCalibChessboardPoints(bool val);
			bool getIsCalibrated() const;
			std::vector<cv::Point> getRectCorners();
			void loadData();
			void resetData();
			void processFrame(cv::Mat &frame, uint32_t frameNr) override;
		
		private:
			const uint8_t MAX_TRIES = 50;
			bool gCalibrated;
			bool gWriteToFileFailed;
			bool gLoadedFromFile;
			bool gLoadFromFileFailed;
			bool gOptShowCalibChessboardPoints;
			uint8_t gTries;
			OcvSettingsStc gOcvSettingsStc;
			CalibrationDataStc gCalibrationDataStc;

			//

			void findCorners(cv::Mat &frame, bool &foundCorners, std::vector<cv::Point2f> &corners);
			bool calibrate(cv::Mat &frame);
			void renderUndistorted(cv::Mat &frame) const;
			bool loadCalibrationDataFromFile();
			void deleteCalibrationDataFile();
			//
			bool _calibrateAndSaveToFile(
					OcvSettingsStc &s, cv::Size imageSize, cv::Mat &outpCameraMatrix, cv::Mat &outpDistCoeffs,
					std::vector<std::vector<cv::Point2f>> imagePoints);
			bool _ocvRunCalibration(
					OcvSettingsStc &s, cv::Size &imageSize, cv::Mat &outpCameraMatrix, cv::Mat &outpDistCoeffs,
					std::vector<std::vector<cv::Point2f>> imagePoints, std::vector<cv::Mat> &outpRvecs, std::vector<cv::Mat> &outpTvecs,
					std::vector<float> &outpReprojErrs, double &outpTotalAvgErr, std::vector<cv::Point3f> &outpNewObjPoints,
					bool doReleaseObject);
			void _ocvCalcBoardCornerPositions(
					float squareSize, std::vector<cv::Point3f> &corners, PatternEn patternType) const;
			static double _ocvComputeReprojectionErrors(
					const std::vector<std::vector<cv::Point3f>> &objectPoints,
					const std::vector<std::vector<cv::Point2f>> &imagePoints,
					const std::vector<cv::Mat> &rvecs, const std::vector<cv::Mat> &tvecs,
					const cv::Mat &cameraMatrix, const cv::Mat &distCoeffs,
					std::vector<float> &outpPerViewErrors, bool fisheye);
			//
			void _saveCalibrationDataToFile();
			void _writeFlagFromCalibrationDataFile(cv::FileStorage &fs, const std::string &flagName, int flagValue) const;
			void _readFlagFromCalibrationDataFile(cv::FileStorage &fs, const std::string &flagName, int flagValue);
	};

}  // namespace framesubproc

#endif  // SUBPROC_CALIBRATE_HPP_
