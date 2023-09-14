#ifndef SUBPROC_HPP_
#define SUBPROC_HPP_

#include <string>
#include <opencv2/opencv.hpp>

#include "../../constants.hpp"

namespace framesubproc {

	class FrameSubProcessor {
		public:
			FrameSubProcessor();
			~FrameSubProcessor();
			void setCamId(fcapconstants::CamIdEn camId);
			virtual void processFrame(cv::Mat &frame) = 0;

		protected:
			fcapconstants::CamIdEn gCamId;

			//

			void log(const std::string &message);
	};

	class FrameSubProcessorBrightnAndContrast : public FrameSubProcessor {
		public:
			FrameSubProcessorBrightnAndContrast();
			void setBrightness(const int16_t val);
			void setContrast(const int16_t val);
			void processFrame(cv::Mat &frame);
		
		private:
			int16_t gBrightness;
			int16_t gContrast;
	};

	class FrameSubProcessorCalibrate : public FrameSubProcessor {
		public:
			FrameSubProcessorCalibrate();
			void processFrame(cv::Mat &frame);
		
		private:
			const uint8_t MAX_TRIES = 100;
			bool gCalibrated;
			uint8_t gTries;
	};

}  // namespace framesubproc

#endif  // SUBPROC_HPP_
