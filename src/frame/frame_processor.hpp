#ifndef FRAME_PROCESSOR_HPP_
#define FRAME_PROCESSOR_HPP_

#include <string>
#include <opencv2/opencv.hpp>

#include "../cfgfile.hpp"
#include "../shared.hpp"
#include "subprocessor/subproc_bnc.hpp"
#include "subprocessor/subproc_calibrate.hpp"
#include "subprocessor/subproc_flip.hpp"
#include "subprocessor/subproc_grid.hpp"
#include "subprocessor/subproc_pt.hpp"
#include "subprocessor/subproc_roi.hpp"
#include "subprocessor/subproc_text.hpp"
#include "subprocessor/subproc_tr.hpp"

namespace frame {

	struct SubProcsStc {
		fcapconstants::OutputCamsEn outputCams;
		fcapconstants::CamIdEn camId;
		framesubproc::FrameSubProcessorBrightnAndContrast bnc;
		framesubproc::FrameSubProcessorCalibrate cal;
		framesubproc::FrameSubProcessorFlip flip;
		framesubproc::FrameSubProcessorGrid grid;
		framesubproc::FrameSubProcessorTranslation tr;
		framesubproc::FrameSubProcessorPerspectiveTransf pt;
	};

	class FrameProcessor {
		public:
			FrameProcessor();
			~FrameProcessor();
			void setRuntimeOptionsPnt(fcapshared::RuntimeOptionsStc *pOptsRt);
			void processFrame(cv::Mat *pFrameL, cv::Mat *pFrameR, cv::Mat *pFrameOut, const uint32_t frameNr);

		private:
			fcapcfgfile::StaticOptionsStc gStaticOptionsStc;
			fcapshared::RuntimeOptionsStc* gPOptsRt;
			framesubproc::FrameSubProcessorRoi gOtherSubProcRoi;
			framesubproc::FrameSubProcessorText gOtherSubProcTextCams;
			framesubproc::FrameSubProcessorText gOtherSubProcTextCal;
			SubProcsStc gSubProcsL;
			SubProcsStc gSubProcsR;
			int8_t gLastOverlCamsOutputCamsInt;
			int8_t gLastOverlCalIsCalibratedInt;
			int32_t gLastOverlCamsResolutionOutpW;
			int32_t gLastOverlCalResolutionOutpW;
			cv::Size gRoiOutputSz;

			//

			void log(const std::string &message);
			void initSubProcs();
			void _initSubProcs_stc(fcapconstants::CamIdEn camId, fcapconstants::OutputCamsEn outputCams, SubProcsStc &subProcsStc);
			void _initSubProcs_fspObj(fcapconstants::CamIdEn camId, fcapconstants::OutputCamsEn outputCams, framesubproc::FrameSubProcessor &fsp);
			void updateSubProcsSettings();
			void _updateSubProcsSettings_stc(SubProcsStc &subProcsStc);
			void procDefaults(SubProcsStc &subProcsStc, cv::Mat &frame);
			void procAddTextOverlayCams(cv::Mat &frameOut, const std::string &camDesc, const fcapconstants::OutputCamsEn outputCams);
			void procAddTextOverlayCal(cv::Mat &frameOut, const bool isCalibrated);
			bool checkFrameSize(const cv::Mat *pFrame, const std::string camName);
			void renderMasterOutput(cv::Mat *pFrameL, cv::Mat *pFrameR, cv::Mat *pFrameOut, const uint32_t frameNr);
	};

}  // namespace frame

#endif  // FRAME_PROCESSOR_HPP_
