#ifndef SUBPROC_HPP_
#define SUBPROC_HPP_

#include <string>
#include <opencv2/opencv.hpp>

#include "../../constants.hpp"
#include "../../cfgfile.hpp"

namespace framesubproc {

	class FrameSubProcessor {
		public:
			explicit FrameSubProcessor(std::string spName);
			virtual ~FrameSubProcessor() = default;
			void setCamIdAndOutputCams(fcapconstants::CamIdEn camId, fcapconstants::OutputCamsEn outputCams);
			virtual void setInputFrameSize(const cv::Size &frameSz);
			virtual void processFrame(cv::Mat &frame, uint32_t frameNr) = 0;

		protected:
			fcapconstants::CamIdEn gCamId;
			fcapconstants::OutputCamsEn gOutputCams;
			cv::Size gInpFrameSz;
			fcapcfgfile::StaticOptionsStc gStaticOptionsStc;
			uint32_t gFrameNr;
			std::string gSpName;

			//

			void log(const std::string &message);
			std::string buildDataFilename(const std::string &extraQualifiers, bool addCamName = true);
			void saveDataToFile_header(cv::FileStorage &fs);
			bool loadDataFromFile_header(cv::FileStorage &fs);
			void deleteDataFile(const std::string &dataFn);
			static void saveDataToFile_point2f(cv::FileStorage &fs, const char *key, uint8_t ix, cv::Point2f &point);
			static void loadDataFromFile_point2f(cv::FileStorage &fs, const char *key, uint8_t ix, cv::Point2f &point);
	};

}  // namespace framesubproc

#endif  // SUBPROC_HPP_
