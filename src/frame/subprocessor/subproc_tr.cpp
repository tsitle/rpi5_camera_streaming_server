#include "../../settings.hpp"
#include "../../shared.hpp"
#include "subproc_tr.hpp"

namespace framesubproc {

	FrameSubProcessorTranslation::FrameSubProcessorTranslation() :
			FrameSubProcessor(),
			gLoadedFromFile(false),
			gLoadFromFileFailed(false),
			gWriteToFileFailed(false) {
		gTransMatrix = cv::Mat(2, 3, CV_64FC1);
		gTransMatrix.at<double>(0, 0) = 1.0;
		gTransMatrix.at<double>(0, 1) = 0.0;
		gTransMatrix.at<double>(0, 2) = 0.0;
		gTransMatrix.at<double>(1, 0) = 0.0;
		gTransMatrix.at<double>(1, 1) = 1.0;
		gTransMatrix.at<double>(1, 2) = 0.0;
	}

	void FrameSubProcessorTranslation::setDelta(const int32_t valDx, const int32_t valDy) {
		gTrDataStc.dx = valDx;
		gTrDataStc.dy = valDy;
		if (! gTrDataStc.equal(gLastTrDataStc)) {
			if (! gWriteToFileFailed) {
				saveTrDataToFile();
			}
			gLastTrDataStc = gTrDataStc;
		}
		gTransMatrix.at<double>(0, 2) = (double)gTrDataStc.dx;
		gTransMatrix.at<double>(1, 2) = (double)gTrDataStc.dy;
	}

	void FrameSubProcessorTranslation::getDelta(int32_t &valDx, int32_t &valDy) {
		valDx = gTrDataStc.dx;
		valDy = gTrDataStc.dy;
	}

	void FrameSubProcessorTranslation::resetData() {
		deleteTrDataFile();
	}

	void FrameSubProcessorTranslation::processFrame(cv::Mat &frame) {
		if (gWriteToFileFailed) {
			return;
		}
		//
		if (! (gLoadedFromFile || gLoadFromFileFailed)) {
			gLoadedFromFile = loadTrDataFromFile();
		}
		//
		if (gTrDataStc.equal(EMPTY_TR_DATA)) {
			return;
		}
		cv::Mat frameIn = frame.clone();
		frame = cv::Mat();
		cv::warpAffine(frameIn, frame, gTransMatrix, frameIn.size(), cv::INTER_LINEAR);
	}

	// -----------------------------------------------------------------------------
	// -----------------------------------------------------------------------------

	void FrameSubProcessorTranslation::saveTrDataToFile() {
		if (gWriteToFileFailed) {
			return;
		}

		std::string extraQual = buildFnExtraQual();
		std::string outpFn = buildDataFilename("TR", extraQual);

		log("TR", "writing Translation data to file '" + outpFn + "'");
		cv::FileStorage fs(outpFn, cv::FileStorage::WRITE | cv::FileStorage::FORMAT_YAML);

		//
		saveDataToFile_header(fs);

		//
		fs << "dx" << gTrDataStc.dx;
		fs << "dy" << gTrDataStc.dy;

		gWriteToFileFailed = (! fcapshared::Shared::fileExists(outpFn));
	}

	bool FrameSubProcessorTranslation::loadTrDataFromFile() {
		if (gLoadedFromFile || gLoadFromFileFailed) {
			return false;
		}

		std::string extraQual = buildFnExtraQual();
		std::string inpFn = buildDataFilename("TR", extraQual);

		if (! fcapshared::Shared::fileExists(inpFn)) {
			gLoadFromFileFailed = true;
			return false;
		}

		log("TR", "loading Translation data from file '" + inpFn + "'");

		cv::FileStorage fs(inpFn, cv::FileStorage::READ | cv::FileStorage::FORMAT_YAML);

		//
		gLoadFromFileFailed = (! loadDataFromFile_header("TR", fs));
		if (gLoadFromFileFailed) {
			return false;
		}

		//
		gTrDataStc.reset();
		fs["dx"] >> gTrDataStc.dx;
		fs["dy"] >> gTrDataStc.dy;
		gLastTrDataStc = gTrDataStc;

		//
		gTransMatrix.at<double>(0, 2) = (double)gTrDataStc.dx;
		gTransMatrix.at<double>(1, 2) = (double)gTrDataStc.dy;

		log("TR", "__reading done");
		return true;
	}

	void FrameSubProcessorTranslation::deleteTrDataFile() {
		std::string extraQual = buildFnExtraQual();
		std::string outpFn = buildDataFilename("TR", extraQual);

		deleteDataFile("TR", outpFn);
		gLoadedFromFile = false;
		gTrDataStc.reset();
		gLastTrDataStc.reset();
		gTransMatrix.at<double>(0, 2) = 0.0;
		gTransMatrix.at<double>(1, 2) = 0.0;
	}

	std::string FrameSubProcessorTranslation::buildFnExtraQual() {
		std::string extraQual = (gStaticOptionsStc.procEnabled.cal ? "wcal" : "");
		if (gStaticOptionsStc.procEnabled.pt) {
			extraQual += (extraQual.empty() ? "" : "_") + std::string("wpt");
		}
		return extraQual;
	}

}  // namespace framesubproc
