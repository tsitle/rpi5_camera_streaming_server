#ifndef SUBPROC_HPP_
#define SUBPROC_HPP_

#include <string>
#include <opencv2/opencv.hpp>

#include "../../constants.hpp"
#include "../../cfgfile.hpp"

namespace framesubproc {

	class FrameSubProcessor {
		public:
			FrameSubProcessor();
			~FrameSubProcessor();
			void setCamId(fcapconstants::CamIdEn camId);
			virtual void processFrame(cv::Mat &frame) = 0;

		protected:
			fcapconstants::CamIdEn gCamId;
			fcapcfgfile::StaticOptionsStc gStaticOptionsStc;

			//

			void log(const std::string &spName, const std::string &message);
	};

}  // namespace framesubproc

#endif  // SUBPROC_HPP_
