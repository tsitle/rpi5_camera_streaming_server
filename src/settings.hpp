#ifndef SETTINGS_HPP_
#define SETTINGS_HPP_

#include <string>
#include <opencv2/opencv.hpp>

#include "constants.hpp"

namespace fcapsettings {

	const uint16_t DEFAULT_SERVER_PORT = 8090;

	const uint8_t JPEG_QUALITY = 90;  // 0..100

	const uint8_t DEFAULT_FPS = 15;
	const bool DEFAULT_ENABLE_ADAPTIVE_FPS = true;

	const cv::Size DEFAULT_CAPTURE_SZ = fcapconstants::PIPE_CAPTURE_SZ_1536X864;  // can also be a PIPE_OUTPUT_SZ_*
	const cv::Size DEFAULT_INPUT_SZ = fcapconstants::PIPE_OUTPUT_SZ_1280X720;  // can also be a PIPE_CAPTURE_SZ_*

	const bool DBG_OPEN_CAM_STREAMS = true;  // for debugging only

	const bool DEFAULT_OUTPUT_PNGS = false;

	//
	const uint8_t SETT_PIPE_FMT1 = fcapconstants::PIPE_FMT_X_BGRX;
	const uint8_t SETT_PIPE_FMT2 = fcapconstants::PIPE_FMT_X_BGR;

	const uint32_t SETT_MAX_STREAMING_CLIENTS = 4;

	const uint8_t QUEUE_SIZE = 5;  // higher values result in higher latency

	const bool SPLITVIEW_FOR_CAMBOTH = true;  // use split view instead of blended view when both cameras are enabled?

	//

	const uint8_t CALIB_CHESS_SQUARES_INNERCORNERS_COL = 4;  // since the image is rotated 90 degress when calibrating, columns and rows need to be swapped here
	const uint8_t CALIB_CHESS_SQUARES_INNERCORNERS_ROW = 6;
	const float CALIB_CHESS_SQUARES_WIDTH_MM = 5.0;  // mm
	const double CALIB_MAX_PROJECTION_ERROR = 0.6;

	const bool PROC_DISABLE_ALL_PROCESSING = false;

	const int16_t PROC_BNC_DEFAULT_ADJ_BRIGHTNESS = 23;
	const int16_t PROC_BNC_DEFAULT_ADJ_CONTRAST = 5;

	const bool PROC_CAL_DEFAULT_SHOWCALIBCHESSPOINTS = false;
	const bool PROC_CAL_UNDISTORT = false;  // roughly doubles the CPU load if enabled

}  // namespace fcapsettings

#endif  // SETTINGS_HPP_
