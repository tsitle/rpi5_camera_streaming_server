#include "../../settings.hpp"
#include "../../shared.hpp"
#include "subproc_pt.hpp"

namespace framesubproc {

	FrameSubProcessorPerspectiveTransf::FrameSubProcessorPerspectiveTransf() :
			FrameSubProcessor(),
			gHaveAllCorners(false),
			gHaveSomeCorners(false),
			gLoadedFromFile(false),
			gLoadFromFileFailed(false),
			gWriteToFileFailed(false) {
	}

	void FrameSubProcessorPerspectiveTransf::setRectCorners(const std::vector<cv::Point> &val) {
		uint8_t inpValSz = val.size();
		bool haveChanged = (inpValSz != gOptRectCorners.size());

		if (! haveChanged) {
			for (uint8_t x = 1; x <= inpValSz && x <= fcapconstants::PROC_PT_RECTCORNERS_MAX; x++) {
				if (val[x] != gOptRectCorners[x]) {
					haveChanged = true;
					break;
				}
			}
			if (! haveChanged) {
				return;
			}
		}
		//
		gHaveAllCorners = false;
		gHaveSomeCorners = false;
		gOptRectCorners.clear();
		for (uint8_t x = 1; x <= inpValSz && x <= fcapconstants::PROC_PT_RECTCORNERS_MAX; x++) {
			cv::Point tmpPoint = val[x - 1];
			gOptRectCorners.push_back(tmpPoint);
			gHaveSomeCorners = true;
		}
		if (gOptRectCorners.size() == fcapconstants::PROC_PT_RECTCORNERS_MAX) {
			// compute destination rectangle corners
			cv::Point tmpPoint1 = gOptRectCorners[0];
			cv::Point tmpPoint2 = gOptRectCorners[1];
			cv::Point tmpPoint3 = gOptRectCorners[2];

			gOptRectCorners.push_back(cv::Point(tmpPoint1.x, tmpPoint1.y));
			gOptRectCorners.push_back(cv::Point(tmpPoint1.x, tmpPoint2.y));
			gOptRectCorners.push_back(cv::Point(tmpPoint3.x, tmpPoint1.y));
			gOptRectCorners.push_back(cv::Point(tmpPoint3.x, tmpPoint2.y));
			// store source rectangle corners
			gPtDataStc.ptsSrc[0] = gOptRectCorners[0];
			gPtDataStc.ptsSrc[1] = gOptRectCorners[1];
			gPtDataStc.ptsSrc[2] = gOptRectCorners[2];
			gPtDataStc.ptsSrc[3] = gOptRectCorners[3];
			// store destination rectangle corners
			gPtDataStc.ptsDst[0] = gOptRectCorners[4];
			gPtDataStc.ptsDst[1] = gOptRectCorners[5];
			gPtDataStc.ptsDst[2] = gOptRectCorners[6];
			gPtDataStc.ptsDst[3] = gOptRectCorners[7];
			//
			gHaveAllCorners = true;
		}
		//
		if (gHaveAllCorners) {
			gLoadedFromFile = false;
			gLoadFromFileFailed = false;
			savePtDataToFile();
		} else if (gLoadedFromFile) {
			deletePtDataFile();
			gLoadedFromFile = false;
		}
	}

	std::vector<cv::Point> FrameSubProcessorPerspectiveTransf::getRectCorners() {
		std::vector<cv::Point> resV;
		uint8_t inpValSz = gOptRectCorners.size();

		for (uint8_t x = 1; x <= inpValSz; x++) {
			cv::Point tmpPoint = gOptRectCorners[x - 1];
			resV.push_back(tmpPoint);
		}
		return resV;
	}

	bool FrameSubProcessorPerspectiveTransf::getNeedRectCorners() {
		return (! gHaveAllCorners);
	}

