#ifndef SUBPROC_TEXT_HPP_
#define SUBPROC_TEXT_HPP_

#include "subproc.hpp"

namespace framesubproc {

	struct TextOverlayPropsStc {
		cv::Point rectEndPoint;
		cv::Scalar rectColor;
		int rectThickness;
		cv::Point textCoordinates;
		int32_t textThickness;
		int32_t textFontId;
		int32_t textBottomYinOutput;
		double outputScale;
	};

	class FrameSubProcessorText : public FrameSubProcessor {
		public:
			FrameSubProcessorText();
			void setText(const std::string valText, const cv::Point coord, cv::Scalar textColor, double scale = 1.0);
			int32_t getTextBottomY();
			void processFrame(cv::Mat &frame);

		private:
			TextOverlayPropsStc gTextOverlayPropsStc;
			std::string gText;
			cv::Point gCoord;
			cv::Scalar gTextColor;

			//

			cv::Size getTextSize(const std::string &text);
	};

}  // namespace framesubproc

#endif  // SUBPROC_TEXT_HPP_
