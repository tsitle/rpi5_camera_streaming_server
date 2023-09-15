#ifndef SUBPROC_BNC_HPP_
#define SUBPROC_BNC_HPP_

#include "subproc.hpp"

namespace framesubproc {

	class FrameSubProcessorBrightnAndContrast : public FrameSubProcessor {
		public:
			FrameSubProcessorBrightnAndContrast();
			void setBrightness(const int16_t val);
			void setContrast(const int16_t val);
			void processFrame(cv::Mat &frame);
		
		private:
			int16_t gOptBrightness;
			int16_t gOptContrast;
	};

}  // namespace framesubproc

#endif  // SUBPROC_BNC_HPP_
