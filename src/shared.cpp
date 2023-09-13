#include <chrono>
#include <fstream>
#include <string>  // for stoi()
#include <stdexcept>

#include "shared.hpp"
#include "settings.hpp"
#include "json/json.hpp"

using namespace std::chrono_literals;
using json = nlohmann::json;

namespace fcapshared {

	//
	bool Shared::gThrVarNeedToStop = false;
	std::mutex Shared::gThrMtxNeedToStop;
	std::condition_variable Shared::gThrCondNeedToStop;

	//
	RuntimeOptionsStc Shared::gThrVargRuntimeOptions;
	std::mutex Shared::gThrMtxRuntimeOptions;

	//
	StaticOptionsStc Shared::gThrVargStaticOptions;
	std::mutex Shared::gThrMtxStaticOptions;

	// -----------------------------------------------------------------------------
	// -----------------------------------------------------------------------------

	void Shared::initGlobals() {
		initStcRuntimeOptions();
		initStcStaticOptions();
	}

	bool Shared::readConfigFile() {
		std::unique_lock<std::mutex> thrLock{gThrMtxStaticOptions, std::defer_lock};

		std::ifstream confFileStream("../" + fcapconstants::CONFIG_FILENAME, std::ifstream::binary);
		json defConfJson;
		std::string defConfStr = getDefaultStaticConfig();

		thrLock.lock();
		try {
			defConfJson = json::parse(defConfStr);
		} catch (json::parse_error& ex) {
			std::cerr << "parse error in DEFAULTS at byte " << ex.byte << std::endl;
			thrLock.unlock();
			return false;
		}

		try {
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
				gThrVargStaticOptions.serverPort = (uint16_t)defConfJson["server_port"];
				if (gThrVargStaticOptions.serverPort == 0) {
					throw std::invalid_argument("invalid value for server_port");
				}
				gThrVargStaticOptions.resolutionOutput = getSizeFromString((std::string)defConfJson["resolution_output"], "resolution_output");
				gThrVargStaticOptions.camL = getCamIdFromString((std::string)defConfJson["camera_assignment"]["left"], "camera_assignment[left]");
				gThrVargStaticOptions.camR = getCamIdFromString((std::string)defConfJson["camera_assignment"]["right"], "camera_assignment[right]");
				gThrVargStaticOptions.camSourceType = getCamSourceFromString((std::string)defConfJson["camera_source"]["type"], "camera_source[type]");
				gThrVargStaticOptions.camSource0 = (std::string)defConfJson["camera_source"]["cam0"];
				gThrVargStaticOptions.camSource1 = (std::string)defConfJson["camera_source"]["cam1"];
				if (gThrVargStaticOptions.camSource0.length() == 0 && gThrVargStaticOptions.camSource1.length() == 0) {
					throw std::invalid_argument("need at least one of camera_source[cam0|cam1]");
				}
				gThrVargStaticOptions.gstreamerResolutionCapture = getSizeFromString((std::string)defConfJson["camera_source"]["gstreamer"]["resolution_capture"], "gstreamer[resolution_capture]");
				gThrVargStaticOptions.cameraFps = (uint8_t)defConfJson["camera_source"]["fps"];
				if (gThrVargStaticOptions.cameraFps == 0) {
					throw std::invalid_argument("invalid value for gstreamer[fps]");
				}
				gThrVargStaticOptions.pngOutputPath = (std::string)defConfJson["png_output_path"];
				gThrVargStaticOptions.outputPngs = (bool)defConfJson["output_pngs"];
			} catch (json::type_error& ex) {
				std::cerr << "Type error while processing config file '" << fcapconstants::CONFIG_FILENAME <<
					"'" << std::endl;
				thrLock.unlock();
				return false;
			} catch (std::invalid_argument& ex) {
				std::cerr << "Error while processing config file '" << fcapconstants::CONFIG_FILENAME <<
					"': " << ex.what() << std::endl;
				thrLock.unlock();
				return false;
			}
		} catch (json::parse_error& ex) {
			std::cerr << "Parsing error in config file '" << fcapconstants::CONFIG_FILENAME <<
					"' at byte " << ex.byte << std::endl;
			thrLock.unlock();
			return false;
		}
		thrLock.unlock();
		return true;
	}

	// -----------------------------------------------------------------------------

	void Shared::setFlagNeedToStop() {
		std::unique_lock<std::mutex> thrLock{gThrMtxNeedToStop, std::defer_lock};

		thrLock.lock();
		gThrVarNeedToStop = true;
		thrLock.unlock();
		gThrCondNeedToStop.notify_all();
	}

	bool Shared::getFlagNeedToStop() {
		bool resB = false;
		std::unique_lock<std::mutex> thrLock{gThrMtxNeedToStop, std::defer_lock};

		thrLock.lock();
		if (gThrCondNeedToStop.wait_for(thrLock, 1ms, []{return gThrVarNeedToStop;})) {
			resB = true;
		}
		thrLock.unlock();
		return resB;
	}

	// -----------------------------------------------------------------------------

	RuntimeOptionsStc Shared::getRuntimeOptions() {
		RuntimeOptionsStc resStc;
		std::unique_lock<std::mutex> thrLock{gThrMtxRuntimeOptions, std::defer_lock};

		thrLock.lock();
		resStc = gThrVargRuntimeOptions;
		thrLock.unlock();
		return resStc;
	}

	void Shared::setRuntimeOptions_outputCams(fcapconstants::OutputCamsEn val) {
		std::unique_lock<std::mutex> thrLock{gThrMtxRuntimeOptions, std::defer_lock};

		thrLock.lock();
		gThrVargRuntimeOptions.outputCams = val;
		thrLock.unlock();
	}

	// -----------------------------------------------------------------------------

	StaticOptionsStc Shared::getStaticOptions() {
		StaticOptionsStc resStc;
		std::unique_lock<std::mutex> thrLock{gThrMtxStaticOptions, std::defer_lock};

		thrLock.lock();
		resStc = gThrVargStaticOptions;
		thrLock.unlock();
		return resStc;
	}

	// -----------------------------------------------------------------------------
	// -----------------------------------------------------------------------------

	void Shared::initStcRuntimeOptions() {
		std::unique_lock<std::mutex> thrLock{gThrMtxRuntimeOptions, std::defer_lock};

		thrLock.lock();
		gThrVargRuntimeOptions.outputCams = fcapconstants::OutputCamsEn::CAM_L;
		thrLock.unlock();
	}

	void Shared::initStcStaticOptions() {
		std::unique_lock<std::mutex> thrLock{gThrMtxStaticOptions, std::defer_lock};

		thrLock.lock();
		gThrVargStaticOptions.serverPort = fcapsettings::SETT_DEFAULT_SERVER_PORT;
		gThrVargStaticOptions.gstreamerResolutionCapture = fcapsettings::SETT_DEFAULT_CAPTURE_SZ;
		gThrVargStaticOptions.resolutionOutput = fcapsettings::SETT_DEFAULT_OUTPUT_SZ;
		gThrVargStaticOptions.cameraFps = fcapsettings::SETT_DEFAULT_FPS;
		gThrVargStaticOptions.camL = fcapconstants::CamIdEn::CAM_0;
		gThrVargStaticOptions.camR = fcapconstants::CamIdEn::CAM_1;
		gThrVargStaticOptions.camSourceType = fcapconstants::CamSourceEn::UNSPECIFIED;
		gThrVargStaticOptions.camSource0 = "";
		gThrVargStaticOptions.camSource1 = "";
		gThrVargStaticOptions.pngOutputPath = "";
		gThrVargStaticOptions.outputPngs = false;
		thrLock.unlock();
	}

	std::string Shared::getDefaultStaticConfig() {
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
				<< "\"png_output_path\": \"\","
				<< "\"output_pngs\": false"
			<< "}";
		return ss.str();
	}

	cv::Size Shared::getSizeFromString(const std::string &x, const std::string &nameArg) {
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

	fcapconstants::CamSourceEn Shared::getCamSourceFromString(const std::string &x, const std::string &nameArg) {
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

	fcapconstants::CamIdEn Shared::getCamIdFromString(const std::string &x, const std::string &nameArg) {
		if (x.compare(fcapconstants::CONFFILE_CAMID_0) == 0) {
			return fcapconstants::CamIdEn::CAM_0;
		}
		if (x.compare(fcapconstants::CONFFILE_CAMID_1) == 0) {
			return fcapconstants::CamIdEn::CAM_1;
		}
		throw std::invalid_argument("invalid value '" + x + "' for " + nameArg);
	}

}  // namespace fcapshared
