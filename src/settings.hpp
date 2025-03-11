#ifndef SETTINGS_HPP_
#define SETTINGS_HPP_

#include <opencv2/opencv.hpp>

#include "constants.hpp"

namespace fcapsettings {

	/**
	 * Default framerate in frames per second
	 */
	constexpr uint8_t STREAM_DEFAULT_FPS = 15;
	/**
	 * Enable adaptive framerate per default?
	 */
	constexpr bool STREAM_DEFAULT_ENABLE_ADAPTIVE_FPS = true;
	/**
	 * Default camera stream capture size.
	 * This defines the field of view
	 */
	const cv::Size STREAM_DEFAULT_CAPTURE_SZ = fcapconstants::PIPE_CAPTURE_SZ_1536X864;  // can also be a PIPE_OUTPUT_SZ_*
	/**
	 * Default camera stream input size.
	 * This defines how the captured stream is scaled before being sent to the application
	 */
	const cv::Size STREAM_DEFAULT_INPUT_SZ = fcapconstants::PIPE_OUTPUT_SZ_1280X720;  // can also be a PIPE_CAPTURE_SZ_*

	/**
	 * For debugging only: open camera streams?
	 */
	constexpr bool DBG_OPEN_CAM_STREAMS = true;

	// -------------------------------------------------------------------------

	/**
	 * Default TCP port for Webserver
	 */
	constexpr uint16_t TCP_DEFAULT_SERVER_PORT = 8090;
	/**
	 * Maximum allowed amount of concurrent clients for Webserver
	 */
	constexpr uint32_t TCP_MAX_CLIENTS = 12;
	/**
	 * Maximum allowed amount of concurrent streaming clients for Webserver.
	 * Needs to be <= TCP_MAX_CLIENTS
	 */
	constexpr uint32_t TCP_MAX_STREAMING_CLIENTS = 4;

	// -------------------------------------------------------------------------

	/**
	 * Size of the image frame queue.
	 * Higher values result in higher latency, but can slightly increase the framerate
	 */
	constexpr uint8_t IF_QUEUE_SIZE = 2;

	// -------------------------------------------------------------------------

	/**
	 * Use split view instead of blended view per default when both cameras are enabled?
	 */
	constexpr bool PROC_DEFAULT_SPLITVIEW_FOR_CAMBOTH = true;

	// -------------------------------------------------------------------------

	/**
	 * JPEG compression aka quality (higher value results in better image quality).
	 * Range is 0..100
	 */
	constexpr uint8_t PROC_JPEG_QUALITY = 90;

	/**
	 * For debugging only: disable all image processing?
	 */
	constexpr bool DBG_PROC_DISABLE_ALL_PROCESSING = false;

	// -------------------------------------------------------------------------

	/**
	 * Algorithm to use for Brightness/Contrast
	 */
	constexpr fcapconstants::ProcBncAlgoEn PROC_BNC_USE_ALGO = fcapconstants::ProcBncAlgoEn::TYPE2;

	// -------------------------------------------------------------------------

	/**
	 * Amount of inner corners on the x-Axis on the calibration chessboard image.
	 * Since the image is rotated 90 degress when calibrating, columns and rows need to be swapped here
	 */
	constexpr uint8_t PROC_CAL_CHESS_SQUARES_INNERCORNERS_COL = 4;
	/**
	 * Amount of inner corners on the y-Axis on the calibration chessboard image.
	 * Since the image is rotated 90 degress when calibrating, columns and rows need to be swapped here
	 */
	constexpr uint8_t PROC_CAL_CHESS_SQUARES_INNERCORNERS_ROW = 6;
	/**
	 * Width of the squares on the calibration chessboard image in mm
	 */
	constexpr float PROC_CAL_CHESS_SQUARES_WIDTH_MM = 5.0;  // mm
	/**
	 * Maximum allowed projection error for the calibration
	 */
	constexpr double PROC_CAL_MAX_PROJECTION_ERROR = 0.6;
	/**
	 * Render undistorted image once calibration has been completed?
	 * Roughly doubles the CPU load if enabled
	 */
	constexpr bool PROC_CAL_UNDISTORT = false;

}  // namespace fcapsettings

#endif  // SETTINGS_HPP_
