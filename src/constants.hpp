#ifndef CONSTANTS_HPP_
#define CONSTANTS_HPP_

#include <string>
#include <opencv2/opencv.hpp>

namespace fcapconstants {

	const std::string CONFIG_FILENAME = "config.json";

	const uint16_t IMAGE_SIZE_MAX = 4608;

	const cv::Size PIPE_CAPTURE_SZ_1536X864 = cv::Size(1536, 864);
	const cv::Size PIPE_CAPTURE_SZ_2304X1296 = cv::Size(2304, 1296);
	//const cv::Size PIPE_CAPTURE_SZ_4608X2592 = cv::Size(4608, 2592);  // too big for Compute Modul 4, two cameras and 1GB of RAM (ERROR V4L2 v4l2_videodevice.cpp:1248 /dev/video14[19:cap]: Not enough buffers provided by V4L2VideoDevice)

	const cv::Size PIPE_OUTPUT_SZ_640X360 = cv::Size(640, 360);
	const cv::Size PIPE_OUTPUT_SZ_800X450 = cv::Size(800, 450);
	const cv::Size PIPE_OUTPUT_SZ_1024X576 = cv::Size(1024, 576);
	const cv::Size PIPE_OUTPUT_SZ_1280X720 = cv::Size(1280, 720);
	const cv::Size PIPE_OUTPUT_SZ_1920X1080 = cv::Size(1920, 1080);
	//const cv::Size PIPE_OUTPUT_SZ_2560x1440 = cv::Size(2560, 1440);  // too big for Compute Modul 4, two cameras and 1GB of RAM (ERROR V4L2 v4l2_videodevice.cpp:1248 /dev/video14[19:cap]: Not enough buffers provided by V4L2VideoDevice)

	const std::string PIPE_FMT_S_BGR("BGR");
	const std::string PIPE_FMT_S_BGRX("BGRx");

	const uint8_t PIPE_FMT_X_BGR = 0;
	const uint8_t PIPE_FMT_X_BGRX = 1;

	const std::string HTTP_CONTENT_TYPE_HTML = "text/html";
	const std::string HTTP_CONTENT_TYPE_JPEG = "image/jpeg";
	const std::string HTTP_CONTENT_TYPE_MULTIPART = "multipart/x-mixed-replace";
	const std::string HTTP_CONTENT_TYPE_JSON = "application/json";

	const std::string HTTP_BOUNDARY_SEPARATOR = "--FrameBoundary";

	enum class OutputCamsEn {
		CAM_L = 0,
		CAM_R = 1,
		CAM_BOTH = 2
	};

	enum class CamSourceEn {
		GSTREAMER = 0,
		MJPEG = 1,
		UNSPECIFIED = 2
	};

	enum class CamIdEn {
		CAM_0 = 0,
		CAM_1 = 1
	};

	const std::string CONFFILE_CAMID_0 = "cam0";
	const std::string CONFFILE_CAMID_1 = "cam1";

	const std::string CONFFILE_CAMSRC_GSTR = "gstreamer";
	const std::string CONFFILE_CAMSRC_MJPEG = "mjpeg";
	const std::string CONFFILE_CAMSRC_UNSPEC = "unspecified";

	const int16_t PROC_MAX_ADJ_BRIGHTNESS = 254;
	const int16_t PROC_MIN_ADJ_BRIGHTNESS = -127;
	const int16_t PROC_MAX_ADJ_CONTRAST = 127;
	const int16_t PROC_MIN_ADJ_CONTRAST = -64;

}  // namespace fcapconstants

#endif  // CONSTANTS_HPP_
