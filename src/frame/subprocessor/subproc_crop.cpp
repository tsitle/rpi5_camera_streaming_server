#include "../../settings.hpp"
#include "../../shared.hpp"
#include "subproc_crop.hpp"

namespace framesubproc {

	FrameSubProcessorCrop::FrameSubProcessorCrop() :
			FrameSubProcessor(),
			gHaveAllCorners(false),
			gLoadedFromFile(false),
			gLoadFromFileFailed(false),
			gWriteToFileFailed(false) {
	}

	void FrameSubProcessorCrop::setRectCorners(const std::vector<cv::Point> &val) {
	}

	std::vector<cv::Point> FrameSubProcessorCrop::getRectCorners() {
	}

	bool FrameSubProcessorCrop::getNeedRectCorners() {
		return (! gHaveAllCorners);
	}

	void FrameSubProcessorCrop::resetData() {
		gHaveAllCorners = false;
		gOptRectCorners.clear();
		deleteCropDataFile();
	}

	void FrameSubProcessorCrop::processFrame(cv::Mat &frame) {
		if (gWriteToFileFailed) {
			return;
		}
		//
		if (! (gHaveAllCorners || gLoadedFromFile || gLoadFromFileFailed)) {
			gLoadedFromFile = loadCropDataFromFile(frame.size());
			if (gLoadedFromFile) {
				gHaveAllCorners = true;
			}
		}
		//
		if (! gHaveAllCorners) {
			// draw only the first four circles
			uint8_t tmpSz = gOptRectCorners.size();
			for (uint8_t x = 1; x <= tmpSz; x++) {
				cv::Point tmpPoint(gOptRectCorners[x - 1].x - 2, gOptRectCorners[x - 1].y - 2);
				cv::Scalar tmpColor = (x <= fcapconstants::PROC_PT_RECTCORNERS_MAX ? cv::Scalar(255, 0, 0) : cv::Scalar(255, 255, 0));
				cv::circle(frame, tmpPoint, 5, tmpColor, -1);
				/**log("CROP", "cirle " + std::to_string(tmpPoint.x) + "/" + std::to_string(tmpPoint.y));**/
			}
			return;
		}
		cv::Mat transMatrix = cv::getPerspectiveTransform(gCropDataStc.ptsSrc, gCropDataStc.ptsDst);
		cv::Mat frameIn = frame.clone();
		frame = cv::Mat();
		cv::warpPerspective(frameIn, frame, transMatrix, frame.size());
	}

	// -----------------------------------------------------------------------------
	// -----------------------------------------------------------------------------

	void FrameSubProcessorCrop::saveCropDataToFile() {
		if (gWriteToFileFailed) {
			return;
		}

		std::string extraQual = buildFnExtraQual();
		std::string outpFn = buildDataFilename("CROP", extraQual);

		log("CROP", "writing Translation data to file '" + outpFn + "'");
		cv::FileStorage fs(outpFn, cv::FileStorage::WRITE | cv::FileStorage::FORMAT_YAML);

		//
		saveDataToFile_header(fs);

		//
		for (uint8_t x = 0; x < fcapconstants::PROC_PT_RECTCORNERS_MAX; x++) {
			saveDataToFile_point2f(fs, "crop_points_src", x, gCropDataStc.ptsSrc[x]);
		}
		for (uint8_t x = 0; x < fcapconstants::PROC_PT_RECTCORNERS_MAX; x++) {
			saveDataToFile_point2f(fs, "crop_points_dst", x, gCropDataStc.ptsDst[x]);
		}

		gWriteToFileFailed = (! fcapshared::Shared::fileExists(outpFn));
	}

	bool FrameSubProcessorCrop::loadCropDataFromFile(cv::Size imageSize) {
		if (gLoadedFromFile || gLoadFromFileFailed) {
			return false;
		}

		std::string extraQual = buildFnExtraQual();
		std::string inpFn = buildDataFilename("CROP", extraQual);

		if (! fcapshared::Shared::fileExists(inpFn)) {
			gLoadFromFileFailed = true;
			return false;
		}

		log("CROP", "loading Translation data from file '" + inpFn + "'");

		cv::FileStorage fs(inpFn, cv::FileStorage::READ | cv::FileStorage::FORMAT_YAML);

		//
		gLoadFromFileFailed = (! loadDataFromFile_header("CROP", imageSize, fs));
		if (gLoadFromFileFailed) {
			return false;
		}

		//
		gOptRectCorners.clear();
		for (uint8_t x = 0; x < fcapconstants::PROC_PT_RECTCORNERS_MAX; x++) {
			loadDataFromFile_point2f(fs, "crop_points_src", x, gCropDataStc.ptsSrc[x]);
			gOptRectCorners.push_back(gCropDataStc.ptsSrc[x]);
		}
		for (uint8_t x = 0; x < fcapconstants::PROC_PT_RECTCORNERS_MAX; x++) {
			loadDataFromFile_point2f(fs, "crop_points_dst", x, gCropDataStc.ptsDst[x]);
		}

		log("CROP", "__reading done");
		return true;
	}

	void FrameSubProcessorCrop::deleteCropDataFile() {
		std::string extraQual = buildFnExtraQual();

		deleteDataFile("CROP", extraQual);
		gLoadedFromFile = false;
		gCropDataStc.reset();
	}

	std::string FrameSubProcessorCrop::buildFnExtraQual() {
		std::string extraQual = (gStaticOptionsStc.procEnabled.cal ? "wcal" : "");
		if (gStaticOptionsStc.procEnabled.pt) {
			extraQual += (extraQual.empty() ? "" : "_") + std::string("wpt");
		}
		if (gStaticOptionsStc.procEnabled.tr) {
			extraQual += (extraQual.empty() ? "" : "_") + std::string("wtr");
		}
		return extraQual;
	}

}  // namespace framesubproc
