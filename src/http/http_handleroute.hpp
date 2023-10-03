#ifndef HTTP_HANDLEROUTE_HPP_
#define HTTP_HANDLEROUTE_HPP_

#include "http_handleclient_data.hpp"

namespace http {

	class HandleRoute {
		public:
			HandleRoute(HandleClientDataStc *pHndCltData, const std::string &httpMethod);
			~HandleRoute();
			virtual bool handleRequest(const std::string &requUriPath, const std::string &requUriQuery);

		protected:
			HandleClientDataStc *gPHndCltData;
			std::string gHttpMethod;
			std::string gRequUriPath;
			std::string gRequUriQuery;

			//

			void log(const std::string &message);
			//
			void _stringSplit(const std::string &valIn, const std::string &split, std::string &valOut1, std::string &valOut2);
			std::map<std::string, std::string> _getQueryParams();
			bool _getCoordsFromQuery(const std::string &keyX, const std::string &keyY,
					cv::Point &valOut, const cv::Point &valMin, const cv::Point &valMax);
			bool getBoolFromQuery(bool &valOut);
			bool getIntFromQuery(int16_t &valOut, const int16_t valMin, const int16_t valMax);
			bool getOutputCamsFromQuery(fcapconstants::OutputCamsEn &valOut);
			bool getCoordsFromQuery(cv::Point &valOut, const cv::Point &valMin, const cv::Point &valMax);
			bool getDualCoordsFromQuery(cv::Point &valOutL, cv::Point &valOutR,
					const cv::Point &valMin, const cv::Point &valMax);
	};

}  // namespace http

#endif  // HTTP_HANDLEROUTE_HPP_
