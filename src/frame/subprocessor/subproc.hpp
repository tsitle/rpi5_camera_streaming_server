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
			void setCamIdAndOutputCams(fcapconstants::CamIdEn camId, fcapconstants::OutputCamsEn outputCams);
			virtual void processFrame(cv::Mat &frame) = 0;

		protected:
			fcapconstants::CamIdEn gCamId;
			fcapconstants::OutputCamsEn gOutputCams;
			fcapcfgfile::StaticOptionsStc gStaticOptionsStc;

			//

			void log(const std::string &spName, const std::string &message);
			std::string buildDataFilename(const std::string &spName);
			void saveDataToFile_header(cv::FileStorage &fs);
			bool loadDataFromFile_header(const std::string &spName, cv::Size &imageSize, cv::FileStorage &fs);
			void deleteDataFile(const std::string &spName);
	};

}  // namespace framesubproc

#endif  // SUBPROC_HPP_
