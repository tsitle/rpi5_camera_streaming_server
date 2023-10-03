#include <chrono>
#include <iostream>
#include <limits>  // for numeric_limits
#include <sstream>
#include <string>  // for stoi()
#include <opencv2/opencv.hpp>

#include "../settings.hpp"
#include "http_handleroute.hpp"

using namespace std::chrono_literals;

namespace http {

	class QueryParamException : public std::exception {
		public:
			QueryParamException(const std::string &msg) : gMessage(msg) {};
			const char* what() {
				return gMessage.c_str();
			};

		private:
			std::string gMessage;
	};

	// -----------------------------------------------------------------------------
	// -----------------------------------------------------------------------------

	HandleRoute::HandleRoute(httppriv::HandleClientDataStc *pHndCltData, const std::string &httpMethod) :
				gPHndCltData(pHndCltData),
				gHttpMethod(httpMethod) {
	}

	HandleRoute::~HandleRoute() {
	}

	// -----------------------------------------------------------------------------

	bool HandleRoute::handleRequest(const std::string &requUriPath, const std::string &requUriQuery) {
		gRequUriPath = requUriPath;
		gRequUriQuery = requUriQuery;
		return true;
	}

	// -----------------------------------------------------------------------------
	// -----------------------------------------------------------------------------

	void HandleRoute::log(const std::string &message) {
		std::cout << "CLIENT#" << std::to_string(gPHndCltData->thrIx)
				<< " [" + gHttpMethod + "]: " << message << std::endl;
	}

	// -----------------------------------------------------------------------------

	void HandleRoute::_checkIntString(const std::string &intStr) {
		if (intStr.empty()) {
			throw std::exception();
		}
		const char *pIntStr = intStr.c_str();

		while (*pIntStr != 0) {
			if (*pIntStr < '0' || *pIntStr > '9') {
				throw std::exception();
			}
			++pIntStr;
		}
	}

	void HandleRoute::_stringSplit(const std::string &valIn, const std::string &split, std::string &valOut1, std::string &valOut2) {
		if (valIn.empty()) {
			throw QueryParamException("empty key/value pair");
		}
		size_t posIx = valIn.find(split);
		if (posIx == std::string::npos) {
			throw QueryParamException(std::string("missing '") + split + std::string("' in key/value pair"));
		}
		valOut1 = valIn.substr(0, posIx);
		valOut2 = valIn.substr(posIx + 1);
	}

	std::map<std::string, std::string> HandleRoute::_getQueryParams() {
		std::map<std::string, std::string> resVec;

		if (gRequUriQuery.empty()) {
			return resVec;
		}
		const char *pQP = gRequUriQuery.c_str();
		uint32_t posStart = 0;
		uint32_t posCur = 0;

		while (*pQP != 0) {
			if (*pQP == '&' || *(pQP + 1) == 0) {
				if (*(pQP + 1) == 0) {
					++posCur;
				}
				std::string tmpKV = gRequUriQuery.substr(posStart, posCur - posStart);
				std::string tmpK;
				std::string tmpV;
				_stringSplit(tmpKV, "=", tmpK, tmpV);
				resVec[tmpK] = tmpV;
				posStart = posCur + 1;
			}
			++pQP;
			++posCur;
		}
		return resVec;
	}

	bool HandleRoute::_getCoordsFromQuery(
			const std::string &keyX,
			const std::string &keyY,
			cv::Point &valOut,
			const cv::Point &valMin,
			const cv::Point &valMax) {
		bool resB = true;

		try {
			std::map<std::string, std::string> qp = _getQueryParams();
			std::map<std::string, std::string>::iterator it;

			it = qp.find(keyX);
			if (it != qp.end()) {
				try {
					_checkIntString(it->second);
					valOut.x = stoi(it->second);
				} catch (std::exception &err) {
					throw QueryParamException("key '" + keyX + "': cannot convert integer value");
				}
			} else {
				throw QueryParamException("missing key '" + keyX + "'");
			}

			it = qp.find(keyY);
			if (it != qp.end()) {
				try {
					_checkIntString(it->second);
					valOut.y = stoi(it->second);
				} catch (std::exception &err) {
					throw QueryParamException("key '" + keyY + "': cannot convert integer value");
				}
			} else {
				throw QueryParamException("missing key '" + keyY + "'");
			}

			if (valOut.x < valMin.x || valOut.x > valMax.x) {
				throw QueryParamException("key '" + keyX + "': out of bounds: " +
						std::to_string(valMin.x) + std::string("..") +
						std::to_string(valMax.x));
			}
			if (valOut.y < valMin.y || valOut.y > valMax.y) {
				throw QueryParamException("key '" + keyY + "': out of bounds: " +
						std::to_string(valMin.y) + std::string("..") +
						std::to_string(valMax.y));
			}
		} catch (QueryParamException &err) {
			gPHndCltData->respErrMsg = "invalid coordinates param";
			const std::string whatStr = err.what();
			if (! whatStr.empty()) {
				gPHndCltData->respErrMsg += std::string(": ") + whatStr;
			}
			gPHndCltData->respErrMsg += std::string(" (example: 'x=1&y=2')");
			resB = false;
		}
		return resB;
	}

