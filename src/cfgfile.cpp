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
		json *pDefConfJson = nullptr;
		getDefaultStaticConfig((void**)&pDefConfJson);

		//
		thrLock.lock();
		try {
			std::ifstream confFileStream(*pCfgfileFn, std::ifstream::binary);

			json confFileJson = json::parse(confFileStream);
			//
			/**std::cout
					<< std::setw(4) << (*pDefConfJson) << std::endl << std::endl
					<< std::setw(4) << confFileJson << std::endl << std::endl;**/
			//
			try {
				pDefConfJson->merge_patch(confFileJson);
				//
				/**std::cout << std::setw(4) << (*pDefConfJson) << std::endl << std::endl;**/
				//
				///
				gThrVarStaticOptions.serverPort = (uint16_t)(*pDefConfJson)["server_port"];
				if (gThrVarStaticOptions.serverPort == 0) {
					throw std::invalid_argument("invalid value for server_port");
				}
				///
				gThrVarStaticOptions.resolutionInputStream = getSizeFromString(
						(std::string)(*pDefConfJson)["resolution_input_stream"],
						"resolution_input_stream"
					);
				///
				gThrVarStaticOptions.camL = getCamIdFromString(
						(std::string)(*pDefConfJson)["camera_assignment"]["left"],
						"camera_assignment[left]"
					);
				gThrVarStaticOptions.camR = getCamIdFromString(
						(std::string)(*pDefConfJson)["camera_assignment"]["right"],
						"camera_assignment[right]"
					);
				///
				gThrVarStaticOptions.camSourceType = getCamSourceFromString(
						(std::string)(*pDefConfJson)["camera_source"]["type"],
						"camera_source[type]"
					);
				///
				gThrVarStaticOptions.camSource0 = (std::string)(*pDefConfJson)["camera_source"]["cam0"];
				gThrVarStaticOptions.camSource1 = (std::string)(*pDefConfJson)["camera_source"]["cam1"];
				if (gThrVarStaticOptions.camSource0.length() == 0 && gThrVarStaticOptions.camSource1.length() == 0) {
					throw std::invalid_argument("need at least one of camera_source[cam0|cam1]");
				}
				///
				gThrVarStaticOptions.gstreamerResolutionCapture = getSizeFromString(
						(std::string)(*pDefConfJson)["camera_source"]["gstreamer"]["resolution_capture"],
						"gstreamer[resolution_capture]"
					);
				///
				gThrVarStaticOptions.cameraFps = (uint8_t)(*pDefConfJson)["camera_source"]["fps"];
				if (gThrVarStaticOptions.cameraFps == 0) {
					throw std::invalid_argument("invalid value for gstreamer[fps]");
				}
				///
				gThrVarStaticOptions.pngOutputPath = (std::string)(*pDefConfJson)["png_output_path"];
				gThrVarStaticOptions.outputPngs = (bool)(*pDefConfJson)["output_pngs"];
				///
				gThrVarStaticOptions.calibOutputPath = (std::string)(*pDefConfJson)["calib_output_path"];
				///
				gThrVarStaticOptions.procEnabled.bnc = (bool)(*pDefConfJson)["processing_enabled"]["bnc"];
				gThrVarStaticOptions.procEnabled.cal = (bool)(*pDefConfJson)["processing_enabled"]["cal"];
				gThrVarStaticOptions.procEnabled.flip = (bool)(*pDefConfJson)["processing_enabled"]["flip"];
				gThrVarStaticOptions.procEnabled.grid = (bool)(*pDefConfJson)["processing_enabled"]["grid"];
				gThrVarStaticOptions.procEnabled.pt = (bool)(*pDefConfJson)["processing_enabled"]["pt"];
				gThrVarStaticOptions.procEnabled.roi = (bool)(*pDefConfJson)["processing_enabled"]["roi"];
				gThrVarStaticOptions.procEnabled.tr = (bool)(*pDefConfJson)["processing_enabled"]["tr"];
				gThrVarStaticOptions.procEnabled.overlCam = (bool)(*pDefConfJson)["processing_enabled"]["overlay_cam"];
				gThrVarStaticOptions.procEnabled.overlCal = (bool)(*pDefConfJson)["processing_enabled"]["overlay_cal"];
				///
				gThrVarStaticOptions.enableAdaptFps = (bool)(*pDefConfJson)["enable_adaptive_fps"];
				///
				gThrVarStaticOptions.flip[fcapconstants::CamIdEn::CAM_0].hor = (bool)(*pDefConfJson)["flip"]["cam0"]["horizontal"];
				gThrVarStaticOptions.flip[fcapconstants::CamIdEn::CAM_0].ver = (bool)(*pDefConfJson)["flip"]["cam0"]["vertical"];
				gThrVarStaticOptions.flip[fcapconstants::CamIdEn::CAM_1].hor = (bool)(*pDefConfJson)["flip"]["cam1"]["horizontal"];
				gThrVarStaticOptions.flip[fcapconstants::CamIdEn::CAM_1].ver = (bool)(*pDefConfJson)["flip"]["cam1"]["vertical"];
			} catch (json::type_error &ex) {
				log("Type error while processing config file '" + fcapconstants::CONFIG_FILENAME + "'");
				thrLock.unlock();
				delete pDefConfJson;
				return false;
			} catch (std::invalid_argument &ex) {
				log("Error while processing config file '" + fcapconstants::CONFIG_FILENAME + "': " + ex.what());
				thrLock.unlock();
				delete pDefConfJson;
				return false;
			}
		} catch (json::parse_error &ex) {
			log("Parsing error in config file '" + fcapconstants::CONFIG_FILENAME + "' at byte " + std::to_string(ex.byte));
			thrLock.unlock();
			delete pDefConfJson;
			return false;
		}
		thrLock.unlock();
		delete pDefConfJson;
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

	void CfgFile::getDefaultStaticConfig(void **ppJsonObj) {
		*((json**)ppJsonObj) = new json {
				{"server_port", fcapsettings::DEFAULT_SERVER_PORT},
				{"resolution_input_stream", std::to_string(fcapsettings::DEFAULT_INPUT_SZ.width) + "x" + std::to_string(fcapsettings::DEFAULT_INPUT_SZ.height)},
				{"camera_assignment", {
						{"left", fcapconstants::CONFFILE_CAMID_0},
						{"right", fcapconstants::CONFFILE_CAMID_1}
					}},
				{"camera_source", {
						{"type", fcapconstants::CONFFILE_CAMSRC_UNSPEC},
						{"cam0", ""},
						{"cam1", ""},
						{"fps", fcapsettings::DEFAULT_FPS},
						{"gstreamer", {
								{"resolution_capture", std::to_string(fcapsettings::DEFAULT_CAPTURE_SZ.width) + "x" + std::to_string(fcapsettings::DEFAULT_CAPTURE_SZ.height)}
							}}
					}},
				{"png_output_path", "."},
				{"output_pngs", fcapsettings::DEFAULT_OUTPUT_PNGS},
				{"calib_output_path", "."},
				{"processing_enabled", {
						{"bnc", ! fcapsettings::PROC_DISABLE_ALL_PROCESSING},
						{"cal", ! fcapsettings::PROC_DISABLE_ALL_PROCESSING},
						{"flip", ! fcapsettings::PROC_DISABLE_ALL_PROCESSING},
						{"grid", ! fcapsettings::PROC_DISABLE_ALL_PROCESSING},
						{"pt", ! fcapsettings::PROC_DISABLE_ALL_PROCESSING},
						{"roi", ! fcapsettings::PROC_DISABLE_ALL_PROCESSING},
						{"tr", ! fcapsettings::PROC_DISABLE_ALL_PROCESSING},
						{"overlay_cam", ! fcapsettings::PROC_DISABLE_ALL_PROCESSING},
						{"overlay_cal", ! fcapsettings::PROC_DISABLE_ALL_PROCESSING}
					}},
				{"enable_adaptive_fps", fcapsettings::DEFAULT_ENABLE_ADAPTIVE_FPS},
				{"flip", {
						{"cam0", {
								{"horizontal", false},
								{"vertical", false}
							}},
						{"cam1", {
								{"horizontal", false},
								{"vertical", false}
							}}
					}}
			};
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
		} catch (std::exception &err) {
			intW = -1;
		}
		if (intW < 1 || intW > fcapconstants::IMAGE_SIZE_MAX) {
			throw std::invalid_argument("invalid value '" + x + "' for " + nameArg);
		}
		try {
			intH = stoi(strH);
		} catch (std::exception &err) {
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
