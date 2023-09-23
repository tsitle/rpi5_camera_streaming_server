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

	class FrameSubProcessorGrid : public FrameSubProcessor {
		public:
			FrameSubProcessorGrid();
			void setData(const bool fullgrid);
			void processFrame(cv::Mat &frame);
		
		private:
			GridDataStc gGridDataStc;
	};

}  // namespace framesubproc

#endif  // SUBPROC_GRID_HPP_
