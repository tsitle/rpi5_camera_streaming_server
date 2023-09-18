#ifndef SUBPROC_TR_HPP_
#define SUBPROC_TR_HPP_

#include "subproc.hpp"

namespace framesubproc {

	struct TrDataStc {
		int32_t dx;
		int32_t dy;

		TrDataStc() {
			reset();
		}

		TrDataStc(int32_t dx, int32_t dy) {
			this->dx = dx;
			this->dy = dy;
		}

		void reset() {
			dx = 0;
			dy = 0;
		}

		bool equal(const TrDataStc &o2) const {
			return (dx == o2.dx && dy == o2.dy);
		}
	};

	class FrameSubProcessorTranslation : public FrameSubProcessor {
		public:
			FrameSubProcessorTranslation();
			void setDelta(const int32_t valDx, const int32_t valDy);
			void getDelta(int32_t &valDx, int32_t &valDy);
			void resetData();
			void processFrame(cv::Mat &frame);
		
		private:
			const TrDataStc EMPTY_TR_DATA = TrDataStc(0, 0);
			TrDataStc gTrDataStc;
			TrDataStc gLastTrDataStc;
			bool gLoadedFromFile;
			bool gLoadFromFileFailed;
			bool gWriteToFileFailed;
			cv::Mat gTransMatrix;

			//

			void saveTrDataToFile();
			bool loadTrDataFromFile(cv::Size imageSize);
			void deleteTrDataFile();
	};

}  // namespace framesubproc

#endif  // SUBPROC_TR_HPP_
