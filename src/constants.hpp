#ifndef INCLUDED_CONSTANTS
#define INCLUDED_CONSTANTS

#include <string>
#include <opencv2/opencv.hpp>

namespace fcapconstants {

	const cv::Size PIPE_CAPTURE_SZ_1536X864 = cv::Size(1536, 864);
	const cv::Size PIPE_CAPTURE_SZ_2304X1296 = cv::Size(2304, 1296);
	const cv::Size PIPE_CAPTURE_SZ_4608X2592 = cv::Size(4608, 2592); // to big for Compuate Modul 4, two cameras and 1GB of RAM

	const cv::Size PIPE_OUTPUT_SZ_640X360 = cv::Size(640, 360);
	const cv::Size PIPE_OUTPUT_SZ_800X450 = cv::Size(800, 450);
	const cv::Size PIPE_OUTPUT_SZ_1024X576 = cv::Size(1024, 576);
	const cv::Size PIPE_OUTPUT_SZ_1280X720 = cv::Size(1280, 720);
	const cv::Size PIPE_OUTPUT_SZ_1920X1080 = cv::Size(1920, 1080);

	const std::string PIPE_FMT_S_BGR("BGR");
	const std::string PIPE_FMT_S_BGRX("BGRx");

	const unsigned int PIPE_FMT_X_BGR = 0;
	const unsigned int PIPE_FMT_X_BGRX = 1;

	// Pi Camera v3 with IMX708 sensor over I2C
	const std::string GSTREAMER_CAMNAME_ZERO("/base/soc/i2c0mux/i2c@0/imx708@1a");
	const std::string GSTREAMER_CAMNAME_ONE("/base/soc/i2c0mux/i2c@1/imx708@1a");

	const std::string HTTP_CONTENT_TYPE_HTML = "text/html";
	const std::string HTTP_CONTENT_TYPE_JPEG = "image/jpeg";
	const std::string HTTP_CONTENT_TYPE_MULTIPART = "multipart/x-mixed-replace";

	const std::string HTTP_BOUNDARY_SEPARATOR = "--FrameBoundary";

	enum class OutputCamsEn {
		CAM_L = 0,
		CAM_R = 1,
		CAM_BOTH = 2
	};

}  // namespace fcapconstants

#endif
