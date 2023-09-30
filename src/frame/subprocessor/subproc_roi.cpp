#include "../../settings.hpp"
#include "../../shared.hpp"
#include "subproc_roi.hpp"

namespace framesubproc {

	FrameSubProcessorRoi::FrameSubProcessorRoi() :
			FrameSubProcessor("ROI"),
			gLoadedFromFile(false),
			gLoadFromFileFailed(false),
			gWriteToFileFailed(false) {
	}

	void FrameSubProcessorRoi::setInputFrameSize(const cv::Size &frameSz) {
		FrameSubProcessor::setInputFrameSize(frameSz);
		setData(gRoiDataStc.percent);
	}

	void FrameSubProcessorRoi::setData(const uint8_t roiSizePercent) {
		uint8_t lastPerc = gRoiDataStc.percent;

		gRoiDataStc.percent = (roiSizePercent < 10 ? 10 : roiSizePercent);
		gRoiDataStc.sizeW = (uint32_t)((double)gInpFrameSz.height * ((double)gRoiDataStc.percent / 100.0));
		gRoiDataStc.sizeH = (uint32_t)(((double)gRoiDataStc.sizeW / 16.0) * 9.0);
		if (lastPerc != gRoiDataStc.percent) {
			saveRoiDataToFile();
		}
		/**log("set roiSizePercent to " + std::to_string(roiSizePercent) + ", osz=" +
				std::to_string(gRoiDataStc.sizeW) + "x" + std::to_string(gRoiDataStc.sizeH));**/
	}

	cv::Size FrameSubProcessorRoi::getOutputSz() {
		return cv::Size(gRoiDataStc.sizeW, gRoiDataStc.sizeH);
	}

	uint8_t FrameSubProcessorRoi::getSizePercent() {
		return gRoiDataStc.percent;
	}

	void FrameSubProcessorRoi::resetData() {
		deleteRoiDataFile();
	}

	void FrameSubProcessorRoi::loadData() {
		gLoadedFromFile = loadRoiDataFromFile();
	}

	void FrameSubProcessorRoi::processFrame(cv::Mat &frame, const uint32_t frameNr) {
		if (gWriteToFileFailed) {
			return;
		}
		//
		if (gRoiDataStc.sizeW == 0 || gRoiDataStc.sizeH == 0) {
			return;
		}
		//
		gFrameNr = frameNr;
		//
		int32_t imgW = frame.cols;
		int32_t imgH = frame.rows;
		int32_t centerX = (int32_t)((imgW - 1) / 2);
		int32_t centerY = (int32_t)((imgH - 1) / 2);
		cv::Mat frameRot;

		// rotate
		///
		const float rotAngle = 90.0;
		cv::Mat transMtx = cv::getRotationMatrix2D(
				cv::Point2f(centerX, centerY),
				rotAngle,
				1
			);
		/// determine bounding rectangle, center not relevant
		cv::Rect2f bbox = cv::RotatedRect(cv::Point2f(), frame.size(), rotAngle).boundingRect2f();
		/// adjust transformation matrix
		transMtx.at<double>(0, 2) += (bbox.width / 2.0) - (imgW / 2.0);
		transMtx.at<double>(1, 2) += (bbox.height / 2.0) - (imgH / 2.0);
		///
		cv::warpAffine(frame, frameRot, transMtx, bbox.size());
		imgW = frameRot.cols;
		imgH = frameRot.rows;

		// crop
		centerX = (int32_t)((imgW - 1) / 2);
		centerY = (int32_t)((imgH - 1) / 2);
		const int32_t roiHalfW = (int32_t)(gRoiDataStc.sizeW / 2);
		const int32_t roiHalfH = (int32_t)(gRoiDataStc.sizeH / 2);
		cv::Range rowRange(centerY - roiHalfH + 1, centerY + roiHalfH);
		cv::Range colRange(centerX - roiHalfW + 1, centerX + roiHalfW);

		if (rowRange.start < 0) {
			rowRange.start = 0;
		}
		if (rowRange.end >= imgH) {
			rowRange.end = imgH - 1;
		}
		if (colRange.start < 0) {
			colRange.start = 0;
		}
		if (colRange.end >= imgW) {
			colRange.end = imgW - 1;
		}

		frame = frameRot(rowRange, colRange);
		/*
		cv::Mat frameCropped = frameRot(rowRange, colRange);

		// resize
		cv::resize(
				frameCropped,
				frame,
				gStaticOptionsStc.resolutionInputStream,
				0.0,
				0.0,
				cv::INTER_LINEAR
			);
		*/
	}

	// -----------------------------------------------------------------------------
	// -----------------------------------------------------------------------------

	void FrameSubProcessorRoi::saveRoiDataToFile() {
		if (gWriteToFileFailed) {
			return;
		}

		std::string extraQual = buildFnExtraQual();
		std::string outpFn = buildDataFilename(extraQual, false);

		log("writing RegionOfIntereset data to file '" + outpFn + "'");
		cv::FileStorage fs(outpFn, cv::FileStorage::WRITE | cv::FileStorage::FORMAT_YAML);

		//
		saveDataToFile_header(fs);

		//
		fs << "roiSizePercent" << (int32_t)gRoiDataStc.percent;

		gWriteToFileFailed = (! fcapshared::Shared::fileExists(outpFn));
	}

	bool FrameSubProcessorRoi::loadRoiDataFromFile() {
		if (gLoadedFromFile || gLoadFromFileFailed) {
			return false;
		}

		std::string extraQual = buildFnExtraQual();
		std::string inpFn = buildDataFilename(extraQual, false);

		if (! fcapshared::Shared::fileExists(inpFn)) {
			gLoadFromFileFailed = true;
			return false;
		}

		log("loading RegionOfIntereset data from file '" + inpFn + "'");

		cv::FileStorage fs(inpFn, cv::FileStorage::READ | cv::FileStorage::FORMAT_YAML);

		//
		gLoadFromFileFailed = (! loadDataFromFile_header(fs));
		if (gLoadFromFileFailed) {
			return false;
		}

		//
		gRoiDataStc.reset();

		//
		int32_t tmpInt;
		fs["roiSizePercent"] >> tmpInt;
		gRoiDataStc.percent = (uint8_t)tmpInt;

		//
		setData(gRoiDataStc.percent);

		//
		log("__reading done");
		return true;
	}

	void FrameSubProcessorRoi::deleteRoiDataFile() {
		std::string extraQual = buildFnExtraQual();
		std::string outpFn = buildDataFilename(extraQual, false);

		deleteDataFile(outpFn);
		gLoadedFromFile = false;
		gRoiDataStc.reset();
	}

	std::string FrameSubProcessorRoi::buildFnExtraQual() {
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
