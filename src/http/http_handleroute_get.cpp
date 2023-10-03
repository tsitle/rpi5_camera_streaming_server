#include <chrono>
#include <iostream>
#include <sstream>
#include <string>  // for stoi()
#include <opencv2/opencv.hpp>

#include "http_handleroute_get.hpp"

using namespace std::chrono_literals;

namespace http {

	HandleRouteGet::HandleRouteGet(httppriv::HandleClientDataStc *pHndCltData) :
				HandleRoute(pHndCltData, "GET") {
	}

	HandleRouteGet::~HandleRouteGet() {
	}

	// -----------------------------------------------------------------------------

	bool HandleRouteGet::handleRequest(const std::string &requUriPath, const std::string &requUriQuery) {
		if (! HandleRoute::handleRequest(requUriPath, requUriQuery)) {
			return false;
		}
		//
		try {
			const HandleRouteGetFnc fncPnt = HANDLEROUTE_LUT.at(gRequUriPath);

			return ((*this).*(fncPnt))();
		} catch (std::out_of_range &ex) {
			/**log("404 invalid path '" + gRequUriPath + "'");**/
			log("404 invalid path");
			gPHndCltData->respHttpStat = 404;
			return false;
		}
	}

	// -----------------------------------------------------------------------------
	// -----------------------------------------------------------------------------

	bool HandleRouteGet::_handleRoute_ROOT() {
		log("200 Path=" + gRequUriPath);
		gPHndCltData->respHttpMsgString = buildWebsite();
		return true;
	}

	bool HandleRouteGet::_handleRoute_STREAM() {
		bool resB;

		resB = getOptionalCidFromQuery(gPHndCltData->streamingClientId);
		if (resB) {
			gPHndCltData->isNewStreamingClientAccepted = gPHndCltData->cbIncStreamingClientCount(
					gPHndCltData->thrIx,
					gPHndCltData->streamingClientId
				);
			if (! gPHndCltData->isNewStreamingClientAccepted) {
				log("500 Path=" + gRequUriPath);
				log("__cannot accept more streaming clients at the moment");
				gPHndCltData->respHttpMsgString = "too many clients";
			} else {
				log("200 Path=" + gRequUriPath);
			}
		} else {
			log("500 Path=" + gRequUriPath);
			log("__Error: " + gPHndCltData->respErrMsg);
		}

		return resB;
	}

	bool HandleRouteGet::_handleRoute_FAVICON() {
		log("404 Path=" + gRequUriPath);
		gPHndCltData->respHttpStat = 404;
		return false;
	}

	bool HandleRouteGet::_handleRoute_STATUS() {
		bool resB;

		resB = getOptionalCidFromQuery(gPHndCltData->streamingClientId);
		if (resB) {
			/**log("200 Path=" + gRequUriPath);**/
			gPHndCltData->respReturnJson = true;
		} else {
			log("500 Path=" + gRequUriPath);
			log("__Error: " + gPHndCltData->respErrMsg);
		}

		return resB;
	}

	// -----------------------------------------------------------------------------

	std::string HandleRouteGet::buildWebsite() {
		std::ostringstream resS;

		resS
				<< "<!DOCTYPE html>"
				<< "<html lang=\"en\">"
				<< "<head>"
					<< "<title>" << fcapconstants::HTTP_SERVER_NAME << "</title>"
				<< "</head>"
				<< "<body>"
					<< "<h1>" << fcapconstants::HTTP_SERVER_NAME << "</h1>"
					<< "<p>"
						<< "<a href='" << fcapconstants::HTTP_URL_PATH_STREAM << "'>MJPEG Stream</a>"
					<< "</p>"
					<< "<div style='margin-top:20px'>"
						<< "<img src='" << fcapconstants::HTTP_URL_PATH_STREAM << "' width='800' height='450' />"
					<< "</div>"
				<< "</body></html>";
		return resS.str();
	}

}  // namespace http
