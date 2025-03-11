#ifndef SUBPROC_SCALE_HPP_
#define SUBPROC_SCALE_HPP_

#include "subproc.hpp"

namespace framesubproc {

	class FrameSubProcessorScale final : public FrameSubProcessor {
		public:
			FrameSubProcessorScale();
			void processFrame(cv::Mat &frame, uint32_t frameNr) override;

		private:
	};

}  // namespace framesubproc

#endif  // SUBPROC_SCALE_HPP_
