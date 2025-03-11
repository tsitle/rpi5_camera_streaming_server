#ifndef SUBPROC_BNC_HPP_
#define SUBPROC_BNC_HPP_

#include "subproc.hpp"

namespace framesubproc {

	struct BncDataStc {
		int16_t brightness;
		int16_t contrast;
		int16_t gamma;

		BncDataStc() {
			reset();
		}

		void reset() {
			brightness = 0;
			contrast = 0;
			gamma = 0;
		}

		bool equal(const BncDataStc &o2) const {
			return (brightness == o2.brightness && contrast == o2.contrast && gamma == o2.gamma);
		}
	};

	class FrameSubProcessorBrightnAndContrast final : public FrameSubProcessor {
		public:
			FrameSubProcessorBrightnAndContrast();
			void setBrightness(int16_t val);
			void setContrast(int16_t val);
			void setGamma(int16_t val);
			void getData(int16_t &brightn, int16_t &contr, int16_t &gamma) const;
			void loadData();
			void processFrame(cv::Mat &frame, uint32_t frameNr) override;
		
		private:
			BncDataStc gBncDataStc;
			BncDataStc gLastBncDataStc;
			double gDblBrightn;
			bool gInitdBrightn;
			double gDblContr;
			bool gInitdContr;
			cv::Mat gGammaLookUpTable;
			bool gInitdGamma;
			bool gLoadedFromFile;
			bool gLoadFromFileFailed;
			bool gWriteToFileFailed;

			//

			inline void processFrame_algo1(cv::Mat &frame);
			inline void processFrame_algo2(cv::Mat &frame) const;
			void saveBncDataToFile();
			bool loadBncDataFromFile();
			void deleteBncDataFile();
			static std::string buildFnExtraQual();
	};

}  // namespace framesubproc

#endif  // SUBPROC_BNC_HPP_
