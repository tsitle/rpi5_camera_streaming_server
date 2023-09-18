#include <chrono>
#include <fstream>
#include <string>  // for stoi()
#include <stdexcept>

#include "cfgfile.hpp"
#include "shared.hpp"
#include "json/json.hpp"

using namespace std::chrono_literals;
using json = nlohmann::json;

namespace fcapcfgfile {

	//
	StaticOptionsStc CfgFile::gThrVarStaticOptions;
	std::mutex CfgFile::gThrMtxStaticOptions;

	// -----------------------------------------------------------------------------
	// -----------------------------------------------------------------------------

	bool CfgFile::readConfigFile(const std::string &cfgfileFn) {
		const std::string* pCfgfileFn = &fcapconstants::CONFIG_FILENAME;
		if (! cfgfileFn.empty()) {
			pCfgfileFn = &cfgfileFn;
		}
		if (! fcapshared::Shared::fileExists(*pCfgfileFn)) {
			log("File '" + *pCfgfileFn + "' does not exist!");
			return false;
		}

		//
		std::unique_lock<std::mutex> thrLock{gThrMtxStaticOptions, std::defer_lock};

		//
		thrLock.lock();
		gThrVarStaticOptions.reset();
		thrLock.unlock();

		//
		json defConfJson;
		try {
			std::string defConfStr = getDefaultStaticConfig();

			defConfJson = json::parse(defConfStr);
		} catch (json::parse_error& ex) {
			log("parse error in DEFAULTS at byte " + std::to_string(ex.byte));
			return false;
		}

		//
		thrLock.lock();
		try {
			std::ifstream confFileStream(*pCfgfileFn, std::ifstream::binary);

			json confFileJson = json::parse(confFileStream);
			//
			/**std::cout
					<< std::setw(4) << defConfJson << std::endl << std::endl
					<< std::setw(4) << confFileJson << std::endl << std::endl;**/
			//
			try {
				defConfJson.merge_patch(confFileJson);
				//
				/**std::cout << std::setw(4) << defConfJson << std::endl << std::endl;**/
				//
				///
				gThrVarStaticOptions.serverPort = (uint16_t)defConfJson["server_port"];
				if (gThrVarStaticOptions.serverPort == 0) {
					throw std::invalid_argument("invalid value for server_port");
				}
				///
				gThrVarStaticOptions.resolutionOutput = getSizeFromString(
						(std::string)defConfJson["resolution_output"],
						"resolution_output"
					);
				///
				gThrVarStaticOptions.camL = getCamIdFromString(
						(std::string)defConfJson["camera_assignment"]["left"],
						"camera_assignment[left]"
					);
				gThrVarStaticOptions.camR = getCamIdFromString(
						(std::string)defConfJson["camera_assignment"]["right"],
						"camera_assignment[right]"
					);
				///
				gThrVarStaticOptions.camSourceType = getCamSourceFromString(
						(std::string)defConfJson["camera_source"]["type"],
						"camera_source[type]"
					);
				///
				gThrVarStaticOptions.camSource0 = (std::string)defConfJson["camera_source"]["cam0"];
				gThrVarStaticOptions.camSource1 = (std::string)defConfJson["camera_source"]["cam1"];
				if (gThrVarStaticOptions.camSource0.length() == 0 && gThrVarStaticOptions.camSource1.length() == 0) {
					throw std::invalid_argument("need at least one of camera_source[cam0|cam1]");
				}
				///
				gThrVarStaticOptions.gstreamerResolutionCapture = getSizeFromString(
						(std::string)defConfJson["camera_source"]["gstreamer"]["resolution_capture"],
						"gstreamer[resolution_capture]"
					);
				///
				gThrVarStaticOptions.cameraFps = (uint8_t)defConfJson["camera_source"]["fps"];
				if (gThrVarStaticOptions.cameraFps == 0) {
					throw std::invalid_argument("invalid value for gstreamer[fps]");
				}
				///
				gThrVarStaticOptions.pngOutputPath = (std::string)defConfJson["png_output_path"];
				gThrVarStaticOptions.outputPngs = (bool)defConfJson["output_pngs"];
				///
				gThrVarStaticOptions.calibOutputPath = (std::string)defConfJson["calib_output_path"];
				///
				gThrVarStaticOptions.procEnabled.bnc = (bool)defConfJson["processing_enabled"]["bnc"];
				gThrVarStaticOptions.procEnabled.cal = (bool)defConfJson["processing_enabled"]["cal"];
				gThrVarStaticOptions.procEnabled.pt = (bool)defConfJson["processing_enabled"]["pt"];
				gThrVarStaticOptions.procEnabled.tr = (bool)defConfJson["processing_enabled"]["tr"];
				gThrVarStaticOptions.procEnabled.overlCam = (bool)defConfJson["processing_enabled"]["overlay_cam"];
				gThrVarStaticOptions.procEnabled.overlCal = (bool)defConfJson["processing_enabled"]["overlay_cal"];
			} catch (json::type_error& ex) {
				log("Type error while processing config file '" + fcapconstants::CONFIG_FILENAME + "'");
				thrLock.unlock();
				return false;
			} catch (std::invalid_argument& ex) {
				log("Error while processing config file '" + fcapconstants::CONFIG_FILENAME + "': " + ex.what());
				thrLock.unlock();
				return false;
			}
		} catch (json::parse_error& ex) {
			log("Parsing error in config file '" + fcapconstants::CONFIG_FILENAME + "' at byte " + std::to_string(ex.byte));
			thrLock.unlock();
			return false;
		}
		thrLock.unlock();
		return true;
	}