	bool HandleRoute::getBoolFromQuery(bool &valOut) {
		bool resB = true;

		try {
			std::map<std::string, std::string> qp = _getQueryParams();
			std::map<std::string, std::string>::iterator it;

			it = qp.find("v");
			if (it != qp.end()) {
				if (it->second.compare("0") != 0 && it->second.compare("1") != 0) {
					throw QueryParamException("key 'v': invalid value: 0 and 1 allowed");
				}
				valOut = (it->second.compare("1") == 0);
			} else {
				throw QueryParamException("missing key 'v'");
			}
		} catch (QueryParamException &err) {
			gPHndCltData->respErrMsg = "invalid boolean param";
			const std::string whatStr = err.what();
			if (! whatStr.empty()) {
				gPHndCltData->respErrMsg += std::string(": ") + whatStr;
			}
			gPHndCltData->respErrMsg += std::string(" (example: 'v=0')");
			resB = false;
		}
		return resB;
	}

	bool HandleRoute::getIntFromQuery(int16_t &valOut, const int16_t valMin, const int16_t valMax) {
		bool resB = true;

		try {
			std::map<std::string, std::string> qp = _getQueryParams();
			std::map<std::string, std::string>::iterator it;

			it = qp.find("v");
			if (it != qp.end()) {
				int16_t tmpInt = 0;
				try {
					_checkIntString(it->second);
					tmpInt = stoi(it->second);
				} catch (std::exception &err) {
					throw QueryParamException("key 'v': cannot convert integer value");
				}
				if (tmpInt < valMin || tmpInt > valMax) {
					throw QueryParamException("key 'v': " +
							std::to_string(valMin) + ".." +
							std::to_string(valMax) + " allowed");
				}
				valOut = tmpInt;
			} else {
				throw QueryParamException("missing key 'v'");
			}
		} catch (QueryParamException &err) {
			gPHndCltData->respErrMsg = "invalid integer param";
			const std::string whatStr = err.what();
			if (! whatStr.empty()) {
				gPHndCltData->respErrMsg += std::string(": ") + whatStr;
			}
			gPHndCltData->respErrMsg += std::string(" (example: 'v=123')");
			resB = false;
		}
		return resB;
	}

	bool HandleRoute::getOutputCamsFromQuery(fcapconstants::OutputCamsEn &valOut) {
		bool resB = true;

		try {
			std::map<std::string, std::string> qp = _getQueryParams();
			std::map<std::string, std::string>::iterator it;

			it = qp.find("cam");
			if (it != qp.end()) {
				if (it->second.compare("L") == 0) {
					valOut = fcapconstants::OutputCamsEn::CAM_L;
				} else if (it->second.compare("R") == 0) {
					valOut = fcapconstants::OutputCamsEn::CAM_R;
				} else if (it->second.compare("BOTH") == 0) {
					valOut = fcapconstants::OutputCamsEn::CAM_BOTH;
				} else {
					throw QueryParamException("key 'cam': allowed 'L', 'R', 'BOTH'");
				}
			} else {
				throw QueryParamException("missing key 'cam'");
			}
		} catch (QueryParamException &err) {
			gPHndCltData->respErrMsg = "invalid camera param";
			const std::string whatStr = err.what();
			if (! whatStr.empty()) {
				gPHndCltData->respErrMsg += std::string(": ") + whatStr;
			}
			gPHndCltData->respErrMsg += std::string(" (example: 'cam=L')");
			resB = false;
		}
		return resB;
	}

	bool HandleRoute::getCoordsFromQuery(cv::Point &valOut, const cv::Point &valMin, const cv::Point &valMax) {
		return _getCoordsFromQuery("x", "y", valOut, valMin, valMax);
	}

	bool HandleRoute::getDualCoordsFromQuery(
			cv::Point &valOutL,
			cv::Point &valOutR,
			const cv::Point &valMin,
			const cv::Point &valMax) {
		bool resB;

		resB = _getCoordsFromQuery("Lx", "Ly", valOutL, valMin, valMax);
		if (resB) {
			resB = _getCoordsFromQuery("Rx", "Ry", valOutR, valMin, valMax);
		}
		return resB;
	}

	bool HandleRoute::getOptionalCidFromQuery(uint32_t &valOut) {
		bool resB = true;

		valOut = 0;
		try {
			std::map<std::string, std::string> qp = _getQueryParams();
			std::map<std::string, std::string>::iterator it;

			it = qp.find("cid");
			if (it != qp.end()) {
				unsigned long tmpInt = 0;
				try {
					_checkIntString(it->second);
					// max 4294967295
					tmpInt = std::strtoul(it->second.c_str(), nullptr, 10);
				} catch (std::exception &err) {
					throw QueryParamException("key 'cid': cannot convert integer value");
				}
				if (tmpInt <= std::numeric_limits<uint32_t>::max()) {
					valOut = tmpInt;
				} else {
					throw QueryParamException("key 'cid': 0.." +
							std::to_string(std::numeric_limits<uint32_t>::max()) +
							" allowed");
				}
			}
		} catch (QueryParamException &err) {
			gPHndCltData->respErrMsg = "invalid integer param";
			const std::string whatStr = err.what();
			if (! whatStr.empty()) {
				gPHndCltData->respErrMsg += std::string(": ") + whatStr;
			}
			gPHndCltData->respErrMsg += std::string(" (example: 'cid=123')");
			resB = false;
		}
		return resB;
	}

}  // namespace http
