#ifndef SUBPROC_PT_HPP_
#define SUBPROC_PT_HPP_

#include "subproc.hpp"

namespace framesubproc {

	struct PtDataStc {
		cv::Point2f ptsSrc[fcapconstants::PROC_PT_RECTCORNERS_MAX];
		cv::Point2f ptsDst[fcapconstants::PROC_PT_RECTCORNERS_MAX];
	};

	class FrameSubProcessorPerspectiveTransf : public FrameSubProcessor {
		public:
			FrameSubProcessorPerspectiveTransf();
			void setRectCorners(const std::vector<cv::Point> &val);
			std::vector<cv::Point> getRectCorners();
			bool getNeedRectCorners();
			void processFrame(cv::Mat &frame);
		
		private:
			std::vector<cv::Point> gOptRectCorners;
			bool gHaveAllCorners;
			bool gHaveSomeCorners;
			PtDataStc gPtDataStc;
			bool gLoadedFromFile;
			bool gLoadFromFileFailed;
			bool gWriteToFileFailed;

			//

			void _savePtDataToFile_point2f(cv::FileStorage &fs, const char *key, uint8_t ix, cv::Point2f &point);
			void _loadPtDataToFile_point2f(cv::FileStorage &fs, const char *key, uint8_t ix, cv::Point2f &point);
			void savePtDataToFile();
			bool loadPtDataFromFile(cv::Size imageSize);
			void deletePtDataFile();
	};

}  // namespace framesubproc

#endif  // SUBPROC_PT_HPP_