	// -----------------------------------------------------------------------------

	StaticOptionsStc CfgFile::getStaticOptions() {
		StaticOptionsStc resStc;
		std::unique_lock<std::mutex> thrLock{gThrMtxStaticOptions, std::defer_lock};

		thrLock.lock();
		resStc = gThrVarStaticOptions;
		thrLock.unlock();
		return resStc;
	}

	// -----------------------------------------------------------------------------
	// -----------------------------------------------------------------------------

	void CfgFile::log(const std::string &message) {
		std::cout << "CFG: " << message << std::endl;
	}

	std::string CfgFile::getDefaultStaticConfig() {
		std::ostringstream ss;

		ss << "{"
				<< "\"server_port\": " << std::to_string(fcapsettings::SETT_DEFAULT_SERVER_PORT) << ","
				<< "\"resolution_output\": \""
								<< std::to_string(fcapsettings::SETT_DEFAULT_OUTPUT_SZ.width)
								<< "x"
								<< std::to_string(fcapsettings::SETT_DEFAULT_OUTPUT_SZ.height)
								<< "\","
				<< "\"camera_assignment\": {"
						<< "\"left\": \"" << fcapconstants::CONFFILE_CAMID_0 << "\","
						<< "\"right\": \"" << fcapconstants::CONFFILE_CAMID_1 << "\""
					<< "},"
				<< "\"camera_source\": {"
						<< "\"type\": \"" << fcapconstants::CONFFILE_CAMSRC_UNSPEC << "\","
						<< "\"cam0\": \"\","
						<< "\"cam1\": \"\","
						<< "\"fps\": " << std::to_string(fcapsettings::SETT_DEFAULT_FPS) << ","
						<< "\"gstreamer\": {"
								<< "\"resolution_capture\": \""
										<< std::to_string(fcapsettings::SETT_DEFAULT_CAPTURE_SZ.width)
										<< "x"
										<< std::to_string(fcapsettings::SETT_DEFAULT_CAPTURE_SZ.height)
										<< "\""
							<< "}"
					<< "},"
				<< "\"png_output_path\": \".\","
				<< "\"output_pngs\": false,"
				<< "\"calib_output_path\": \".\""
			<< "}";
		return ss.str();
	}

	cv::Size CfgFile::getSizeFromString(const std::string &x, const std::string &nameArg) {
		size_t ix = x.find("x");
		if (ix == std::string::npos) {
			throw std::invalid_argument("invalid value '" + x + "' for " + nameArg);
		}
		std::string strW = x.substr(0, ix);
		std::string strH = x.substr(ix + 1);
		int intW = -1;
		int intH = -1;
		try {
			intW = stoi(strW);
		} catch (std::exception& err) {
			intW = -1;
		}
		if (intW < 1 || intW > fcapconstants::IMAGE_SIZE_MAX) {
			throw std::invalid_argument("invalid value '" + x + "' for " + nameArg);
		}
		try {
			intH = stoi(strH);
		} catch (std::exception& err) {
			intH = -1;
		}
		if (intH < 1 || intH > fcapconstants::IMAGE_SIZE_MAX) {
			throw std::invalid_argument("invalid value '" + x + "' for " + nameArg);
		}
		return cv::Size(intW, intH);
	}

	fcapconstants::CamSourceEn CfgFile::getCamSourceFromString(const std::string &x, const std::string &nameArg) {
		if (x.compare(fcapconstants::CONFFILE_CAMSRC_GSTR) == 0) {
			return fcapconstants::CamSourceEn::GSTREAMER;
		}
		if (x.compare(fcapconstants::CONFFILE_CAMSRC_MJPEG) == 0) {
			return fcapconstants::CamSourceEn::MJPEG;
		}
		if (x.compare(fcapconstants::CONFFILE_CAMSRC_UNSPEC) == 0) {
			return fcapconstants::CamSourceEn::UNSPECIFIED;
		}
		throw std::invalid_argument("invalid value '" + x + "' for " + nameArg);
	}

	fcapconstants::CamIdEn CfgFile::getCamIdFromString(const std::string &x, const std::string &nameArg) {
		if (x.compare(fcapconstants::CONFFILE_CAMID_0) == 0) {
			return fcapconstants::CamIdEn::CAM_0;
		}
		if (x.compare(fcapconstants::CONFFILE_CAMID_1) == 0) {
			return fcapconstants::CamIdEn::CAM_1;
		}
		throw std::invalid_argument("invalid value '" + x + "' for " + nameArg);
	}

}  // namespace fcapcfgfile
