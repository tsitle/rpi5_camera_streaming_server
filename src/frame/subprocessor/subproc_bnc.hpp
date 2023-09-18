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
	};

	class FrameSubProcessorBrightnAndContrast : public FrameSubProcessor {
		public:
			FrameSubProcessorBrightnAndContrast();
			void setBrightness(const int16_t val);
			void setContrast(const int16_t val);
			void processFrame(cv::Mat &frame);
		
		private:
			BncDataStc gBncDataStc;
	};

}  // namespace framesubproc

#endif  // SUBPROC_BNC_HPP_
