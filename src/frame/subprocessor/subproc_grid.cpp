#include "../../settings.hpp"
#include "../../shared.hpp"
#include "subproc_grid.hpp"

namespace framesubproc {

	FrameSubProcessorGrid::FrameSubProcessorGrid() :
			FrameSubProcessor() {
	}

	void FrameSubProcessorGrid::setData(const bool fullgrid) {
		gGridDataStc.fullgrid = fullgrid;
	}

	void FrameSubProcessorGrid::processFrame(cv::Mat &frame, const uint32_t frameNr) {
		uint32_t imgW = frame.size().width;
		uint32_t imgH = frame.size().height;
		const int32_t centerX = (int32_t)((imgW - 1) / 2);
		const int32_t centerY = (int32_t)((imgH - 1) / 2);

		//
		gFrameNr = frameNr;
		//
		cv::line(
				frame,
				cv::Point(centerX, 0),
				cv::Point(centerX, imgH - 1),
				cv::Scalar(0, 0, 255),
				1
			);
		cv::line(
				frame,
				cv::Point(0, centerY),
				cv::Point(imgW - 1, centerY),
				cv::Scalar(0, 0, 255),
				1
			);

		if (! gGridDataStc.fullgrid) {
			return;
		}

		//
		const int32_t GRID_DIVS = 30;
		int32_t tmpLineX;
		int32_t tmpLineY;

		for (int32_t tmpX = 0; tmpX < (int32_t)(imgW / (GRID_DIVS * 2)); tmpX++) {
			tmpLineX = centerX - tmpX * GRID_DIVS;
			if (tmpLineX != centerX) {
				cv::line(
						frame,
						cv::Point(tmpLineX, 0),
						cv::Point(tmpLineX, imgH - 1),
						cv::Scalar(127, 0, 127),
						1
					);
			}
			tmpLineX = centerX + tmpX * GRID_DIVS;
			if (tmpLineX != centerX) {
				cv::line(
						frame,
						cv::Point(tmpLineX, 0),
						cv::Point(tmpLineX, imgH - 1),
						cv::Scalar(127, 0, 127),
						1
					);
			}
		}
		for (int32_t tmpY = 0; tmpY < (int32_t)(imgH / (GRID_DIVS * 2)); tmpY++) {
			tmpLineY = centerY - tmpY * GRID_DIVS;
			if (tmpLineY != centerY) {
				cv::line(
						frame,
						cv::Point(0, tmpLineY),
						cv::Point(imgW - 1, tmpLineY),
						cv::Scalar(127, 127, 0),
						1
					);
			}
			tmpLineY = centerY + tmpY * GRID_DIVS;
			if (tmpLineY != centerY) {
				cv::line(
						frame,
						cv::Point(0, tmpLineY),
						cv::Point(imgW - 1, tmpLineY),
						cv::Scalar(127, 127, 0),
						1
					);
			}
		}
	}

}  // namespace framesubproc
