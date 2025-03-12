#include <chrono>
#include <fstream>
#include <string>  // for stoi()
#include <stdexcept>

#include "md5/md5.hpp"
#include "cfgfile.hpp"
#include "shared.hpp"
#include "json/json.hpp"

using namespace std::chrono_literals;
using json = nlohmann::json;

namespace fcapcfgfile {

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
				std::map<std::string, std::string> tmpApiKeyMap = (*pDefConfJson)["api_keys"];
				for (const auto& tmpIt : tmpApiKeyMap) {
					std::string tmpAkKey = fcapshared::Shared::toUpper(tmpIt.first);
					if (tmpAkKey == "SOMEID" || tmpAkKey == "OTHERID") {
						continue;
					}
					std::string tmpAkVal = fcapshared::Shared::toLower(tmpIt.second);
					if (tmpAkVal.empty()) {
						throw std::invalid_argument("ApiKey may not be empty");
					}
					std::string tmpMd5 = fcapconstants::CONFFILE_APIKEY_MD5PRE;
					tmpMd5 += tmpAkVal;
					tmpMd5 += fcapconstants::CONFFILE_APIKEY_MD5POST;
					for (uint32_t tmpRound = 0; tmpRound < fcapconstants::CONFFILE_APIKEY_MD5ROUNDS; tmpRound++) {
						tmpMd5 = md5::md5(tmpMd5);
					}
					gThrVarStaticOptions.apiKeys.push_back(tmpMd5);
					log("API Key Hash for '" + tmpAkKey + "' = '" + tmpMd5 + "'");
				}
				//
				///
				gThrVarStaticOptions.serverPort = static_cast<uint16_t>((*pDefConfJson)["server_port"]);
				if (gThrVarStaticOptions.serverPort == 0) {
					throw std::invalid_argument("invalid value for server_port");
				}
				///
				gThrVarStaticOptions.resolutionInputStream = getSizeFromString(
						static_cast<std::string>((*pDefConfJson)["resolution_input_stream"]),
						"resolution_input_stream"
					);
				///
				gThrVarStaticOptions.camL = getCamIdFromString(
						static_cast<std::string>((*pDefConfJson)["camera_assignment"]["left"]),
						"camera_assignment[left]"
					);
				gThrVarStaticOptions.camR = getCamIdFromString(
						static_cast<std::string>((*pDefConfJson)["camera_assignment"]["right"]),
						"camera_assignment[right]"
					);
				///
				gThrVarStaticOptions.camSourceType = getCamSourceFromString(
						static_cast<std::string>((*pDefConfJson)["camera_source"]["type"]),
						"camera_source[type]"
					);
				///
				gThrVarStaticOptions.camSource0 = static_cast<std::string>((*pDefConfJson)["camera_source"]["cam0"]);
				gThrVarStaticOptions.camSource1 = static_cast<std::string>((*pDefConfJson)["camera_source"]["cam1"]);
				if (gThrVarStaticOptions.camSource0.empty() && gThrVarStaticOptions.camSource1.empty()) {
					throw std::invalid_argument("need at least one of camera_source[cam0|cam1]");
				}
				///
				gThrVarStaticOptions.gstreamerResolutionCapture = getSizeFromString(
						static_cast<std::string>((*pDefConfJson)["camera_source"]["gstreamer"]["resolution_capture"]),
						"gstreamer[resolution_capture]"
					);
				///
				gThrVarStaticOptions.cameraFps = static_cast<uint8_t>((*pDefConfJson)["camera_source"]["fps"]);
				if (gThrVarStaticOptions.cameraFps == 0) {
					throw std::invalid_argument("invalid value for gstreamer[fps]");
				}
				///
				gThrVarStaticOptions.pngOutputPath = static_cast<std::string>((*pDefConfJson)["png_output_path"]);
				gThrVarStaticOptions.outputPngs = static_cast<bool>((*pDefConfJson)["output_pngs"]);
				///
				gThrVarStaticOptions.calibOutputPath = static_cast<std::string>((*pDefConfJson)["calib_output_path"]);
				///
				gThrVarStaticOptions.procEnabled.bnc = static_cast<bool>((*pDefConfJson)["processing_enabled"]["bnc"]);
				gThrVarStaticOptions.procEnabled.cal = static_cast<bool>((*pDefConfJson)["processing_enabled"]["cal"]);
				gThrVarStaticOptions.procEnabled.flip = static_cast<bool>((*pDefConfJson)["processing_enabled"]["flip"]);
				gThrVarStaticOptions.procEnabled.pt = static_cast<bool>((*pDefConfJson)["processing_enabled"]["pt"]);
				gThrVarStaticOptions.procEnabled.roi = static_cast<bool>((*pDefConfJson)["processing_enabled"]["roi"]);
				gThrVarStaticOptions.procEnabled.scale = static_cast<bool>((*pDefConfJson)["processing_enabled"]["scale"]);
				gThrVarStaticOptions.procEnabled.tr = static_cast<bool>((*pDefConfJson)["processing_enabled"]["tr"]);
				gThrVarStaticOptions.procEnabled.overlCam = static_cast<bool>((*pDefConfJson)["processing_enabled"]["overlay_cam"]);
				gThrVarStaticOptions.procEnabled.overlCal = static_cast<bool>((*pDefConfJson)["processing_enabled"]["overlay_cal"]);
				///
				gThrVarStaticOptions.enableAdaptFps = static_cast<bool>((*pDefConfJson)["enable_adaptive_fps"]);
				///
				gThrVarStaticOptions.flip[fcapconstants::CamIdEn::CAM_0].hor = static_cast<bool>((*pDefConfJson)["flip"]["cam0"]["horizontal"]);
				gThrVarStaticOptions.flip[fcapconstants::CamIdEn::CAM_0].ver = static_cast<bool>((*pDefConfJson)["flip"]["cam0"]["vertical"]);
				gThrVarStaticOptions.flip[fcapconstants::CamIdEn::CAM_1].hor = static_cast<bool>((*pDefConfJson)["flip"]["cam1"]["horizontal"]);
				gThrVarStaticOptions.flip[fcapconstants::CamIdEn::CAM_1].ver = static_cast<bool>((*pDefConfJson)["flip"]["cam1"]["vertical"]);
				///
				gThrVarStaticOptions.scale.resolutionOutputScaled = getSizeFromString(
						static_cast<std::string>((*pDefConfJson)["scale"]["resolution_output_scaled"]),
						"scale[resolution_output_scaled]"
					);
			} catch (json::type_error &ex) {
				log("Type error while processing config file '" + fcapconstants::CONFIG_FILENAME + "': " + ex.what());
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
				{"api_keys", {
						{"someId", "someValue"},
						{"otherId", "otherValue"}
					}},
				{"server_port", fcapsettings::TCP_DEFAULT_SERVER_PORT},
				{"resolution_input_stream",
						std::to_string(fcapsettings::STREAM_DEFAULT_INPUT_SZ.width) +
						"x" + std::to_string(fcapsettings::STREAM_DEFAULT_INPUT_SZ.height)
					},
				{"camera_assignment", {
						{"left", fcapconstants::CONFFILE_CAMID_0},
						{"right", fcapconstants::CONFFILE_CAMID_1}
					}},
				{"camera_source", {
						{"type", fcapconstants::CONFFILE_CAMSRC_UNSPEC},
						{"cam0", ""},
						{"cam1", ""},
						{"fps", fcapsettings::STREAM_DEFAULT_FPS},
						{"gstreamer", {
								{"resolution_capture",
										std::to_string(fcapsettings::STREAM_DEFAULT_CAPTURE_SZ.width) +
										"x" + std::to_string(fcapsettings::STREAM_DEFAULT_CAPTURE_SZ.height)
									}
							}}
					}},
				{"png_output_path", "."},
				{"output_pngs", false},
				{"calib_output_path", "."},
				{"processing_enabled", {
						{"bnc", true},
						{"cal", true},
						{"flip", true},
						{"pt", true},
						{"roi", true},
						{"scale", true},
						{"tr", true},
						{"overlay_cam", true},
						{"overlay_cal", true}
					}},
				{"enable_adaptive_fps", fcapsettings::STREAM_DEFAULT_ENABLE_ADAPTIVE_FPS},
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
		const size_t ix = x.find('x');
		if (ix == std::string::npos) {
			throw std::invalid_argument("invalid value '" + x + "' for " + nameArg);
		}
		const std::string strW = x.substr(0, ix);
		const std::string strH = x.substr(ix + 1);
		int intW;
		int intH;
		try {
			intW = stoi(strW);
		} catch (__attribute__((unused)) std::exception &err) {
			intW = -1;
		}
		if (intW < 1 || intW > fcapconstants::IMAGE_SIZE_MAX) {
			throw std::invalid_argument("invalid value '" + x + "' for " + nameArg);
		}
		try {
			intH = stoi(strH);
		} catch (__attribute__((unused)) std::exception &err) {
			intH = -1;
		}
		if (intH < 1 || intH > fcapconstants::IMAGE_SIZE_MAX) {
			throw std::invalid_argument("invalid value '" + x + "' for " + nameArg);
		}
		return {intW, intH};
	}

	fcapconstants::CamSourceEn CfgFile::getCamSourceFromString(const std::string &x, const std::string &nameArg) {
		if (x == fcapconstants::CONFFILE_CAMSRC_GSTR) {
			return fcapconstants::CamSourceEn::GSTREAMER;
		}
		if (x == fcapconstants::CONFFILE_CAMSRC_MJPEG) {
			return fcapconstants::CamSourceEn::MJPEG;
		}
		if (x == fcapconstants::CONFFILE_CAMSRC_UNSPEC) {
			return fcapconstants::CamSourceEn::UNSPECIFIED;
		}
		throw std::invalid_argument("invalid value '" + x + "' for " + nameArg);
	}

	fcapconstants::CamIdEn CfgFile::getCamIdFromString(const std::string &x, const std::string &nameArg) {
		if (x == fcapconstants::CONFFILE_CAMID_0) {
			return fcapconstants::CamIdEn::CAM_0;
		}
		if (x == fcapconstants::CONFFILE_CAMID_1) {
			return fcapconstants::CamIdEn::CAM_1;
		}
		throw std::invalid_argument("invalid value '" + x + "' for " + nameArg);
	}

}  // namespace fcapcfgfile
