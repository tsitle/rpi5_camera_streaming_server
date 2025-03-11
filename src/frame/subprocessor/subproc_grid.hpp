#ifndef SUBPROC_GRID_HPP_
#define SUBPROC_GRID_HPP_

#include "subproc.hpp"

namespace framesubproc {

	struct GridDataStc {
		bool fullgrid;

		GridDataStc() {
			reset();
		}

		void reset() {
			fullgrid = true;
		}
	};

	class FrameSubProcessorGrid final : public FrameSubProcessor {
		public:
			FrameSubProcessorGrid();
			void setData(bool fullgrid);
			void processFrame(cv::Mat &frame, uint32_t frameNr) override;
		
		private:
			GridDataStc gGridDataStc;
	};

}  // namespace framesubproc

#endif  // SUBPROC_GRID_HPP_
