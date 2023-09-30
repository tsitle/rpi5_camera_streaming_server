#include <stdexcept>

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

	void FrameSubProcessorPerspectiveTransf::setManualRectCorners(const std::vector<cv::Point> &val) {
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
			gOptRectCorners.push_back(val[x - 1]);
			gHaveSomeCorners = true;
		}
		if (gOptRectCorners.size() == fcapconstants::PROC_PT_RECTCORNERS_MAX) {
			cv::Point tmpPoint1 = translatePoint(getUntranslatedCorner(0));
			cv::Point tmpPoint2 = translatePoint(getUntranslatedCorner(1));
			cv::Point tmpPoint3 = translatePoint(getUntranslatedCorner(2));
			cv::Point tmpPoint4 = translatePoint(getUntranslatedCorner(3));

			// store source rectangle corners
			gPtDataStc.ptsSrc[0] = tmpPoint1;
			gPtDataStc.ptsSrc[1] = tmpPoint2;
			gPtDataStc.ptsSrc[2] = tmpPoint3;
			gPtDataStc.ptsSrc[3] = tmpPoint4;

			// compute destination rectangle corners
			gOptRectCorners.push_back(cv::Point(tmpPoint1.x, tmpPoint1.y));
			gOptRectCorners.push_back(cv::Point(tmpPoint1.x, tmpPoint2.y));
			gOptRectCorners.push_back(cv::Point(tmpPoint3.x, tmpPoint1.y));
			gOptRectCorners.push_back(cv::Point(tmpPoint3.x, tmpPoint2.y));

			// store destination rectangle corners
			gPtDataStc.ptsDst[0] = gOptRectCorners[0 + fcapconstants::PROC_PT_RECTCORNERS_MAX];
			gPtDataStc.ptsDst[1] = gOptRectCorners[1 + fcapconstants::PROC_PT_RECTCORNERS_MAX];
			gPtDataStc.ptsDst[2] = gOptRectCorners[2 + fcapconstants::PROC_PT_RECTCORNERS_MAX];
			gPtDataStc.ptsDst[3] = gOptRectCorners[3 + fcapconstants::PROC_PT_RECTCORNERS_MAX];
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

	void FrameSubProcessorPerspectiveTransf::setCalRectCorners(const std::vector<cv::Point> &val) {
		gOptRectCorners.clear();
		if (val.size() != fcapconstants::PROC_PT_RECTCORNERS_MAX) {
			gHaveAllCorners = false;
			gHaveSomeCorners = false;
			if (gLoadedFromFile) {
				deletePtDataFile();
			}
			return;
		}
		//
		cv::Point tmpPoint1 = val[0];
		cv::Point tmpPoint2 = val[1];
		cv::Point tmpPoint3 = val[2];
		cv::Point tmpPoint4 = val[3];

		//
		for (uint8_t x = 0; x < fcapconstants::PROC_PT_RECTCORNERS_MAX; x++) {
			gOptRectCorners.push_back(cv::Point(x + 1, x + 1));  // dummy
		}

		// store source rectangle corners
		gPtDataStc.ptsSrc[0] = tmpPoint1;
		gPtDataStc.ptsSrc[1] = tmpPoint2;
		gPtDataStc.ptsSrc[2] = tmpPoint3;
		gPtDataStc.ptsSrc[3] = tmpPoint4;

		// compute and store destination rectangle corners
		gPtDataStc.ptsDst[0] = cv::Point(tmpPoint1.x, tmpPoint1.y);
		gPtDataStc.ptsDst[1] = cv::Point(tmpPoint1.x, tmpPoint2.y);
		gPtDataStc.ptsDst[2] = cv::Point(tmpPoint3.x, tmpPoint1.y);
		gPtDataStc.ptsDst[3] = cv::Point(tmpPoint3.x, tmpPoint2.y);

		//
		gHaveSomeCorners = true;
		gHaveAllCorners = true;
		gLoadedFromFile = false;
		gLoadFromFileFailed = false;
		savePtDataToFile();
	}

	std::vector<cv::Point> FrameSubProcessorPerspectiveTransf::getManualRectCorners() {
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

	void FrameSubProcessorPerspectiveTransf::setRoiOutputSz(const cv::Size &val) {
		gRoiOutputSz.width = val.width;
		gRoiOutputSz.height = val.height;
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

	void FrameSubProcessorPerspectiveTransf::processFrame(cv::Mat &frame, const uint32_t frameNr) {
		if (gWriteToFileFailed) {
			return;
		}
		//
		gFrameNr = frameNr;
		//
		if (! gHaveAllCorners) {
			if (! gHaveSomeCorners) {
				return;
			}
			// draw only the first four circles
			uint8_t tmpSz = gOptRectCorners.size();
			for (uint8_t x = 1; x <= tmpSz; x++) {
				cv::Point tmpPoint = translatePoint(gOptRectCorners[x - 1]);
				cv::Scalar tmpColor = (x <= fcapconstants::PROC_PT_RECTCORNERS_MAX ? cv::Scalar(255, 0, 0) : cv::Scalar(255, 255, 0));
				cv::circle(frame, tmpPoint, 5, tmpColor, -1);
				/**log("PT", "cirle " + std::to_string(tmpPoint.x) + "/" + std::to_string(tmpPoint.y));**/
			}
			return;
		}
		//
		/**
		cv::Point tmpLastPoint = cv::Point(-1, -1);
		for (uint8_t x = 1; x <= fcapconstants::PROC_PT_RECTCORNERS_MAX; x++) {
			cv::Point tmpPoint = gPtDataStc.ptsSrc[x - 1];
			cv::Scalar tmpColor;
			switch (x) {
				case 1:
					tmpColor = cv::Scalar(255, 0, 0);  // blue
					break;
				case 2:
					tmpColor = cv::Scalar(0, 255, 0);  // green
					break;
				case 3:
					tmpColor = cv::Scalar(0, 0, 255);  // red
					break;
				default:
					tmpColor = cv::Scalar(255, 0, 255);  // purple
			}
			cv::circle(frame, tmpPoint, 5, tmpColor, -1);
			if (tmpLastPoint != cv::Point(-1, -1)) {
				cv::line(
						frame,
						tmpLastPoint,
						tmpPoint,
						cv::Scalar(0, 255, 255),
						1
					);
			}
			tmpLastPoint = tmpPoint;
		}
		**/
		//
		cv::Mat transMatrix = cv::getPerspectiveTransform(gPtDataStc.ptsSrc, gPtDataStc.ptsDst);
		cv::Mat frameIn = frame.clone();
		frame = cv::Mat();
		cv::warpPerspective(frameIn, frame, transMatrix, frame.size());
	}

	// -----------------------------------------------------------------------------
	// -----------------------------------------------------------------------------

	cv::Point FrameSubProcessorPerspectiveTransf::translatePoint(const cv::Point &pnt) {
		const uint32_t imgW = gInpFrameSz.width;
		const uint32_t imgH = gInpFrameSz.height;
		const uint32_t imgRotW = (gStaticOptionsStc.procEnabled.roi ? imgH : imgW);
		const uint32_t imgRotH = (gStaticOptionsStc.procEnabled.roi ? imgW : imgH);
		cv::Point resPnt(
				pnt.x + (gStaticOptionsStc.procEnabled.roi ? (gInpFrameSz.height - gRoiOutputSz.width) / 2 : 0),
				pnt.y + (gStaticOptionsStc.procEnabled.roi ? (gInpFrameSz.width - gRoiOutputSz.height) / 2 : 0)
			);

		/**std::cout << std::endl << "__translatePoint " << pnt << " --> " << resPnt << " delta " << (resPnt - pnt) << std::endl;**/
		if (imgW == 0 || imgH == 0) {
			log("PT", "invalid gRoiOutputSz");
			return resPnt;
		}
		if (resPnt.x < 0) {
			resPnt.x = 0;
		}
		if (resPnt.y < 0) {
			resPnt.y = 0;
		}

		if (gStaticOptionsStc.procEnabled.flip) {
			if (gStaticOptionsStc.flip[gCamId].hor && ! gStaticOptionsStc.flip[gCamId].ver) {
				if (gStaticOptionsStc.procEnabled.roi) {
					resPnt.x = resPnt.x;
					resPnt.y = imgRotH - 1 - resPnt.y;
				} else {
					resPnt.x = imgRotW - 1 - resPnt.x;
					resPnt.y = resPnt.y;
				}
			} else if (! gStaticOptionsStc.flip[gCamId].hor && gStaticOptionsStc.flip[gCamId].ver) {
				if (gStaticOptionsStc.procEnabled.roi) {
					resPnt.x = imgRotW - 1 - resPnt.x;
					resPnt.y = resPnt.y;
				} else {
					resPnt.x = resPnt.x;
					resPnt.y = imgRotH - 1 - resPnt.y;
				}
			} else if (gStaticOptionsStc.flip[gCamId].hor && gStaticOptionsStc.flip[gCamId].ver) {
				resPnt.x = imgRotW - 1 - resPnt.x;
				resPnt.y = imgRotH - 1 - resPnt.y;
			}
		}
		if (gStaticOptionsStc.procEnabled.roi) {
			const int32_t centerRotX = (int32_t)((imgRotW - 1) / 2);  // use center of rotated image
			const int32_t centerRotY = (int32_t)((imgRotH - 1) / 2);
			const float rotAngle = -90.0;

			//
			///
			/**std::cout << "__ getRotationMatrix2D (" << imgRotW << "x" << imgRotH << ")" << std::endl;**/
			cv::Mat transMtx = cv::getRotationMatrix2D(cv::Point(centerRotX, centerRotY), rotAngle, 1);
			/// determine bounding rectangle, center not relevant
			cv::Rect2f bbox = cv::RotatedRect(cv::Point2f(), cv::Size(imgRotW, imgRotH), rotAngle).boundingRect2f();
			/// adjust transformation matrix
			transMtx.at<double>(0, 2) += (bbox.width / 2.0) - (imgRotW / 2.0);
			transMtx.at<double>(1, 2) += (bbox.height / 2.0) - (imgRotH / 2.0);
			/**std::cout << "__ tm x" << bbox.x << ", y" << bbox.y << ", w" << bbox.width << ", h" << bbox.height << std::endl;**/

			cv::Point3d pnt3d = cv::Point3d(resPnt.x, resPnt.y, 1);
			cv::Mat pntDst = (transMtx * cv::Mat(pnt3d)).t();

			resPnt.x = (int32_t)pntDst.at<double>(0) /*- (gInpFrameSz.width - gRoiOutputSz.height)*/;
			resPnt.y = (int32_t)pntDst.at<double>(1) /*- (gInpFrameSz.height - gRoiOutputSz.width)*/;
			/**std::cout << "__ roi " << pnt3d << " ---> " << pntDst << " ---> ";**/
			if (resPnt.x < 0) {
				resPnt.x = 0;
			} else if (resPnt.x >= (int32_t)imgW) {
				resPnt.x = imgW - 1;
			}
			if (resPnt.y < 0) {
				resPnt.y = 0;
			} else if (resPnt.y >= (int32_t)imgH) {
				resPnt.y = imgH - 1;
			}
			/**std::cout << resPnt << std::endl;**/
		}
		return resPnt;
	}

	cv::Point FrameSubProcessorPerspectiveTransf::getUntranslatedCorner(const uint8_t ix) {
		uint8_t ixPnt1a = 0;
		uint8_t ixPnt2a = 1;
		uint8_t ixPnt3a = 2;
		uint8_t ixPnt4a = 3;

		if (gOptRectCorners.size() < fcapconstants::PROC_PT_RECTCORNERS_MAX) {
			throw std::invalid_argument("gOptRectCorners incomplete");
		}

		if (gStaticOptionsStc.procEnabled.flip &&
				gStaticOptionsStc.flip[gCamId].hor && ! gStaticOptionsStc.flip[gCamId].ver) {
			if (gStaticOptionsStc.procEnabled.roi) {
				ixPnt1a = 0;
				ixPnt2a = 2;
				ixPnt3a = 1;
				ixPnt4a = 3;
			} else {
				ixPnt1a = 2;
				ixPnt2a = 3;
				ixPnt3a = 0;
				ixPnt4a = 1;
			}
		} else if (gStaticOptionsStc.procEnabled.flip &&
				! gStaticOptionsStc.flip[gCamId].hor && gStaticOptionsStc.flip[gCamId].ver) {
			if (gStaticOptionsStc.procEnabled.roi) {
				ixPnt1a = 3;
				ixPnt2a = 1;
				ixPnt3a = 2;
				ixPnt4a = 0;
			} else {
				ixPnt1a = 1;
				ixPnt2a = 0;
				ixPnt3a = 3;
				ixPnt4a = 2;
			}
		} else if (gStaticOptionsStc.procEnabled.flip &&
				gStaticOptionsStc.flip[gCamId].hor && gStaticOptionsStc.flip[gCamId].ver) {
			if (gStaticOptionsStc.procEnabled.roi) {
				ixPnt1a = 2;
				ixPnt2a = 0;
				ixPnt3a = 3;
				ixPnt4a = 1;
			} else {
				ixPnt1a = 3;
				ixPnt2a = 2;
				ixPnt3a = 1;
				ixPnt4a = 0;
			}
		} else if (gStaticOptionsStc.procEnabled.roi) {
			ixPnt1a = 1;
			ixPnt2a = 3;
			ixPnt3a = 0;
			ixPnt4a = 2;
		}
		switch (ix) {
			case 0:
				return gOptRectCorners[ixPnt1a];
			case 1:
				return gOptRectCorners[ixPnt2a];
			case 2:
				return gOptRectCorners[ixPnt3a];
			default:
				return gOptRectCorners[ixPnt4a];
		}
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
			gOptRectCorners.push_back(cv::Point(x * 10 + 100, x * 10 + 100));  // dummy
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

		if (gStaticOptionsStc.procEnabled.roi) {
			extraQual += (extraQual.empty() ? "" : "_") + std::string("wroi");
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
