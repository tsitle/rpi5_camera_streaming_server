#include "../../settings.hpp"
#include "../../shared.hpp"
#include "subproc_bnc.hpp"

namespace framesubproc {

	constexpr double AT1_MIN_BRIGHTNESS = -100.0;
	constexpr double AT1_MAX_BRIGHTNESS = 100.0;
	constexpr double AT1_MIN_CONTRAST = 1.0;
	constexpr double AT1_MAX_CONTRAST = 3.0;
	constexpr double AT1_MIN_GAMMA = 0.5;
	constexpr double AT1_CENTER_GAMMA = 1.0;
	constexpr double AT1_MAX_GAMMA = 1.5;
	//
	constexpr int16_t AT2_MAX_BRIGHTNESS = 127;
	constexpr int16_t AT2_MIN_BRIGHTNESS = -254;
	constexpr int16_t AT2_MAX_CONTRAST = 63;
	constexpr int16_t AT2_MIN_CONTRAST = -127;

	// -----------------------------------------------------------------------------
	// -----------------------------------------------------------------------------

	FrameSubProcessorBrightnAndContrast::FrameSubProcessorBrightnAndContrast() :
			FrameSubProcessor("BNC"),
			gDblBrightn(0.0),
			gInitdBrightn(false),
			gDblContr(0.0),
			gInitdContr(false),
			gGammaLookUpTable(1, 256, CV_8U),
			gInitdGamma(false),
			gLoadedFromFile(false),
			gLoadFromFileFailed(false),
			gWriteToFileFailed(false) {
	}

	void FrameSubProcessorBrightnAndContrast::setBrightness(const int16_t val) {
		if (val >= -100 && val <= 100) {
			gBncDataStc.brightness = val;
		}
		if (gInitdBrightn && gBncDataStc.equal(gLastBncDataStc)) {
			return;
		}
		if (! gBncDataStc.equal(gLastBncDataStc) && ! gWriteToFileFailed) {
			saveBncDataToFile();
		}
		//
		double halfBr;
		double minBr;

		if (fcapsettings::PROC_BNC_USE_ALGO == fcapconstants::ProcBncAlgoEn::TYPE1) {
			halfBr = (AT1_MAX_BRIGHTNESS - AT1_MIN_BRIGHTNESS) / 2.0;
			minBr = AT1_MIN_BRIGHTNESS;
		} else {
			halfBr = (AT2_MAX_BRIGHTNESS - AT2_MIN_BRIGHTNESS) / 2.0;
			minBr = AT2_MIN_BRIGHTNESS;
		}

		gLastBncDataStc = gBncDataStc;
		gDblBrightn = (minBr + halfBr) + (halfBr * ((double)gBncDataStc.brightness / 100.0));
		gInitdBrightn = true;
	}

	void FrameSubProcessorBrightnAndContrast::setContrast(const int16_t val) {
		if (val >= -100 && val <= 100) {
			gBncDataStc.contrast = val;
		}
		if (gInitdContr && gBncDataStc.equal(gLastBncDataStc)) {
			return;
		}
		if (! gBncDataStc.equal(gLastBncDataStc) && ! gWriteToFileFailed) {
			saveBncDataToFile();
		}
		//
		double halfCo;
		double minCo;

		if (fcapsettings::PROC_BNC_USE_ALGO == fcapconstants::ProcBncAlgoEn::TYPE1) {
			halfCo = (AT1_MAX_CONTRAST - AT1_MIN_CONTRAST) / 2.0f;
			minCo = AT1_MIN_CONTRAST;
		} else {
			halfCo = (AT2_MAX_CONTRAST - AT2_MIN_CONTRAST) / 2.0f;
			minCo = AT2_MIN_CONTRAST;
		}

		gLastBncDataStc = gBncDataStc;
		gDblContr = (minCo + halfCo) + (halfCo * ((double)gBncDataStc.contrast / 100.0));
		gInitdContr = true;
	}

