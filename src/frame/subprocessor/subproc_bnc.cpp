#include "../../settings.hpp"
#include "subproc_bnc.hpp"

namespace framesubproc {

	FrameSubProcessorBrightnAndContrast::FrameSubProcessorBrightnAndContrast() :
			FrameSubProcessor() {
	}

	void FrameSubProcessorBrightnAndContrast::setBrightness(const int16_t val) {
		if (val >= fcapconstants::PROC_BNC_MIN_ADJ_BRIGHTNESS && val <= fcapconstants::PROC_BNC_MAX_ADJ_BRIGHTNESS) {
			gBncDataStc.brightness = val;
		}
	}

	void FrameSubProcessorBrightnAndContrast::setContrast(const int16_t val) {
		if (val >= fcapconstants::PROC_BNC_MIN_ADJ_CONTRAST && val <= fcapconstants::PROC_BNC_MAX_ADJ_CONTRAST) {
			gBncDataStc.contrast = val;
		}
	}

	void FrameSubProcessorBrightnAndContrast::processFrame(cv::Mat &frame) {
		if (gBncDataStc.brightness == 0 && gBncDataStc.contrast == 0) {
			return;
		}

		cv::Mat *pContrFrameInp = &frame;
		cv::Mat brightnFrameOut;
		cv::Mat contrFrameOut;
		cv::Mat *pFrameOut = &brightnFrameOut;

		if (gBncDataStc.brightness != 0) {
			int16_t shadow = gBncDataStc.brightness;
			int16_t highlight = 255;
			if (gBncDataStc.brightness > 0) {
				shadow = gBncDataStc.brightness;
				highlight = 255;
			} else {
				shadow = 0;
				highlight = 255 + gBncDataStc.brightness;
			}
			double alpha_b = (double)(highlight - shadow) / 255.0;
			double gamma_b = (double)shadow;
			
			/**
			 * gBncDataStc.brightness = 23
			 * --> a=0.909804, g=23
			 */
			/**std::cout << "B a=" << alpha_b << ", g=" << gamma_b << std::endl;**/
			cv::addWeighted(frame, alpha_b, frame, 0.0, gamma_b, *pFrameOut);
			pContrFrameInp = pFrameOut;
		}
		if (gBncDataStc.contrast != 0) {
			double f = 131.0 * (double)(gBncDataStc.contrast + 127) / (double)(127 * (131 - gBncDataStc.contrast));
			double alpha_c = f;
			double gamma_c = 127.0 * (double)(1.0 - f);
			
			/**
			 * gBncDataStc.contrast = 5
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
