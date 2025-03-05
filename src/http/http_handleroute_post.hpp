#ifndef HTTP_HANDLEROUTE_POST_HPP_
#define HTTP_HANDLEROUTE_POST_HPP_

#include "http_handleclient_data.hpp"
#include "http_handleroute.hpp"

namespace http {

	class HandleRoutePost : public HandleRoute {
		public:
			explicit HandleRoutePost(httppriv::HandleClientDataStc *pHndCltData);
			bool handleRequest(const std::string &requUriPath, const std::string &requUriQuery) override;

		private:
			typedef bool (HandleRoutePost::*HandleRoutePostFnc)();
			//
			std::map<const std::string, const HandleRoutePostFnc> HANDLEROUTE_LUT = {
					{"/output/cams/enable", &HandleRoutePost::_handleRoute_OUTPUT_CAMS_ENABLE},
					{"/output/cams/disable", &HandleRoutePost::_handleRoute_OUTPUT_CAMS_DISABLE},
					{"/output/cams/swap", &HandleRoutePost::_handleRoute_OUTPUT_CAMS_SWAP},
					{"/proc/bnc/brightness", &HandleRoutePost::_handleRoute_PROC_BNC_BRIGHTN},
					{"/proc/bnc/contrast", &HandleRoutePost::_handleRoute_PROC_BNC_CONTRAST},
					{"/proc/bnc/gamma", &HandleRoutePost::_handleRoute_PROC_BNC_GAMMA},
					{"/proc/cal/showchesscorners", &HandleRoutePost::_handleRoute_PROC_CAL_SHOWCHESSCORNERS},
					{"/proc/cal/start", &HandleRoutePost::_handleRoute_PROC_CAL_START},
					{"/proc/cal/reset", &HandleRoutePost::_handleRoute_PROC_CAL_RESET},
					{"/proc/grid/show", &HandleRoutePost::_handleRoute_PROC_GRID_SHOW},
					{"/proc/pt/rect_corner", &HandleRoutePost::_handleRoute_PROC_PT_RECTCORNER},
					{"/proc/pt/reset", &HandleRoutePost::_handleRoute_PROC_PT_RESET},
					{"/proc/roi/size", &HandleRoutePost::_handleRoute_PROC_ROI_SIZE},
					{"/proc/tr/fixdelta/L", &HandleRoutePost::_handleRoute_PROC_TR_FIXDELTA_L},
					{"/proc/tr/fixdelta/R", &HandleRoutePost::_handleRoute_PROC_TR_FIXDELTA_R},
					{"/proc/tr/dyndelta", &HandleRoutePost::_handleRoute_PROC_TR_DYNDELTA},
					{"/proc/tr/reset", &HandleRoutePost::_handleRoute_PROC_TR_RESET}
				};

			//

			bool _handleRoute_OUTPUT_CAMS_ENABLE();
			bool _handleRoute_OUTPUT_CAMS_DISABLE();
			bool _handleRoute_OUTPUT_CAMS_SWAP();
			bool _handleRoute_PROC_BNC_BRIGHTN();
			bool _handleRoute_PROC_BNC_CONTRAST();
			bool _handleRoute_PROC_BNC_GAMMA();
			void _handleRoute_PROC_CAL_SHOWCHESSCORNERS_x(fcapconstants::CamIdEn camId, bool newVal);
			bool _handleRoute_PROC_CAL_SHOWCHESSCORNERS();
			bool _handleRoute_PROC_CAL_START();
			bool _handleRoute_PROC_CAL_RESET();
			bool _handleRoute_PROC_GRID_SHOW();
			bool _handleRoute_PROC_PT_RECTCORNER();
			bool _handleRoute_PROC_PT_RESET();
			bool _handleRoute_PROC_ROI_SIZE();
			bool _handleRoute_PROC_TR_FIXDELTA_x(fcapconstants::CamIdEn camId);
			bool _handleRoute_PROC_TR_FIXDELTA_L();
			bool _handleRoute_PROC_TR_FIXDELTA_R();
			bool _handleRoute_PROC_TR_DYNDELTA();
			bool _handleRoute_PROC_TR_RESET();
	};

}  // namespace http

#endif  // HTTP_HANDLEROUTE_POST_HPP_
