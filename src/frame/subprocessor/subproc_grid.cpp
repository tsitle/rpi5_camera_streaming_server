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

	void FrameSubProcessorGrid::processFrame(cv::Mat &frame) {
		cv::line(
				frame,
				cv::Point((int32_t)(gOutpFrameSz.width / 2), 0),
				cv::Point((int32_t)(gOutpFrameSz.width / 2), gOutpFrameSz.height - 1),
				cv::Scalar(0, 0, 255),
				1
			);
		cv::line(
				frame,
				cv::Point(0, (int32_t)(gOutpFrameSz.height / 2)),
				cv::Point(gOutpFrameSz.width - 1, (int32_t)(gOutpFrameSz.height / 2)),
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

		for (int32_t tmpX = 0; tmpX < (int32_t)(gOutpFrameSz.width / (GRID_DIVS * 2)); tmpX++) {
			tmpLineX = (int32_t)(gOutpFrameSz.width / 2) - tmpX * GRID_DIVS;
			if (tmpLineX != int(gOutpFrameSz.width / 2)) {
				cv::line(
						frame,
						cv::Point(tmpLineX, 0),
						cv::Point(tmpLineX, gOutpFrameSz.height - 1),
						cv::Scalar(127, 0, 127),
						1
					);
			}
			tmpLineX = (int32_t)(gOutpFrameSz.width / 2) + tmpX * GRID_DIVS;
			if (tmpLineX != int(gOutpFrameSz.width / 2)) {
				cv::line(
						frame,
						cv::Point(tmpLineX, 0),
						cv::Point(tmpLineX, gOutpFrameSz.height - 1),
						cv::Scalar(127, 0, 127),
						1
					);
			}
		}
		for (int32_t tmpY = 0; tmpY < (int32_t)(gOutpFrameSz.height / (GRID_DIVS * 2)); tmpY++) {
			tmpLineY = (int32_t)(gOutpFrameSz.height / 2) - tmpY * GRID_DIVS;
			if (tmpLineY != (int32_t)(gOutpFrameSz.height / 2)) {
				cv::line(
						frame,
						cv::Point(0, tmpLineY),
						cv::Point(gOutpFrameSz.width - 1, tmpLineY),
						cv::Scalar(127, 127, 0),
						1
					);
			}
			tmpLineY = (int32_t)(gOutpFrameSz.height / 2) + tmpY * GRID_DIVS;
			if (tmpLineY != (int32_t)(gOutpFrameSz.height / 2)) {
				cv::line(
						frame,
						cv::Point(0, tmpLineY),
						cv::Point(gOutpFrameSz.width - 1, tmpLineY),
						cv::Scalar(127, 127, 0),
						1
					);
			}
		}
	}

}  // namespace framesubproc
