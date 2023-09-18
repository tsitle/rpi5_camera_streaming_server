#ifndef HTTP_HANDLEROUTE_GET_HPP_
#define HTTP_HANDLEROUTE_GET_HPP_

//#include <stdlib.h>

#include "http_handleclient_data.hpp"

namespace http {

	class HandleRouteGet {
		public:
			HandleRouteGet(HandleClientDataStc *pHndCltData);
			~HandleRouteGet();
			bool handleRequest(const std::string &requUriPath, const std::string &requUriQuery);

		private:
			typedef bool (HandleRouteGet::*HandleRouteGetFnc)(void);
			//
			std::map<const std::string, const HandleRouteGetFnc> HANDLEROUTE_LUT = {
					{"/", &HandleRouteGet::_handleRoute_ROOT},
					{URL_PATH_STREAM, &HandleRouteGet::_handleRoute_STREAM},
					{"/favicon.ico", &HandleRouteGet::_handleRoute_FAVICON},
					{URL_PATH_OUTPUT_CAMS_ENABLE, &HandleRouteGet::_handleRoute_OUTPUT_CAMS_ENABLE},
					{URL_PATH_OUTPUT_CAMS_DISABLE, &HandleRouteGet::_handleRoute_OUTPUT_CAMS_DISABLE},
					{URL_PATH_OUTPUT_CAMS_SWAP, &HandleRouteGet::_handleRoute_OUTPUT_CAMS_SWAP},
					{"/proc/bnc/brightness", &HandleRouteGet::_handleRoute_PROC_BNC_BRIGHTN},
					{"/proc/bnc/contrast", &HandleRouteGet::_handleRoute_PROC_BNC_CONTRAST},
					{"/proc/cal/showchesscorners", &HandleRouteGet::_handleRoute_PROC_CAL_SHOWCHESSCORNERS},
					{"/proc/cal/reset", &HandleRouteGet::_handleRoute_PROC_CAL_RESET},
					{"/proc/pt/rect_corner", &HandleRouteGet::_handleRoute_PROC_PT_RECTCORNER},
					{"/proc/pt/reset", &HandleRouteGet::_handleRoute_PROC_PT_RESET},
					{"/proc/tr/delta", &HandleRouteGet::_handleRoute_PROC_TR_DELTA},
					{"/proc/tr/reset", &HandleRouteGet::_handleRoute_PROC_TR_RESET},
					{"/status", &HandleRouteGet::_handleRoute_STATUS}
				};
			//
			HandleClientDataStc *gPHndCltData;
			std::string gRequUriPath;
			std::string gRequUriQuery;

			//

			void log(const std::string &message);
			//
			bool _handleRoute_ROOT();
			bool _handleRoute_STREAM();
			bool _handleRoute_FAVICON();
			bool _handleRoute_OUTPUT_CAMS_ENABLE();
			bool _handleRoute_OUTPUT_CAMS_DISABLE();
			bool _handleRoute_OUTPUT_CAMS_SWAP();
			bool _handleRoute_PROC_BNC_BRIGHTN();
			bool _handleRoute_PROC_BNC_CONTRAST();
			bool _handleRoute_PROC_CAL_SHOWCHESSCORNERS();
			bool _handleRoute_PROC_CAL_RESET();
			bool _handleRoute_PROC_PT_RECTCORNER();
			bool _handleRoute_PROC_PT_RESET();
			bool _handleRoute_PROC_TR_DELTA();
			bool _handleRoute_PROC_TR_RESET();
			bool _handleRoute_STATUS();
			//
			bool isCameraAvailabelL();
			bool isCameraAvailabelR();
			//
			bool getBoolFromQuery(bool &valOut);
			bool getIntFromQuery(int16_t &valOut, const int16_t valMin, const int16_t valMax);
			bool getOutputCamsFromQuery(fcapconstants::OutputCamsEn &valOut);
			void _stringSplit(const std::string &valIn, const std::string &split, std::string &valOut1, std::string &valOut2);
			bool getCoordsFromQuery(cv::Point &valOut, const cv::Point &valMin, const cv::Point &valMax);
			//
			std::string buildWebsite();
	};

}  // namespace http

#endif  // HTTP_HANDLEROUTE_GET_HPP_
