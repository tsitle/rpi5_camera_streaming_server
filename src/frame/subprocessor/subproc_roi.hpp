#ifndef SUBPROC_CROP_HPP_
#define SUBPROC_CROP_HPP_

#include "subproc.hpp"

namespace framesubproc {

	struct RoiDataStc {
		uint32_t sizeW;
		uint32_t sizeH;
		uint8_t percent;

		RoiDataStc() {
			reset();
		}

		void reset() {
			sizeH = 0;
			sizeW = 0;
			percent = 100;
		}
	};

	class FrameSubProcessorRoi final : public FrameSubProcessor {
		public:
			FrameSubProcessorRoi();
			void setInputFrameSize(const cv::Size &frameSz) override;
			void setData(uint8_t roiSizePercent);
			cv::Size getOutputSz() const;
			uint8_t getSizePercent() const;
			void resetData();
			void loadData();
			void processFrame(cv::Mat &frame, uint32_t frameNr) override;
		
		private:
			RoiDataStc gRoiDataStc;
			bool gLoadedFromFile;
			bool gLoadFromFileFailed;
			bool gWriteToFileFailed;

			//

			void saveRoiDataToFile();
			bool loadRoiDataFromFile();
			void deleteRoiDataFile();
			std::string buildFnExtraQual();
	};

}  // namespace framesubproc

#endif  // SUBPROC_CROP_HPP_
