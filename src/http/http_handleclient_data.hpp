#ifndef HTTP_HANDLECLIENT_DATA_HPP_
#define HTTP_HANDLECLIENT_DATA_HPP_

#include <stdio.h>

#include "../constants.hpp"
#include "../cfgfile.hpp"
#include "../shared.hpp"
#include "http_tcp_server.hpp"

namespace http {

	struct HandleClientDataStc {
		uint32_t thrIx;
		CbIncStreamingClientCount cbIncStreamingClientCount;
		fcapcfgfile::StaticOptionsStc staticOptionsStc;
		uint32_t respHttpStat;
		std::string respHttpMsgString;
		std::string respErrMsg;
		bool respReturnJson;
		bool isNewStreamingClientAccepted;
		fcapshared::RuntimeOptionsStc rtOptsCur;
		fcapshared::RuntimeOptionsStc rtOptsNew;

		HandleClientDataStc() {
			thrIx = 0;
			cbIncStreamingClientCount = NULL;
			respHttpStat = 500;
			respReturnJson = false;
			isNewStreamingClientAccepted = false;
		}

		fcapconstants::CamIdEn* curCamId() {
			return (
					rtOptsCur.outputCams == fcapconstants::OutputCamsEn::CAM_L ?
						&staticOptionsStc.camL :
							(rtOptsCur.outputCams == fcapconstants::OutputCamsEn::CAM_R ?
								&staticOptionsStc.camR :
									NULL)
				);
		}

		bool isCameraAvailabelL() {
			if (staticOptionsStc.camL == fcapconstants::CamIdEn::CAM_0) {
				return (! staticOptionsStc.camSource0.empty());
			}
			return (! staticOptionsStc.camSource1.empty());
		}

		bool isCameraAvailabelR() {
			if (staticOptionsStc.camR == fcapconstants::CamIdEn::CAM_0) {
				return (! staticOptionsStc.camSource0.empty());
			}
			return (! staticOptionsStc.camSource1.empty());
		}
	};

}  // namespace http

#endif  // HTTP_HANDLECLIENT_DATA_HPP_