	void FrameSubProcessorBrightnAndContrast::setGamma(const int16_t val) {
		if (fcapsettings::PROC_BNC_USE_ALGO != fcapconstants::ProcBncAlgoEn::TYPE1) {
			return;
		}

		if (val >= -100 && val <= 100) {
			gBncDataStc.gamma = val;
		}
		if (gInitdGamma && gBncDataStc.equal(gLastBncDataStc)) {
			return;
		}
		if (! gBncDataStc.equal(gLastBncDataStc) && ! gWriteToFileFailed) {
			saveBncDataToFile();
		}
		//
		double dblGamma;
		if (gBncDataStc.gamma < 0) {
			dblGamma = AT1_CENTER_GAMMA + ((AT1_CENTER_GAMMA - AT1_MIN_GAMMA) * ((double)gBncDataStc.gamma / 100.0));
		} else {
			dblGamma = AT1_CENTER_GAMMA + ((AT1_MAX_GAMMA - AT1_CENTER_GAMMA) * ((double)gBncDataStc.gamma / 100.0));
		}

		uchar* p = gGammaLookUpTable.ptr();
		for (int16_t i = 0; i < 256; ++i) {
			p[i] = cv::saturate_cast<uchar>(pow(i / 255.0, dblGamma) * 255.0);
		}
		gLastBncDataStc = gBncDataStc;
		gInitdGamma = true;
	}

	void FrameSubProcessorBrightnAndContrast::getData(int16_t &brightn, int16_t &contr, int16_t &gamma) const {
		brightn = gBncDataStc.brightness;
		contr = gBncDataStc.contrast;
		gamma = gBncDataStc.gamma;
	}

	void FrameSubProcessorBrightnAndContrast::loadData() {
		gLoadedFromFile = loadBncDataFromFile();
	}

	void FrameSubProcessorBrightnAndContrast::processFrame(cv::Mat &frame, const uint32_t frameNr) {
		gFrameNr = frameNr;
		//
		switch (fcapsettings::PROC_BNC_USE_ALGO) {
			case fcapconstants::ProcBncAlgoEn::TYPE1:
				processFrame_algo1(frame);
				break;
			case fcapconstants::ProcBncAlgoEn::TYPE2:
				processFrame_algo2(frame);
				break;
			default:
				return;
		}
	}

	// -----------------------------------------------------------------------------
	// -----------------------------------------------------------------------------

	void FrameSubProcessorBrightnAndContrast::processFrame_algo1(cv::Mat &frame) {
		if (gBncDataStc.brightness == 0 && gBncDataStc.contrast == 0 && gBncDataStc.gamma == 0) {
			return;
		}
		if (! (gInitdBrightn && gInitdContr && gInitdGamma)) {
			return;
		}

		/**auto timeStart = std::chrono::steady_clock::now();**/

		cv::Mat frameOut;
		frame.convertTo(frameOut, -1, gDblContr, gDblBrightn);

		frame = frameOut.clone();
		cv::LUT(frameOut, gGammaLookUpTable, frame);

		/**if (gFrameNr % 10 == 0) {
			log("b " + std::to_string(gDblBrightn) + " c " + std::to_string(gDblContr));
			auto timeEnd = std::chrono::steady_clock::now();
			log("Algo1 (" + std::to_string(gBncDataStc.brightness) + "/" + std::to_string(gBncDataStc.contrast) + "/" + std::to_string(gBncDataStc.gamma) + ") Elapsed time: " +
					std::to_string(std::chrono::duration_cast<std::chrono::microseconds>(timeEnd - timeStart).count()) +
					" us");
		}**/
	}

