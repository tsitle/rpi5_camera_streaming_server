#ifndef FRAME_PROCESSOR_HPP_
#define FRAME_PROCESSOR_HPP_

#include <string>
#include <opencv2/opencv.hpp>

#include "../cfgfile.hpp"
#include "../shared.hpp"
#include "subprocessor/subproc.hpp"

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

	struct SubProcsStc {
		framesubproc::FrameSubProcessorBrightnAndContrast bnc;
		framesubproc::FrameSubProcessorCalibrate cal;
	};

	class FrameProcessor {
		public:
			FrameProcessor();
			~FrameProcessor();
			void setRuntimeOptionsPnt(fcapshared::RuntimeOptionsStc *pOptsRt);
			void processFrame(fcapconstants::OutputCamsEn outputCams, cv::Mat *pFrameL, cv::Mat *pFrameR, cv::Mat **ppFrameOut);

		private:
			fcapcfgfile::StaticOptionsStc gStaticOptionsStc;
			fcapshared::RuntimeOptionsStc* gPOptsRt;
			bool gDisableProcessing;
			TextOverlayPropsStc gTextOverlayPropsStc;
			SubProcsStc gSubProcsL;
			SubProcsStc gSubProcsR;

			//

			void log(const std::string &message);
			void updateSubProcsSettings();
			void _updateSubProcsSettings_stc(SubProcsStc &subProcsStc);
			void procDefaults(SubProcsStc &subProcsStc, cv::Mat &frame);
			void procAddTextOverlay(cv::Mat &frameOut, const std::string &camDesc, const bool isOneCam);
			cv::Size getTextSize(const std::string &text);
	};

}  // namespace frame

#endif  // FRAME_PROCESSOR_HPP_
