#include "../../settings.hpp"
#include "subproc_text.hpp"

namespace framesubproc {

	FrameSubProcessorText::FrameSubProcessorText() :
			FrameSubProcessor(),
			gText("---"),
			gCoord(0, 0),
			gTextColor(0.0, 0.0, 0.0) {
		//
		///
		gTextOverlayPropsStc.textFontScale = 1.0;
		gTextOverlayPropsStc.textThickness = 2;
		gTextOverlayPropsStc.textFontId = cv::FONT_HERSHEY_SIMPLEX;
		///
		gTextOverlayPropsStc.rectColor = cv::Scalar(255.0, 255.0, 255.0);
		gTextOverlayPropsStc.rectThickness = cv::FILLED;
	}

	void FrameSubProcessorText::setText(const std::string valText, const cv::Point coord, cv::Scalar textColor) {
		const uint8_t _BORDER = 5;

		gText = (valText.empty() ? "---" : valText);
		//
		cv::Size tmpSz = getTextSize(gText);
		//
		gCoord = coord;
		gTextOverlayPropsStc.textCoordinates = cv::Point(
				gCoord.x + _BORDER,
				gCoord.y + _BORDER + tmpSz.height
			);
		gTextColor = textColor;
		//
		gTextOverlayPropsStc.rectEndPoint = cv::Point(
				gCoord.x + _BORDER + tmpSz.width + _BORDER,
				gCoord.x + _BORDER + tmpSz.height + _BORDER
			);
	}

	void FrameSubProcessorText::processFrame(cv::Mat &frame) {
		cv::rectangle(
				frame,
				gCoord,
				gTextOverlayPropsStc.rectEndPoint,
				gTextOverlayPropsStc.rectColor,
				gTextOverlayPropsStc.rectThickness
			);
		cv::putText(
				frame,
				gText,
				gTextOverlayPropsStc.textCoordinates,
				gTextOverlayPropsStc.textFontId,
				gTextOverlayPropsStc.textFontScale,
				gTextColor,
				gTextOverlayPropsStc.textThickness,
				cv::LINE_AA,
				false
			);
	}

	// -----------------------------------------------------------------------------
	// -----------------------------------------------------------------------------

	cv::Size FrameSubProcessorText::getTextSize(const std::string &text) {
		int baseline = 0;
		cv::Size resSz = cv::getTextSize(
				text,
				gTextOverlayPropsStc.textFontId,
				gTextOverlayPropsStc.textFontScale,
				gTextOverlayPropsStc.textThickness,
				&baseline
			);
		baseline += gTextOverlayPropsStc.textThickness;
		return resSz + cv::Size(0, baseline) - cv::Size(0, 11);
	}

}  // namespace framesubproc
