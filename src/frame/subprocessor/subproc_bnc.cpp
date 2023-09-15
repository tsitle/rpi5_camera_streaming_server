#include "../../settings.hpp"
#include "subproc_bnc.hpp"

namespace framesubproc {

	FrameSubProcessorBrightnAndContrast::FrameSubProcessorBrightnAndContrast() :
			FrameSubProcessor(),
			gOptBrightness(fcapsettings::PROC_BNC_DEFAULT_ADJ_BRIGHTNESS),
			gOptContrast(fcapsettings::PROC_BNC_DEFAULT_ADJ_CONTRAST) {
	}

	void FrameSubProcessorBrightnAndContrast::setBrightness(const int16_t val) {
		if (val >= fcapconstants::PROC_BNC_MIN_ADJ_BRIGHTNESS && val <= fcapconstants::PROC_BNC_MAX_ADJ_BRIGHTNESS) {
			gOptBrightness = val;
		}
	}

	void FrameSubProcessorBrightnAndContrast::setContrast(const int16_t val) {
		if (val >= fcapconstants::PROC_BNC_MIN_ADJ_CONTRAST && val <= fcapconstants::PROC_BNC_MAX_ADJ_CONTRAST) {
			gOptContrast = val;
		}
	}

	void FrameSubProcessorBrightnAndContrast::processFrame(cv::Mat &frame) {
		if (gOptBrightness == 0 && gOptContrast == 0) {
			return;
		}

		cv::Mat *pContrFrameInp = &frame;
		cv::Mat brightnFrameOut;
		cv::Mat contrFrameOut;
		cv::Mat *pFrameOut = &brightnFrameOut;

		if (gOptBrightness != 0) {
			int16_t shadow = gOptBrightness;
			int16_t highlight = 255;
			if (gOptBrightness > 0) {
				shadow = gOptBrightness;
				highlight = 255;
			} else {
				shadow = 0;
				highlight = 255 + gOptBrightness;
			}
			double alpha_b = (double)(highlight - shadow) / 255.0;
			double gamma_b = (double)shadow;
			
			/**
			 * gOptBrightness = 23
			 * --> a=0.909804, g=23
			 */
			/**std::cout << "B a=" << alpha_b << ", g=" << gamma_b << std::endl;**/
			cv::addWeighted(frame, alpha_b, frame, 0.0, gamma_b, *pFrameOut);
			pContrFrameInp = pFrameOut;
		}
		if (gOptContrast != 0) {
			double f = 131.0 * (double)(gOptContrast + 127) / (double)(127 * (131 - gOptContrast));
			double alpha_c = f;
			double gamma_c = 127.0 * (double)(1.0 - f);
			
			/**
			 * gOptContrast = 5
			 * --> a=1.08061, g=-10.2381
			 */
			/**std::cout << "C a=" << alpha_c << ", g=" << gamma_c << std::endl;**/
			cv::addWeighted(*pContrFrameInp, alpha_c, *pContrFrameInp, 0.0, gamma_c, contrFrameOut);
			pFrameOut = &contrFrameOut;
		}
		//
		frame = pFrameOut->clone();
	}

}  // namespace framesubproc
