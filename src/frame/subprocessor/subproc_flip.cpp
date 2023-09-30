#include "../../settings.hpp"
#include "../../shared.hpp"
#include "subproc_flip.hpp"

namespace framesubproc {

	FrameSubProcessorFlip::FrameSubProcessorFlip() :
			FrameSubProcessor("FLIP") {
	}

	void FrameSubProcessorFlip::setData(const bool flipHor, const bool flipVer) {
		gFlipDataStc.hor = flipHor;
		gFlipDataStc.ver = flipVer;
		//
		if (gFlipDataStc.hor && gFlipDataStc.ver) {
			gFlipDataStc.flag = -1;  // flip around both axis
		} else if (gFlipDataStc.ver) {
			gFlipDataStc.flag = 0;  // flip around the x-axis
		} else {
			gFlipDataStc.flag = 1;  // flip around the y-axis
		}
	}

	void FrameSubProcessorFlip::processFrame(cv::Mat &frame, const uint32_t frameNr) {
		if (! (gFlipDataStc.hor || gFlipDataStc.ver)) {
			return;
		}
		//
		gFrameNr = frameNr;
		//
		cv::Mat frameIn = frame.clone();
		frame = cv::Mat();
		cv::flip(frameIn, frame, gFlipDataStc.flag);
	}

}  // namespace framesubproc
