#ifndef SETTINGS_HPP_
#define SETTINGS_HPP_

#include <string>
#include <opencv2/opencv.hpp>

#include "constants.hpp"

namespace fcapsettings {

	const uint16_t SETT_DEFAULT_SERVER_PORT = 8090;

	const uint8_t SETT_JPEG_QUALITY = 90;  // 0..100

	const uint8_t SETT_DEFAULT_FPS = 15;

	const cv::Size SETT_DEFAULT_CAPTURE_SZ = fcapconstants::PIPE_CAPTURE_SZ_1536X864;  // can also be a PIPE_OUTPUT_SZ_*
	const cv::Size SETT_DEFAULT_OUTPUT_SZ = fcapconstants::PIPE_OUTPUT_SZ_1280X720;  // can also be a PIPE_CAPTURE_SZ_*

	const bool SETT_OPEN_CAM_STREAMS = true;  // for debugging only

	//
	const uint8_t SETT_PIPE_FMT1 = fcapconstants::PIPE_FMT_X_BGRX;
	const uint8_t SETT_PIPE_FMT2 = fcapconstants::PIPE_FMT_X_BGR;

	const uint32_t SETT_MAX_STREAMING_CLIENTS = 4;

}  // namespace fcapsettings

#endif  // SETTINGS_HPP_
