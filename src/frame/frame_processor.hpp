#ifndef FRAME_PROCESSOR_HPP_
#define FRAME_PROCESSOR_HPP_

#include <string>
#include <opencv2/opencv.hpp>

#include "../cfgfile.hpp"
#include "../shared.hpp"
#include "subprocessor/subproc_bnc.hpp"
#include "subprocessor/subproc_calibrate.hpp"
#include "subprocessor/subproc_text.hpp"

namespace frame {

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
			framesubproc::FrameSubProcessorText gOtherSubProcTextCams;
			SubProcsStc gSubProcsL;
			SubProcsStc gSubProcsR;
			int8_t gLastOutputCamsInt;

			//

			void log(const std::string &message);
			void updateSubProcsSettings();
			void _updateSubProcsSettings_stc(SubProcsStc &subProcsStc);
			void procDefaults(SubProcsStc &subProcsStc, cv::Mat &frame);
			void procAddTextOverlay(cv::Mat &frameOut, const std::string &camDesc, const fcapconstants::OutputCamsEn outputCams);
	};

}  // namespace frame

#endif  // FRAME_PROCESSOR_HPP_
