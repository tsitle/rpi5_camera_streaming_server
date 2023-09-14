#include <stdio.h>

#include "../../settings.hpp"
#include "subproc.hpp"

namespace framesubproc {

	FrameSubProcessorBrightnAndContrast::FrameSubProcessorBrightnAndContrast() :
			FrameSubProcessor(),
			gBrightness(fcapsettings::PROC_DEFAULT_ADJ_BRIGHTNESS),
			gContrast(fcapsettings::PROC_DEFAULT_ADJ_CONTRAST) {
	}

	void FrameSubProcessorBrightnAndContrast::setBrightness(const int16_t val) {
		if (val >= fcapconstants::PROC_MIN_ADJ_BRIGHTNESS && val <= fcapconstants::PROC_MAX_ADJ_BRIGHTNESS) {
			gBrightness = val;
		}
	}

	void FrameSubProcessorBrightnAndContrast::setContrast(const int16_t val) {
		if (val >= fcapconstants::PROC_MIN_ADJ_CONTRAST && val <= fcapconstants::PROC_MAX_ADJ_CONTRAST) {
			gContrast = val;
		}
	}

	void FrameSubProcessorBrightnAndContrast::processFrame(cv::Mat &frame) {
		int16_t contrast = gContrast - 64;
		int16_t brightness = gBrightness - 127;

		if (brightness != 0) {
			int16_t shadow = brightness;
			int16_t highlight = 255;
			if (brightness > 0) {
				shadow = brightness;
				highlight = 255;
			} else {
				shadow = 0;
				highlight = 255 + brightness;
			}
			double alpha_b = (double)(highlight - shadow) / 255.0;
			double gamma_b = (double)shadow;
			
			cv::addWeighted(frame, alpha_b, frame, 0.0, gamma_b, frame);
		}
		
		if (contrast != 0) {
			double f = 131.0 * (double)(contrast + 127) / (double)(127 * (131 - contrast));
			double alpha_c = f;
			double gamma_c = 127.0 * (double)(1.0 - f);
			
			cv::addWeighted(frame, alpha_c, frame, 0.0, gamma_c, frame);
		}
	}

}  // namespace framesubproc
