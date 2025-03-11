#include <ctime>  // for time(), localtime(), strftime()
#include <utility>

#include "../../shared.hpp"
#include "subproc.hpp"

namespace framesubproc {

	FrameSubProcessor::FrameSubProcessor(std::string spName) :
			gCamId(fcapconstants::CamIdEn::CAM_0),
			gOutputCams(fcapconstants::OutputCamsEn::CAM_BOTH),
			gFrameNr(0),
			gSpName(std::move(spName)) {
		gStaticOptionsStc = fcapcfgfile::CfgFile::getStaticOptions();
	}

	void FrameSubProcessor::setCamIdAndOutputCams(fcapconstants::CamIdEn camId, fcapconstants::OutputCamsEn outputCams) {
		gCamId = camId;
		gOutputCams = outputCams;
	}

	void FrameSubProcessor::setInputFrameSize(const cv::Size &frameSz) {
		gInpFrameSz = frameSz;
	}

	// -----------------------------------------------------------------------------
	// -----------------------------------------------------------------------------

	void FrameSubProcessor::log(const std::string &message) {
		std::string ciStr;
		std::string ocStr;

		switch (gOutputCams) {
			case fcapconstants::OutputCamsEn::CAM_L:
				ciStr = std::to_string((int)gCamId);
				ocStr = "L";
				break;
			case fcapconstants::OutputCamsEn::CAM_R:
				ciStr = std::to_string((int)gCamId);
				ocStr = "R";
				break;
			default:
				ciStr = "-";
				ocStr = "BOTH";
		}
		
		std::cout << "FSUBPROC[CAM" << ciStr << ocStr << "_" + gSpName + "] " << message << std::endl;
	}

	// -----------------------------------------------------------------------------

	std::string FrameSubProcessor::buildDataFilename(const std::string &extraQualifiers, const bool addCamName) {
		std::string camSrcStr;
		switch (gStaticOptionsStc.camSourceType) {
			case fcapconstants::CamSourceEn::GSTREAMER:
				camSrcStr = "gstreamer";
				break;
			case fcapconstants::CamSourceEn::MJPEG:
				camSrcStr = "mjpeg";
				break;
			default:
				camSrcStr = "unspec";
		}
		return gStaticOptionsStc.calibOutputPath + "/" + gSpName +
				(addCamName ? "-cam" + std::to_string((int)gCamId) : std::string("")) + "-" +
				camSrcStr + "-" +
				(gStaticOptionsStc.camSourceType == fcapconstants::CamSourceEn::GSTREAMER ?
						std::to_string(gStaticOptionsStc.gstreamerResolutionCapture.width) + "x" +
						std::to_string(gStaticOptionsStc.gstreamerResolutionCapture.height) + "-"
						: "") +
				std::to_string(gStaticOptionsStc.resolutionInputStream.width) + "x" +
				std::to_string(gStaticOptionsStc.resolutionInputStream.height) +
				(! extraQualifiers.empty() ? "-" + extraQualifiers : "") +
				".yaml";
	}

	void FrameSubProcessor::saveDataToFile_header(cv::FileStorage &fs) {
		time_t tm;
		::time(&tm);
		struct tm *t2 = ::localtime(&tm);
		char buf[1024];
		::strftime(buf, sizeof(buf), "%c", t2);

		fs << "calibrationTime" << buf;
		//
		fs << "camID" << (int)gCamId;
		fs << "camSource" << (int)gStaticOptionsStc.camSourceType;
		if (gStaticOptionsStc.camSourceType == fcapconstants::CamSourceEn::GSTREAMER) {
			fs << "gstreamer_resolution_capture" << gStaticOptionsStc.gstreamerResolutionCapture;
		}
		fs << "resolution_input_stream" << gStaticOptionsStc.resolutionInputStream;
	}

	bool FrameSubProcessor::loadDataFromFile_header(cv::FileStorage &fs) {
		int cfileInt;
		cv::Size cfileSz;

		//
		std::string cfileCalibrationTime;
		fs["calibrationTime"] >> cfileCalibrationTime;
		//
		///
		fs["camID"] >> cfileInt;
		if (cfileInt != (int)gCamId) {
			log("camID from PersTransf data file does not match current camID (is=" + std::to_string(cfileInt) +
					", exp=" + std::to_string((int)gCamId) + ")");
			return false;
		}
		///
		fs["camSource"] >> cfileInt;
		if (cfileInt != (int)gStaticOptionsStc.camSourceType) {
			log("camSource from PersTransf data file does not match current camSource (is=" + std::to_string(cfileInt) +
					", exp=" + std::to_string((int)gStaticOptionsStc.camSourceType) + ")");
			return false;
		}
		///
		if (gStaticOptionsStc.camSourceType == fcapconstants::CamSourceEn::GSTREAMER) {
			fs["gstreamer_resolution_capture"] >> cfileSz;
			if (cfileSz != gStaticOptionsStc.gstreamerResolutionCapture) {
				log("gstreamer_resolution_capture from PersTransf data file "
						"does not match current gstreamer_resolution_capture (is=" +
						std::to_string(cfileSz.width) + "x" + std::to_string(cfileSz.height) +
						", exp=" +
						std::to_string(gStaticOptionsStc.gstreamerResolutionCapture.width) + "x" +
						std::to_string(gStaticOptionsStc.gstreamerResolutionCapture.height) +
						")");
				return false;
			}
		}
		///
		fs["resolution_input_stream"] >> cfileSz;
		if (cfileSz != gStaticOptionsStc.resolutionInputStream) {
			log("resolution_input_stream from PersTransf data file "
					"does not match current resolution_input_stream (is=" +
					std::to_string(cfileSz.width) + "x" + std::to_string(cfileSz.height) +
					", exp=" +
					std::to_string(gStaticOptionsStc.resolutionInputStream.width) + "x" +
					std::to_string(gStaticOptionsStc.resolutionInputStream.height) +
					")");
			return false;
		}

		return true;
	}

	void FrameSubProcessor::deleteDataFile(const std::string &dataFn) {
		if (! fcapshared::Shared::fileExists(dataFn)) {
			return;
		}

		log("deleting data file '" + dataFn + "'");
		::remove(dataFn.c_str());
	}

	void FrameSubProcessor::saveDataToFile_point2f(
			cv::FileStorage &fs, const char *key, uint8_t ix, cv::Point2f &point) {
		fs << std::string(key) + "_" + std::to_string(ix) + "_x" << point.x;
		fs << std::string(key) + "_" + std::to_string(ix) + "_y" << point.y;
	}

	void FrameSubProcessor::loadDataFromFile_point2f(
			cv::FileStorage &fs, const char *key, uint8_t ix, cv::Point2f &point) {
		fs[std::string(key) + "_" + std::to_string(ix) + "_x"] >> point.x;
		fs[std::string(key) + "_" + std::to_string(ix) + "_y"] >> point.y;
	}

}  // namespace framesubproc
