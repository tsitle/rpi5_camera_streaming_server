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
			gPtDataStc.ptsSrc[0] = translatePoint(gOptRectCorners[0]);
			gPtDataStc.ptsSrc[1] = translatePoint(gOptRectCorners[1]);
			gPtDataStc.ptsSrc[2] = translatePoint(gOptRectCorners[2]);
			gPtDataStc.ptsSrc[3] = translatePoint(gOptRectCorners[3]);
			// store destination rectangle corners
			gPtDataStc.ptsDst[0] = translatePoint(gOptRectCorners[4]);
			gPtDataStc.ptsDst[1] = translatePoint(gOptRectCorners[5]);
			gPtDataStc.ptsDst[2] = translatePoint(gOptRectCorners[6]);
			gPtDataStc.ptsDst[3] = translatePoint(gOptRectCorners[7]);
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

	void FrameSubProcessorPerspectiveTransf::resetData() {
		gHaveAllCorners = false;
		gHaveSomeCorners = false;
		gOptRectCorners.clear();
		deletePtDataFile();
	}

	void FrameSubProcessorPerspectiveTransf::loadData() {
		gLoadedFromFile = loadPtDataFromFile();
		if (gLoadedFromFile) {
			gHaveAllCorners = true;
		}
	}

	void FrameSubProcessorPerspectiveTransf::processFrame(cv::Mat &frame) {
		if (gWriteToFileFailed) {
			return;
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
				tmpPoint = translatePoint(tmpPoint);
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

	cv::Point FrameSubProcessorPerspectiveTransf::translatePoint(const cv::Point &pnt) {
		cv::Point resPnt = pnt;

		if (gStaticOptionsStc.procEnabled.flip) {
			if (gStaticOptionsStc.flip[gCamId].hor && ! gStaticOptionsStc.flip[gCamId].ver) {
				resPnt.x = gInpFrameSz.width - 1 - pnt.x;
				resPnt.y = pnt.y;
			} else if (! gStaticOptionsStc.flip[gCamId].hor && gStaticOptionsStc.flip[gCamId].ver) {
				resPnt.x = pnt.x;
				resPnt.y = gInpFrameSz.height - 1 - pnt.y;
			} else if (gStaticOptionsStc.flip[gCamId].hor && gStaticOptionsStc.flip[gCamId].ver) {
				resPnt.x = gInpFrameSz.width - 1 - pnt.x;
				resPnt.y = gInpFrameSz.height - 1 - pnt.y;
			}
		}
		if (gStaticOptionsStc.procEnabled.roi) {
			const uint32_t imgW = gInpFrameSz.width;
			const uint32_t imgH = gInpFrameSz.height;
			const int32_t centerX = (int32_t)((imgW - 1) / 2);
			const int32_t centerY = (int32_t)((imgH - 1) / 2);
			const float rotAngle = 90.0;

			//
			///
			cv::Mat transMtx = cv::getRotationMatrix2D(cv::Point(centerX, centerY), rotAngle, 1);
			/// determine bounding rectangle, center not relevant
			cv::Rect2f bbox = cv::RotatedRect(cv::Point2f(), gInpFrameSz, rotAngle).boundingRect2f();
			/// adjust transformation matrix
			transMtx.at<double>(0, 2) += (bbox.width / 2.0) - (imgW / 2.0);
			transMtx.at<double>(1, 2) += (bbox.height / 2.0) - (imgH / 2.0);
			std::cout << std::endl << "tm x" << bbox.x << ", y" << bbox.y << ", w" << bbox.width << ", h" << bbox.height << std::endl;

			cv::Point3d pnt3d = cv::Point3d(resPnt.x, resPnt.y, 1);
			cv::Mat pntDst = (transMtx * cv::Mat(pnt3d)).t();

			resPnt.x = pntDst.at<double>(0) - bbox.width;
			resPnt.y = pntDst.at<double>(1) + bbox.width;
			std::cout << "roi " << pnt3d << " ---> " << pntDst << " ---> " << resPnt << std::endl;
			if (resPnt.x < 0) {
				resPnt.x = 0;
				std::cout << "__roi " << resPnt << std::endl;
			} else if (resPnt.x >= imgW) {
				resPnt.x = imgW - 1;
				std::cout << "__roi " << resPnt << std::endl;
			}
			if (resPnt.y < 0) {
				resPnt.y = 0;
				std::cout << "__roi " << resPnt << std::endl;
			} else if (resPnt.y >= imgH) {
				resPnt.y = imgH - 1;
				std::cout << "__roi " << resPnt << std::endl;
			}
		}
		return resPnt;
	}

	void FrameSubProcessorPerspectiveTransf::savePtDataToFile() {
		if (gWriteToFileFailed) {
			return;
		}

		std::string extraQual = buildFnExtraQual();
		std::string outpFn = buildDataFilename("PT", extraQual);

		log("PT", "writing PersTransf data to file '" + outpFn + "'");
		cv::FileStorage fs(outpFn, cv::FileStorage::WRITE | cv::FileStorage::FORMAT_YAML);

		//
		saveDataToFile_header(fs);

		//
		for (uint8_t x = 0; x < fcapconstants::PROC_PT_RECTCORNERS_MAX; x++) {
			saveDataToFile_point2f(fs, "pt_points_src", x, gPtDataStc.ptsSrc[x]);
		}
		for (uint8_t x = 0; x < fcapconstants::PROC_PT_RECTCORNERS_MAX; x++) {
			saveDataToFile_point2f(fs, "pt_points_dst", x, gPtDataStc.ptsDst[x]);
		}

		gWriteToFileFailed = (! fcapshared::Shared::fileExists(outpFn));
	}

	bool FrameSubProcessorPerspectiveTransf::loadPtDataFromFile() {
		if (gLoadedFromFile || gLoadFromFileFailed) {
			return false;
		}

		std::string extraQual = buildFnExtraQual();
		std::string inpFn = buildDataFilename("PT", extraQual);

		if (! fcapshared::Shared::fileExists(inpFn)) {
			gLoadFromFileFailed = true;
			return false;
		}

		log("PT", "loading PersTransf data from file '" + inpFn + "'");

		cv::FileStorage fs(inpFn, cv::FileStorage::READ | cv::FileStorage::FORMAT_YAML);

		//
		gLoadFromFileFailed = (! loadDataFromFile_header("PT", fs));
		if (gLoadFromFileFailed) {
			return false;
		}

		//
		gOptRectCorners.clear();
		gPtDataStc.reset();
		for (uint8_t x = 0; x < fcapconstants::PROC_PT_RECTCORNERS_MAX; x++) {
			loadDataFromFile_point2f(fs, "pt_points_src", x, gPtDataStc.ptsSrc[x]);
			gOptRectCorners.push_back(gPtDataStc.ptsSrc[x]);
		}
		for (uint8_t x = 0; x < fcapconstants::PROC_PT_RECTCORNERS_MAX; x++) {
			loadDataFromFile_point2f(fs, "pt_points_dst", x, gPtDataStc.ptsDst[x]);
		}

		log("PT", "__reading done");
		return true;
	}

	void FrameSubProcessorPerspectiveTransf::deletePtDataFile() {
		std::string extraQual = buildFnExtraQual();
		std::string outpFn = buildDataFilename("PT", extraQual);

		deleteDataFile("PT", outpFn);
		gLoadedFromFile = false;
		gPtDataStc.reset();
	}

	std::string FrameSubProcessorPerspectiveTransf::buildFnExtraQual() {
		std::string extraQual = (gStaticOptionsStc.procEnabled.cal ? "wcal" : "");
		return extraQual;
	}

}  // namespace framesubproc
