#ifndef SUBPROC_FLIP_HPP_
#define SUBPROC_FLIP_HPP_

#include "subproc.hpp"

namespace framesubproc {

	struct FlipDataStc {
		bool hor;
		bool ver;
		int32_t flag;

		FlipDataStc() {
			reset();
		}

		void reset() {
			hor = false;
			ver = false;
			flag = 0;
		}
	};

	class FrameSubProcessorFlip : public FrameSubProcessor {
		public:
			FrameSubProcessorFlip();
			void setData(const bool flipHor, const bool flipVer);
			void processFrame(cv::Mat &frame, const uint32_t frameNr);
		
		private:
			FlipDataStc gFlipDataStc;
	};

}  // namespace framesubproc

#endif  // SUBPROC_FLIP_HPP_
