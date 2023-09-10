#ifndef INCLUDED_SETTINGS
#define INCLUDED_SETTINGS

#include <string>
#include <opencv2/opencv.hpp>

#include "constants.hpp"

namespace fcapsettings {

	const unsigned int SETT_SERVER_PORT = 8090;

	const unsigned int SETT_JPEG_QUALITY = 70;  // 0..100
	const unsigned int SETT_FPS = 10;
	const std::string SETT_PNG_PATH = "/media/usbhd";

	const cv::Size SETT_CAPTURE_SZ = fcapconstants::PIPE_OUTPUT_SZ_1280X720;
	const cv::Size SETT_OUTPUT_SZ = fcapconstants::PIPE_OUTPUT_SZ_1280X720;

	const fcapconstants::OutputCamsEn SETT_OUTPUT_CAMS = fcapconstants::OutputCamsEn::CAM_BOTH;

	const bool SETT_WRITE_PNG_TO_FILE_L = false;
	const bool SETT_WRITE_PNG_TO_FILE_R = false;
	const bool SETT_WRITE_PNG_TO_FILE_BLEND = false;

	const bool SETT_OPEN_CAM_STREAMS = true;

	//
	const unsigned int SETT_PIPE_FMT1 = fcapconstants::PIPE_FMT_X_BGRX;
	const unsigned int SETT_PIPE_FMT2 = fcapconstants::PIPE_FMT_X_BGR;

	const unsigned int SETT_CAM_NR_LEFT = 1;
	const unsigned int SETT_CAM_NR_RIGHT = 0;

}  // namespace fcapsettings

#endif
