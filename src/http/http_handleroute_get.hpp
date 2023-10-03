#ifndef HTTP_HANDLEROUTE_GET_HPP_
#define HTTP_HANDLEROUTE_GET_HPP_

#include "http_handleclient_data.hpp"
#include "http_handleroute.hpp"

namespace http {

	class HandleRouteGet : public HandleRoute {
		public:
			HandleRouteGet(httppriv::HandleClientDataStc *pHndCltData);
			~HandleRouteGet();
			bool handleRequest(const std::string &requUriPath, const std::string &requUriQuery);

		private:
			typedef bool (HandleRouteGet::*HandleRouteGetFnc)(void);
			//
			std::map<const std::string, const HandleRouteGetFnc> HANDLEROUTE_LUT = {
					{"/", &HandleRouteGet::_handleRoute_ROOT},
					{fcapconstants::HTTP_URL_PATH_STREAM, &HandleRouteGet::_handleRoute_STREAM},
					{"/favicon.ico", &HandleRouteGet::_handleRoute_FAVICON},
					{"/status", &HandleRouteGet::_handleRoute_STATUS}
				};
			//
			bool _handleRoute_STATUS();

			//

			bool _handleRoute_ROOT();
			bool _handleRoute_STREAM();
			bool _handleRoute_FAVICON();
			//
			std::string buildWebsite();
	};

}  // namespace http

#endif  // HTTP_HANDLEROUTE_GET_HPP_
