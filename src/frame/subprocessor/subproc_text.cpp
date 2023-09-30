#include "../../settings.hpp"
#include "subproc_text.hpp"

namespace framesubproc {

	FrameSubProcessorText::FrameSubProcessorText() :
			FrameSubProcessor("TEXT") {
		// init coordinates etc.
		setText(gText, gCoord, gTextColor);
	}

	void FrameSubProcessorText::setText(
			const std::string valText,
			const cv::Point coord,
			cv::Scalar textColor,
			double scale) {
		const uint8_t _BORDER = 5;

		gText = (valText.empty() ? "---" : valText);
		//
		cv::Size tmpSz = getTextSize(gText);
		//
		gCoord = coord;
		gTextOverlayPropsStc.textCoordinates = cv::Point(
				gCoord.x + _BORDER,
				gCoord.y + _BORDER + tmpSz.height - 6
			);
		gTextColor = textColor;
		//
		gTextOverlayPropsStc.rectEndPoint = cv::Point(
				gCoord.x + _BORDER + tmpSz.width + _BORDER,
				gCoord.y + _BORDER + tmpSz.height + _BORDER
			);
		//
		gTextOverlayPropsStc.outputScale = scale;
	}

	int32_t FrameSubProcessorText::getTextBottomY() {
		return gTextOverlayPropsStc.textBottomYinOutput;
	}

	void FrameSubProcessorText::processFrame(cv::Mat &frame, const uint32_t frameNr) {
		gFrameNr = frameNr;
		//
		cv::Mat textFrame = cv::Mat(
				gTextOverlayPropsStc.rectEndPoint.y - gCoord.y,
				gTextOverlayPropsStc.rectEndPoint.x - gCoord.x,
				CV_8UC3/*,
				cv::Scalar(0, 0, 0)*/
			);

		cv::rectangle(
				textFrame,
				cv::Point(0, 0),
				gTextOverlayPropsStc.rectEndPoint,
				gTextOverlayPropsStc.rectColor,
				gTextOverlayPropsStc.rectThickness
			);
		cv::putText(
				textFrame,
				gText,
				gTextOverlayPropsStc.textCoordinates - gCoord,
				gTextOverlayPropsStc.textFontId,
				/*textFontScale:*/1.0,
				gTextColor,
				gTextOverlayPropsStc.textThickness,
				cv::LINE_AA,
				false
			);

		if (textFrame.channels() != frame.channels()) {
			cv::cvtColor(textFrame, textFrame, cv::COLOR_BGR2GRAY);
		}

		cv::Size tfrSz = textFrame.size();

		if (gTextOverlayPropsStc.outputScale < 0.99 || gTextOverlayPropsStc.outputScale > 1.01) {
			cv::resize(
					textFrame,
					textFrame,
					cv::Size((int32_t)(tfrSz.width * gTextOverlayPropsStc.outputScale),
							(int32_t)(tfrSz.height * gTextOverlayPropsStc.outputScale)),
					0.0,
					0.0,
					cv::INTER_LINEAR
				);
			tfrSz = textFrame.size();
		}
		gTextOverlayPropsStc.textBottomYinOutput = gCoord.y + tfrSz.height;
		textFrame.copyTo(
				frame
					.rowRange(gCoord.y, gTextOverlayPropsStc.textBottomYinOutput)
					.colRange(gCoord.x, gCoord.x + tfrSz.width)
			);
	}

	// -----------------------------------------------------------------------------
	// -----------------------------------------------------------------------------

	cv::Size FrameSubProcessorText::getTextSize(const std::string &text) {
		int32_t baseline = 0;
		cv::Size resSz = cv::getTextSize(
				text,
				gTextOverlayPropsStc.textFontId,
				/*textFontScale:*/1.0,
				gTextOverlayPropsStc.textThickness,
				&baseline
			);
		baseline += gTextOverlayPropsStc.textThickness;
		return resSz + cv::Size(0, baseline);
	}

}  // namespace framesubproc
