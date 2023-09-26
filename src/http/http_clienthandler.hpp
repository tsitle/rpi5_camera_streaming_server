#ifndef HTTP_CLIENTHANDLER_HPP_
#define HTTP_CLIENTHANDLER_HPP_

#include "http_handleclient_data.hpp"
#include "http_handleroute_get.hpp"

namespace http {

	class ClientHandler {
		public:
			ClientHandler(
					const uint32_t thrIx,
					const int32_t socket,
					CbAddRunningHandler cbAddRunningHandler,
					CbRemoveRunningHandler cbRemoveRunningHandler,
					CbIncStreamingClientCount cbIncStreamingClientCount,
					CbDecStreamingClientCount cbDecStreamingClientCount,
					CbGetFrameFromQueue cbGetFrameFromQueue);
			~ClientHandler();
			static std::thread startThread(
					const uint32_t thrIx,
					const int32_t socket,
					CbAddRunningHandler cbAddRunningHandler,
					CbRemoveRunningHandler cbRemoveRunningHandler,
					CbIncStreamingClientCount cbIncStreamingClientCount,
					CbDecStreamingClientCount cbDecStreamingClientCount,
					CbGetFrameFromQueue cbGetFrameFromQueue);

		private:
			HandleClientDataStc gHndCltData;
			int32_t gClientSocket;
			std::string gRespMultipartPrefix;
			CbDecStreamingClientCount gCbDecStreamingClientCount;
			CbGetFrameFromQueue gCbGetFrameFromQueue;
			std::string gRequUriPath;
			std::string gRequUriQuery;
			HandleRouteGet *gPHandleRouteGet;

			//

			static void _startThread_internal(
					const uint32_t thrIx,
					const int32_t socket,
					CbAddRunningHandler cbAddRunningHandler,
					CbRemoveRunningHandler cbRemoveRunningHandler,
					CbIncStreamingClientCount cbIncStreamingClientCount,
					CbDecStreamingClientCount cbDecStreamingClientCount,
					CbGetFrameFromQueue cbGetFrameFromQueue);
			static void log(const uint32_t thrIx, const std::string &message);
			//
			void handleRequest(const char *buffer, const uint32_t bufSz);
			//
			std::string buildResponse(const std::string *pHttpContentType, const std::string *pContent);
			bool sendResponse(const std::string *pHttpContentType, const std::string *pContent);
			void buildJsonResult_procTrDeltaX(void *pJsonObj, const std::string key, std::map<fcapconstants::CamIdEn, cv::Point> &val);
			std::string buildJsonResult(const bool success);
			//
			void startStreaming();
			bool sendFrame(uint8_t *pData, const uint32_t bufferSz);
			//
			bool isCameraAvailabelL();
			bool isCameraAvailabelR();
			//
			bool getBoolFromQuery(bool &valOut);
			bool getIntFromQuery(int16_t &valOut, const int16_t valMin, const int16_t valMax);
			bool getOutputCamsFromQuery(fcapconstants::OutputCamsEn &valOut);
			void _stringSplit(const std::string &valIn, const std::string &split, std::string &valOut1, std::string &valOut2);
			bool getCoordsFromQuery(cv::Point &valOut, const cv::Point &valMin, const cv::Point &valMax);
	};

}  // namespace http

#endif  // HTTP_CLIENTHANDLER_HPP_
