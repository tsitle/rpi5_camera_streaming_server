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

	class FrameSubProcessorFlip final : public FrameSubProcessor {
		public:
			FrameSubProcessorFlip();
			void setData(bool flipHor, bool flipVer);
			void processFrame(cv::Mat &frame, uint32_t frameNr) override;
		
		private:
			FlipDataStc gFlipDataStc;
	};

}  // namespace framesubproc

#endif  // SUBPROC_FLIP_HPP_
