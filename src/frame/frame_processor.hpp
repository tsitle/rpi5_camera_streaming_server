#ifndef FRAME_PROCESSOR_HPP_
#define FRAME_PROCESSOR_HPP_

#include <opencv2/opencv.hpp>

#include "../cfgfile.hpp"
#include "../shared.hpp"
#include "subprocessor/subproc_bnc.hpp"
#include "subprocessor/subproc_calibrate.hpp"
#include "subprocessor/subproc_flip.hpp"
#include "subprocessor/subproc_grid.hpp"
#include "subprocessor/subproc_pt.hpp"
#include "subprocessor/subproc_roi.hpp"
#include "subprocessor/subproc_scale.hpp"
#include "subprocessor/subproc_text.hpp"
#include "subprocessor/subproc_tr.hpp"

namespace frame {

	struct SubProcsStc {
		fcapconstants::OutputCamsEn outputCams;
		fcapconstants::CamIdEn camId;
		framesubproc::FrameSubProcessorCalibrate cal;
		framesubproc::FrameSubProcessorFlip flip;
		framesubproc::FrameSubProcessorTranslation tr;
		framesubproc::FrameSubProcessorPerspectiveTransf pt;
	};

	class FrameProcessor {
		public:
			FrameProcessor();
			~FrameProcessor() = default;
			void setRuntimeOptionsPnt(fcapshared::RuntimeOptionsStc *pOptsRt);
			void processFrame(cv::Mat *pFrameL, cv::Mat *pFrameR, cv::Mat *pFrameOut, uint32_t frameNr);

		private:
			fcapcfgfile::StaticOptionsStc gStaticOptionsStc;
			fcapshared::RuntimeOptionsStc* gPOptsRt;
			framesubproc::FrameSubProcessorBrightnAndContrast gOtherSubProcBnc;
			framesubproc::FrameSubProcessorGrid gOtherSubProcGrid;
			framesubproc::FrameSubProcessorRoi gOtherSubProcRoi;
			framesubproc::FrameSubProcessorScale gOtherSubProcScale;
			framesubproc::FrameSubProcessorText gOtherSubProcTextCal;
			framesubproc::FrameSubProcessorText gOtherSubProcTextCams;
			SubProcsStc gSubProcsL;
			SubProcsStc gSubProcsR;
			int8_t gLastOverlCalIsCalibratedInt;
			int32_t gLastOverlCalResolutionOutpW;
			int8_t gLastOverlCamsOutputCamsInt;
			int32_t gLastOverlCamsResolutionOutpW;
			cv::Size gRoiOutputSz;

			//

			static void log(const std::string &message);
			void initSubProcs();
			void _initSubProcs_stc(
					fcapconstants::CamIdEn camId,
					fcapconstants::OutputCamsEn outputCams,
					SubProcsStc &subProcsStc);
			void _initSubProcs_fspObj(
					fcapconstants::CamIdEn camId,
					fcapconstants::OutputCamsEn outputCams,
					framesubproc::FrameSubProcessor &fsp) const;
			void updateSubProcsSettings();
			void _updateSubProcsSettings_stc(SubProcsStc &subProcsStc) const;
			void procDefaults(SubProcsStc &subProcsStc, cv::Mat &frame, uint32_t frameNr);
			void procAddTextOverlayCams(
					cv::Mat &frameOut,
					uint32_t frameNr,
					const std::string &camDesc,
					fcapconstants::OutputCamsEn outputCams);
			void procAddTextOverlayCal(cv::Mat &frameOut, uint32_t frameNr, bool isCalibrated);
			bool checkFrameSize(const cv::Mat *pFrame, const std::string &camName) const;
			static void renderMasterOutput(
					const cv::Mat *pFrameL,
					const cv::Mat *pFrameR,
					cv::Mat *pFrameOut,
					__attribute__((unused)) uint32_t frameNr);
	};

}  // namespace frame

#endif  // FRAME_PROCESSOR_HPP_
