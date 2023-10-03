#include <chrono>
#include <iostream>
#include <sstream>
#include <string>  // for stoi()
#include <opencv2/opencv.hpp>

#include "../settings.hpp"
#include "http_handleroute.hpp"

using namespace std::chrono_literals;

namespace http {

	HandleRoute::HandleRoute(HandleClientDataStc *pHndCltData, const std::string &httpMethod) :
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

	void HandleRoute::_stringSplit(const std::string &valIn, const std::string &split, std::string &valOut1, std::string &valOut2) {
		if (valIn.empty()) {
			throw std::exception();
		}
		size_t posIx = valIn.find(split);
		if (posIx == std::string::npos) {
			throw std::exception();
		}
		valOut1 = valIn.substr(0, posIx);
		valOut2 = valIn.substr(posIx + 1);
	}

	std::map<std::string, std::string> HandleRoute::_getQueryParams() {
		const char *pQP = gRequUriQuery.c_str();
		std::map<std::string, std::string> resVec;
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
		std::string errDesc;

		try {
			std::map<std::string, std::string> qp = _getQueryParams();
			std::map<std::string, std::string>::iterator it;

			it = qp.find(keyX);
			if (it != qp.end()) {
				valOut.x = stoi(it->second);
			} else {
				errDesc = "missing x";
				throw std::exception();
			}

			it = qp.find(keyY);
			if (it != qp.end()) {
				valOut.y = stoi(it->second);
			} else {
				errDesc = "missing y";
				throw std::exception();
			}

			if (valOut.x < valMin.x || valOut.x > valMax.x) {
				errDesc = std::string("x out of bounds: ") + std::to_string(valMin.x) + std::string("..") + std::to_string(valMax.x);
				throw std::exception();
			}
			if (valOut.y < valMin.y || valOut.y > valMax.y) {
				errDesc = std::string("y out of bounds: ") + std::to_string(valMin.y) + std::string("..") + std::to_string(valMax.y);
				throw std::exception();
			}
		} catch (std::exception &err) {
			gPHndCltData->respErrMsg = "invalid coordinates";
			if (! errDesc.empty()) {
				gPHndCltData->respErrMsg += std::string(": ") + errDesc;
			}
			gPHndCltData->respErrMsg += std::string(" (example: 'x=1&y=2')");
			resB = false;
		}
		return resB;
	}

	bool HandleRoute::getBoolFromQuery(bool &valOut) {
		bool resB = true;
		std::string errDesc;

		try {
			std::map<std::string, std::string> qp = _getQueryParams();
			std::map<std::string, std::string>::iterator it;

			it = qp.find("v");
			if (it != qp.end()) {
				if (it->second.compare("0") != 0 && it->second.compare("1") != 0) {
					errDesc = "allowed: 0 and 1";
					throw std::exception();
				}
				valOut = (it->second.compare("1") == 0);
			} else {
				errDesc = "missing v";
				throw std::exception();
			}
		} catch (std::exception &err) {
			gPHndCltData->respErrMsg = "invalid boolean value";
			if (! errDesc.empty()) {
				gPHndCltData->respErrMsg += std::string(": ") + errDesc;
			}
			gPHndCltData->respErrMsg += std::string(" (example: 'v=0')");
			resB = false;
		}
		return resB;
	}

	bool HandleRoute::getIntFromQuery(int16_t &valOut, const int16_t valMin, const int16_t valMax) {
		bool resB = true;
		std::string errDesc;

		try {
			std::map<std::string, std::string> qp = _getQueryParams();
			std::map<std::string, std::string>::iterator it;

			it = qp.find("v");
			if (it != qp.end()) {
				int16_t tmpInt = stoi(it->second);
				if (tmpInt < valMin || tmpInt > valMax) {
					errDesc = "allowed " + std::to_string(valMin) + ".." + std::to_string(valMax);
					throw std::exception();
				}
				valOut = tmpInt;
			} else {
				errDesc = "missing v";
				throw std::exception();
			}
		} catch (std::exception &err) {
			gPHndCltData->respErrMsg = "invalid integer value";
			if (! errDesc.empty()) {
				gPHndCltData->respErrMsg += std::string(": ") + errDesc;
			}
			gPHndCltData->respErrMsg += std::string(" (example: 'v=123')");
			resB = false;
		}
		return resB;
	}

	bool HandleRoute::getOutputCamsFromQuery(fcapconstants::OutputCamsEn &valOut) {
		bool resB = true;
		std::string errDesc;

		try {
			std::map<std::string, std::string> qp = _getQueryParams();
			std::map<std::string, std::string>::iterator it;

			it = qp.find("cam");
			if (it != qp.end()) {
				if (it->second.compare("L") == 0) {
					valOut = fcapconstants::OutputCamsEn::CAM_L;
				} else if (it->second.compare("R") == 0) {
					valOut = fcapconstants::OutputCamsEn::CAM_R;
				} else if (it->second.compare("BOTH") == 0)  {
					valOut = fcapconstants::OutputCamsEn::CAM_BOTH;
				} else {
					errDesc = "allowed 'L', 'R', 'BOTH'";
					throw std::exception();
				}
			} else {
				errDesc = "missing cam";
				throw std::exception();
			}
		} catch (std::exception &err) {
			gPHndCltData->respErrMsg = "invalid camera name";
			if (! errDesc.empty()) {
				gPHndCltData->respErrMsg += std::string(": ") + errDesc;
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

}  // namespace http
