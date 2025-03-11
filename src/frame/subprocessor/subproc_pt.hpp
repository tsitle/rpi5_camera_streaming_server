#ifndef SUBPROC_PT_HPP_
#define SUBPROC_PT_HPP_

#include "subproc.hpp"

namespace framesubproc {

	struct PtDataStc {
		cv::Point2f ptsSrc[fcapconstants::PROC_PT_RECTCORNERS_MAX];
		cv::Point2f ptsDst[fcapconstants::PROC_PT_RECTCORNERS_MAX];

		PtDataStc() {
			reset();
		}

		void reset() {
			for (uint8_t x = 0; x < fcapconstants::PROC_PT_RECTCORNERS_MAX; x++) {
				ptsSrc[x].x = 0.0;
				ptsSrc[x].y = 0.0;
				ptsDst[x].x = 0.0;
				ptsDst[x].y = 0.0;
			}
		}
	};

	class FrameSubProcessorPerspectiveTransf final : public FrameSubProcessor {
		public:
			FrameSubProcessorPerspectiveTransf();
			void setManualRectCorners(const std::vector<cv::Point> &val);
			void setCalRectCorners(const std::vector<cv::Point> &val);
			std::vector<cv::Point> getManualRectCorners();
			bool getNeedRectCorners() const;
			void setRoiOutputSz(const cv::Size &val);
			void resetData();
			void loadData();
			void processFrame(cv::Mat &frame, uint32_t frameNr) override;
		
		private:
			std::vector<cv::Point> gOptRectCorners;
			bool gHaveAllCorners;
			bool gHaveSomeCorners;
			PtDataStc gPtDataStc;
			cv::Size gRoiOutputSz;
			bool gLoadedFromFile;
			bool gLoadFromFileFailed;
			bool gWriteToFileFailed;

			//

			cv::Point translatePoint(const cv::Point &pnt);
			cv::Point getUntranslatedCorner(uint8_t ix);
			void savePtDataToFile();
			bool loadPtDataFromFile();
			void deletePtDataFile();
			std::string buildFnExtraQual();
	};

}  // namespace framesubproc

#endif  // SUBPROC_PT_HPP_
