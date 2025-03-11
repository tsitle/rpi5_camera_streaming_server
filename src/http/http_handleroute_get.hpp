#ifndef HTTP_HANDLEROUTE_GET_HPP_
#define HTTP_HANDLEROUTE_GET_HPP_

#include "http_handleclient_data.hpp"
#include "http_handleroute.hpp"

namespace http {

	const std::string HTTP_URL_PATH_ROOT = "/";
	const std::string HTTP_URL_PATH_FAVICON = "/favicon.ico";

	class HandleRouteGet : public HandleRoute {
		public:
			explicit HandleRouteGet(httppriv::HandleClientDataStc *pHndCltData);
			bool handleRequest(const std::string &requUriPath, const std::string &requUriQuery) override;

		private:
			typedef bool (HandleRouteGet::*HandleRouteGetFnc)();
			//
			std::map<const std::string, const HandleRouteGetFnc> HANDLEROUTE_LUT = {
					{HTTP_URL_PATH_ROOT, &HandleRouteGet::_handleRoute_ROOT},
					{fcapconstants::HTTP_URL_PATH_STREAM, &HandleRouteGet::_handleRoute_STREAM},
					{HTTP_URL_PATH_FAVICON, &HandleRouteGet::_handleRoute_FAVICON},
					{"/status", &HandleRouteGet::_handleRoute_STATUS}
				};
			//
			bool _handleRoute_STATUS();

			//

			bool _handleRoute_ROOT();
			bool _handleRoute_STREAM();
			bool _handleRoute_FAVICON();
			//
			static std::string buildWebsite();
	};

}  // namespace http

#endif  // HTTP_HANDLEROUTE_GET_HPP_
