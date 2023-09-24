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
			void setInputFrameSize(const cv::Size &frameSz);
			virtual void processFrame(cv::Mat &frame) = 0;

		protected:
			fcapconstants::CamIdEn gCamId;
			fcapconstants::OutputCamsEn gOutputCams;
			cv::Size gInpFrameSz;
			fcapcfgfile::StaticOptionsStc gStaticOptionsStc;

			//

			void log(const std::string &spName, const std::string &message);
			std::string buildDataFilename(const std::string &spName, const std::string &extraQualifiers, const bool addCamName = true);
			void saveDataToFile_header(cv::FileStorage &fs);
			bool loadDataFromFile_header(const std::string &spName, cv::FileStorage &fs);
			void deleteDataFile(const std::string &spName, const std::string &dataFn);
			void saveDataToFile_point2f(cv::FileStorage &fs, const char *key, uint8_t ix, cv::Point2f &point);
			void loadDataFromFile_point2f(cv::FileStorage &fs, const char *key, uint8_t ix, cv::Point2f &point);
	};

}  // namespace framesubproc

#endif  // SUBPROC_HPP_
