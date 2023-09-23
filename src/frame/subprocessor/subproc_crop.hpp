#ifndef SUBPROC_CROP_HPP_
#define SUBPROC_CROP_HPP_

#include "subproc.hpp"

namespace framesubproc {

	struct CropDataStc {
		int32_t dx;
		int32_t dy;

		CropDataStc() {
			reset();
		}

		void reset() {
			dx = 0;
			dy = 0;
		}
	};

	class FrameSubProcessorCrop : public FrameSubProcessor {
		public:
			FrameSubProcessorCrop();
			void setRectCorners(const std::vector<cv::Point> &val);
			std::vector<cv::Point> getRectCorners();
			bool getNeedRectCorners();
			void resetData();
			void processFrame(cv::Mat &frame);
		
		private:
			std::vector<cv::Point> gOptRectCorners;
			bool gHaveAllCorners;
			CropDataStc gCropDataStc;
			bool gLoadedFromFile;
			bool gLoadFromFileFailed;
			bool gWriteToFileFailed;

			//

			void saveCropDataToFile();
			bool loadCropDataFromFile(cv::Size imageSize);
			void deleteCropDataFile();
			std::string buildFnExtraQual();
	};

}  // namespace framesubproc

#endif  // SUBPROC_CROP_HPP_
