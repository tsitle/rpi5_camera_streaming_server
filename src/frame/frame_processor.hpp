#ifndef FRAME_PROCESSOR_HPP_
#define FRAME_PROCESSOR_HPP_

#include <string>
#include <opencv2/opencv.hpp>

#include "../cfgfile.hpp"

namespace frame {

	const std::string TEXT_CAM_PREFIX = "CAM ";
	const std::string TEXT_CAM_SUFFIX_L = "L";
	const std::string TEXT_CAM_SUFFIX_R = "R";
	const std::string TEXT_CAM_SUFFIX_BOTH = "L+R";

	struct TextOverlayPropsStc {
		cv::Point rectStartPoint;
		cv::Point rectEndPointOne;
		cv::Point rectEndPointTwo;
		cv::Scalar rectColor;
		int rectThickness;
		cv::Point textCoordinates;
		double textFontScale;
		cv::Scalar textColor;
		int textThickness;
		int textFontId;
		std::string textPrefix;
	};

	class FrameProcessor {
		public:
			FrameProcessor();
			~FrameProcessor();
			void processFrame(cv::Mat *pFrameL, cv::Mat *pFrameR, cv::Mat **ppFrameOut);

		private:
			fcapcfgfile::StaticOptionsStc gStaticOptionsStc;
			bool gDisableProcessing;
			TextOverlayPropsStc gTextOverlayPropsStc;

			//

			void log(const std::string &message);
			void procAddTextOverlay(cv::Mat &frameOut, const std::string &camDesc, const bool isOneCam);
			cv::Size getTextSize(const std::string &text);
	};

}  // namespace frame

#endif  // FRAME_PROCESSOR_HPP_