	void FrameSubProcessorPerspectiveTransf::processFrame(cv::Mat &frame) {
		if (gWriteToFileFailed) {
			return;
		}
		//
		if (! (gHaveAllCorners || gLoadedFromFile || gLoadFromFileFailed)) {
			gLoadedFromFile = loadPtDataFromFile(frame.size());
			if (gLoadedFromFile) {
				gHaveAllCorners = true;
			}
		}
		//
		if (! gHaveAllCorners) {
			if (! gHaveSomeCorners) {
				return;
			}
			// draw only the first four circles
			uint8_t tmpSz = gOptRectCorners.size();
			for (uint8_t x = 1; x <= tmpSz; x++) {
				cv::Point tmpPoint(gOptRectCorners[x - 1].x - 2, gOptRectCorners[x - 1].y - 2);
				cv::Scalar tmpColor = (x <= fcapconstants::PROC_PT_RECTCORNERS_MAX ? cv::Scalar(255, 0, 0) : cv::Scalar(255, 255, 0));
				cv::circle(frame, tmpPoint, 5, tmpColor, -1);
				/**log("PT", "cirle " + std::to_string(tmpPoint.x) + "/" + std::to_string(tmpPoint.y));**/
			}
			return;
		}
		cv::Mat transMatrix = cv::getPerspectiveTransform(gPtDataStc.ptsSrc, gPtDataStc.ptsDst);
		cv::Mat frameIn = frame.clone();
		frame = cv::Mat();
		cv::warpPerspective(frameIn, frame, transMatrix, frame.size());
	}

	// -----------------------------------------------------------------------------
	// -----------------------------------------------------------------------------

	void FrameSubProcessorPerspectiveTransf::_savePtDataToFile_point2f(
			cv::FileStorage &fs, const char *key, uint8_t ix, cv::Point2f &point) {
		fs << std::string(key) + "_" + std::to_string(ix) + "_x" << point.x;
		fs << std::string(key) + "_" + std::to_string(ix) + "_y" << point.y;
	}

	void FrameSubProcessorPerspectiveTransf::_loadPtDataToFile_point2f(
			cv::FileStorage &fs, const char *key, uint8_t ix, cv::Point2f &point) {
		fs[std::string(key) + "_" + std::to_string(ix) + "_x"] >> point.x;
		fs[std::string(key) + "_" + std::to_string(ix) + "_y"] >> point.y;
	}

	void FrameSubProcessorPerspectiveTransf::savePtDataToFile() {
		if (gWriteToFileFailed) {
			return;
		}

		std::string outpFn = buildDataFilename("PT");

		log("PT", "writing PersTransf data to file '" + outpFn + "'");
		cv::FileStorage fs(outpFn, cv::FileStorage::WRITE | cv::FileStorage::FORMAT_YAML);

		//
		saveDataToFile_header(fs);

		//
		for (uint8_t x = 0; x < fcapconstants::PROC_PT_RECTCORNERS_MAX; x++) {
			_savePtDataToFile_point2f(fs, "pt_points_src", x, gPtDataStc.ptsSrc[x]);
		}
		for (uint8_t x = 0; x < fcapconstants::PROC_PT_RECTCORNERS_MAX; x++) {
			_savePtDataToFile_point2f(fs, "pt_points_dst", x, gPtDataStc.ptsDst[x]);
		}

		gWriteToFileFailed = (! fcapshared::Shared::fileExists(outpFn));
	}

	bool FrameSubProcessorPerspectiveTransf::loadPtDataFromFile(cv::Size imageSize) {
		if (gLoadedFromFile || gLoadFromFileFailed) {
			return false;
		}

		std::string inpFn = buildDataFilename("PT");

		if (! fcapshared::Shared::fileExists(inpFn)) {
			gLoadFromFileFailed = true;
			return false;
		}

		log("PT", "loading PersTransf data from file '" + inpFn + "'");

		cv::FileStorage fs(inpFn, cv::FileStorage::READ | cv::FileStorage::FORMAT_YAML);

		//
		gLoadFromFileFailed = (! loadDataFromFile_header("PT", imageSize, fs));
		if (gLoadFromFileFailed) {
			return false;
		}

		//
		gOptRectCorners.clear();
		for (uint8_t x = 0; x < fcapconstants::PROC_PT_RECTCORNERS_MAX; x++) {
			_loadPtDataToFile_point2f(fs, "pt_points_src", x, gPtDataStc.ptsSrc[x]);
			gOptRectCorners.push_back(gPtDataStc.ptsSrc[x]);
		}
		for (uint8_t x = 0; x < fcapconstants::PROC_PT_RECTCORNERS_MAX; x++) {
			_loadPtDataToFile_point2f(fs, "pt_points_dst", x, gPtDataStc.ptsDst[x]);
		}

		log("PT", "__reading done");
		return true;
	}

	void FrameSubProcessorPerspectiveTransf::deletePtDataFile() {
		std::string dataFn = buildDataFilename("PT");

		if (! fcapshared::Shared::fileExists(dataFn)) {
			return;
		}

		log("PT", "deleting PersTransf data file '" + dataFn + "'");
		::remove(dataFn.c_str());
	}

}  // namespace framesubproc