	void FrameSubProcessorBrightnAndContrast::processFrame_algo2(cv::Mat &frame) const {
		if (gBncDataStc.brightness == 0 && gBncDataStc.contrast == 0) {
			return;
		}
		if (! (gInitdBrightn && gInitdContr)) {
			return;
		}

		/**auto timeStart = std::chrono::steady_clock::now();**/

		cv::Mat *pContrFrameInp = &frame;
		cv::Mat brightnFrameOut;
		cv::Mat contrFrameOut;
		cv::Mat *pFrameOut = &brightnFrameOut;

		if (gBncDataStc.brightness != 0) {
			const auto intBrightn = static_cast<int16_t>(gDblBrightn);
			int16_t shadow = intBrightn;
			int16_t highlight = 255;
			if (intBrightn > 0) {
				shadow = intBrightn;
				highlight = 255;
			} else {
				shadow = 0;
				highlight = 255 + intBrightn;
			}
			double alpha_b = (double)(highlight - shadow) / 255.0;
			double gamma_b = (double)shadow;
			
			/**
			 * intBrightn = 23
			 * --> a=0.909804, g=23
			 */
			/**std::cout << "B a=" << alpha_b << ", g=" << gamma_b << std::endl;**/
			cv::addWeighted(frame, alpha_b, frame, 0.0, gamma_b, *pFrameOut);
			pContrFrameInp = pFrameOut;
		}
		if (gBncDataStc.contrast != 0) {
			const auto intContr = static_cast<int16_t>(gDblContr);
			double alpha_c = 131.0 * (double)(intContr + 127) / (double)(127 * (131 - intContr));
			double gamma_c = 127.0 * (double)(1.0 - alpha_c);
			
			/**
			 * intContr = 5
			 * --> a=1.08061, g=-10.2381
			 */
			/**std::cout << "C a=" << alpha_c << ", g=" << gamma_c << std::endl;**/
			cv::addWeighted(*pContrFrameInp, alpha_c, *pContrFrameInp, 0.0, gamma_c, contrFrameOut);
			pFrameOut = &contrFrameOut;
		}
		//
		frame = pFrameOut->clone();

		/**if (gFrameNr % 10 == 0) {
			auto timeEnd = std::chrono::steady_clock::now();
			log("Algo2 Elapsed time: " +
					std::to_string(std::chrono::duration_cast<std::chrono::microseconds>(timeEnd - timeStart).count()) +
					" us");
		}**/
	}

	void FrameSubProcessorBrightnAndContrast::saveBncDataToFile() {
		if (gWriteToFileFailed) {
			return;
		}

		std::string extraQual = buildFnExtraQual();
		std::string outpFn = buildDataFilename(extraQual, false);

		log("writing Brightness/Contrast data to file '" + outpFn + "'");
		cv::FileStorage fs(outpFn, cv::FileStorage::WRITE | cv::FileStorage::FORMAT_YAML);

		//
		saveDataToFile_header(fs);

		//
		fs << "b" << gBncDataStc.brightness;
		fs << "c" << gBncDataStc.contrast;
		fs << "g" << gBncDataStc.gamma;

		gWriteToFileFailed = (! fcapshared::Shared::fileExists(outpFn));
	}

	bool FrameSubProcessorBrightnAndContrast::loadBncDataFromFile() {
		if (gLoadedFromFile || gLoadFromFileFailed) {
			return false;
		}

		std::string extraQual = buildFnExtraQual();
		std::string inpFn = buildDataFilename(extraQual, false);

		if (! fcapshared::Shared::fileExists(inpFn)) {
			gLoadFromFileFailed = true;
			return false;
		}

		log("loading Brightness/Contrast data from file '" + inpFn + "'");

		cv::FileStorage fs(inpFn, cv::FileStorage::READ | cv::FileStorage::FORMAT_YAML);

		//
		gLoadFromFileFailed = (! loadDataFromFile_header(fs));
		if (gLoadFromFileFailed) {
			return false;
		}

		//
		gBncDataStc.reset();
		fs["b"] >> gBncDataStc.brightness;
		fs["c"] >> gBncDataStc.contrast;
		fs["g"] >> gBncDataStc.gamma;
		gLastBncDataStc = gBncDataStc;

		//
		setBrightness(gBncDataStc.brightness);
		setContrast(gBncDataStc.contrast);
		setGamma(gBncDataStc.gamma);

		log("__reading done");
		return true;
	}

	void FrameSubProcessorBrightnAndContrast::deleteBncDataFile() {
		const std::string extraQual = buildFnExtraQual();
		const std::string outpFn = buildDataFilename(extraQual, false);

		deleteDataFile(outpFn);
		gLoadedFromFile = false;
		gBncDataStc.reset();
		gLastBncDataStc.reset();
	}

	std::string FrameSubProcessorBrightnAndContrast::buildFnExtraQual() {
		std::string extraQual;

		switch (fcapsettings::PROC_BNC_USE_ALGO) {
			case fcapconstants::ProcBncAlgoEn::TYPE1:
				extraQual = "t1";
				break;
			case fcapconstants::ProcBncAlgoEn::TYPE2:
				extraQual = "t2";
				break;
		}
		return extraQual;
	}

}  // namespace framesubproc
