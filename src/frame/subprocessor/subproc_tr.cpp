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

	void FrameSubProcessorTranslation::setFixDelta(const int32_t valDx, const int32_t valDy) {
		gTrDataFixStc.dx = valDx;
		gTrDataFixStc.dy = valDy;
		if (! gTrDataFixStc.equal(gLastTrDataFixStc)) {
			if (! gWriteToFileFailed) {
				saveTrDataToFile();
			}
			gLastTrDataFixStc = gTrDataFixStc;
		}
		updateTranslMtx();
	}

	void FrameSubProcessorTranslation::getFixDelta(int32_t &valDx, int32_t &valDy) {
		valDx = gTrDataFixStc.dx;
		valDy = gTrDataFixStc.dy;
	}

	void FrameSubProcessorTranslation::setDynDelta(const int32_t valDx, const int32_t valDy) {
		gTrDataDynStc.dx = valDx;
		gTrDataDynStc.dy = valDy;
		updateTranslMtx();
	}

	void FrameSubProcessorTranslation::resetData() {
		deleteTrDataFile();
	}

	void FrameSubProcessorTranslation::loadData() {
		gLoadedFromFile = loadTrDataFromFile();
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
		if (gTrDataFixStc.equal(EMPTY_TR_DATA) && gTrDataDynStc.equal(EMPTY_TR_DATA)) {
			return;
		}
		cv::Mat frameIn = frame.clone();
		frame = cv::Mat();
		cv::warpAffine(frameIn, frame, gTransMatrix, frameIn.size(), cv::INTER_LINEAR);
	}

	// -----------------------------------------------------------------------------
	// -----------------------------------------------------------------------------

	void FrameSubProcessorTranslation::updateTranslMtx() {
		// y-Axis
		gTransMatrix.at<double>(0, 2) = (double)(gStaticOptionsStc.procEnabled.roi ? -gTrDataFixStc.dy : gTrDataFixStc.dx);
		gTransMatrix.at<double>(0, 2) += (double)(gStaticOptionsStc.procEnabled.roi ? gTrDataDynStc.dy : gTrDataDynStc.dx);
		// x-Axis
		gTransMatrix.at<double>(1, 2) = (double)(gStaticOptionsStc.procEnabled.roi ? gTrDataFixStc.dx : gTrDataFixStc.dy);
		gTransMatrix.at<double>(1, 2) += (double)(gStaticOptionsStc.procEnabled.roi ? gTrDataDynStc.dx : -gTrDataDynStc.dy);
	}

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
		fs << "dx" << gTrDataFixStc.dx;
		fs << "dy" << gTrDataFixStc.dy;

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
		gTrDataFixStc.reset();
		fs["dx"] >> gTrDataFixStc.dx;
		fs["dy"] >> gTrDataFixStc.dy;
		gLastTrDataFixStc = gTrDataFixStc;

		//
		setFixDelta(gTrDataFixStc.dx, gTrDataFixStc.dy);

		log("TR", "__reading done");
		return true;
	}

	void FrameSubProcessorTranslation::deleteTrDataFile() {
		std::string extraQual = buildFnExtraQual();
		std::string outpFn = buildDataFilename("TR", extraQual);

		deleteDataFile("TR", outpFn);
		gLoadedFromFile = false;
		gTrDataFixStc.reset();
		gLastTrDataFixStc.reset();
		gTrDataDynStc.reset();
		setFixDelta(0, 0);
	}

	std::string FrameSubProcessorTranslation::buildFnExtraQual() {
		std::string extraQual = (gStaticOptionsStc.procEnabled.cal ? "wcal" : "");

		if (gStaticOptionsStc.procEnabled.roi) {
			extraQual += (extraQual.empty() ? "" : "_") + std::string("wroi");
		}
		if (gStaticOptionsStc.procEnabled.pt) {
			extraQual += (extraQual.empty() ? "" : "_") + std::string("wpt");
		}
		if (gStaticOptionsStc.procEnabled.flip &&
				(gStaticOptionsStc.flip[gCamId].hor || gStaticOptionsStc.flip[gCamId].ver)) {
			extraQual += (extraQual.empty() ? "" : "_") + std::string("wflip");
			if (gStaticOptionsStc.flip[gCamId].hor && ! gStaticOptionsStc.flip[gCamId].ver) {
				extraQual += "H";
			} else if (! gStaticOptionsStc.flip[gCamId].hor && gStaticOptionsStc.flip[gCamId].ver) {
				extraQual += "V";
			} else {
				extraQual += "B";
			}
		}
		return extraQual;
	}

}  // namespace framesubproc
