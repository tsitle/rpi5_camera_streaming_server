#ifndef SETTINGS_HPP_
#define SETTINGS_HPP_

#include <string>
#include <opencv2/opencv.hpp>

#include "constants.hpp"

namespace fcapsettings {

	const unsigned int SETT_SERVER_PORT = 8090;

	const unsigned int SETT_JPEG_QUALITY = 70;  // 0..100

	/**
	 * Single Camera:                Dual Cameras:
	 *   57 for Capt= 640,Outp= 640
	 *   50 for Capt=1536,Outp= 640    34
	 *   45 for Capt=1536,Outp= 800
	 *   26 for Capt=2304,Outp= 640
	 *   23 for Capt=1536,Outp=1280
	 *   16 for Capt=1280,Outp=1280
	 *   10 for Capt=2304,Outp=1296
	 *    7 for Capt=2304,Outp=2304
	 */
	const unsigned int SETT_FPS = 23;

	const std::string SETT_PNG_PATH = "/media/usbhd";

	const cv::Size SETT_CAPTURE_SZ = fcapconstants::PIPE_CAPTURE_SZ_1536X864;  // can also be a PIPE_OUTPUT_SZ_*
	const cv::Size SETT_OUTPUT_SZ = fcapconstants::PIPE_OUTPUT_SZ_1280X720;  // can also be a PIPE_CAPTURE_SZ_*

	const bool SETT_WRITE_PNG_TO_FILE_L = false;
	const bool SETT_WRITE_PNG_TO_FILE_R = false;
	const bool SETT_WRITE_PNG_TO_FILE_BLEND = false;

	const bool SETT_OPEN_CAM_STREAMS = true;

	//
	const unsigned int SETT_PIPE_FMT1 = fcapconstants::PIPE_FMT_X_BGRX;
	const unsigned int SETT_PIPE_FMT2 = fcapconstants::PIPE_FMT_X_BGR;

	const unsigned int SETT_CAM_NR_LEFT = 1;
	const unsigned int SETT_CAM_NR_RIGHT = 0;

	const uint32_t SETT_MAX_STREAMING_CLIENTS = 4;

}  // namespace fcapsettings

#endif  // SETTINGS_HPP_
