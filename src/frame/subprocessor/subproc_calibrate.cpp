#include <stdio.h>

#include "../../settings.hpp"
#include "subproc.hpp"

namespace framesubproc {

	FrameSubProcessorCalibrate::FrameSubProcessorCalibrate() :
			FrameSubProcessor(),
			gCalibrated(false),
			gTries(0) {
	}

	void FrameSubProcessorCalibrate::processFrame(cv::Mat &frame) {
		if (gCalibrated || gTries == MAX_TRIES) {
			return;
		}
		cv::cvtColor(frame, frame, cv::COLOR_BGR2GRAY);
		//
		std::vector<cv::Point2f> corners;
		bool patternfound = cv::findChessboardCorners(
				frame,
				cv::Size(fcapsettings::CALIB_CHESS_SQUARES_CNT_X, fcapsettings::CALIB_CHESS_SQUARES_CNT_Y),
				corners
			);
		if (patternfound) {
			log("found chessboard pattern");
		}
		//
		//++gTries;
	}

}  // namespace framesubproc
