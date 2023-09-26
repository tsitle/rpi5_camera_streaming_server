#ifndef SUBPROC_BNC_HPP_
#define SUBPROC_BNC_HPP_

#include "subproc.hpp"

namespace framesubproc {

	struct BncDataStc {
		int16_t brightness;
		int16_t contrast;

		BncDataStc() {
			reset();
		}

		void reset() {
			brightness = fcapsettings::PROC_BNC_DEFAULT_ADJ_BRIGHTNESS;
			contrast = fcapsettings::PROC_BNC_DEFAULT_ADJ_CONTRAST;
		}

		bool equal(const BncDataStc &o2) const {
			return (brightness == o2.brightness && contrast == o2.contrast);
		}
	};

	class FrameSubProcessorBrightnAndContrast : public FrameSubProcessor {
		public:
			FrameSubProcessorBrightnAndContrast();
			void setBrightness(const int16_t val);
			void setContrast(const int16_t val);
			void getData(int16_t &brightn, int16_t &contr);
			void loadData();
			void processFrame(cv::Mat &frame);
		
		private:
			BncDataStc gBncDataStc;
			BncDataStc gLastBncDataStc;
			bool gLoadedFromFile;
			bool gLoadFromFileFailed;
			bool gWriteToFileFailed;

			//

			void saveBncDataToFile();
			bool loadBncDataFromFile();
			void deleteBncDataFile();
	};

}  // namespace framesubproc

#endif  // SUBPROC_BNC_HPP_
