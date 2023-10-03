#ifndef HTTP_CLIENTHANDLER_HPP_
#define HTTP_CLIENTHANDLER_HPP_

#include "http_handleclient_data.hpp"
#include "../cputemp/cputemp.hpp"

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
					CbGetFrameFromQueue cbGetFrameFromQueue,
					CbSetFramerateInfo cbSetFramerateInfo,
					CbGetFramerateInfo cbGetFramerateInfo);
			~ClientHandler();
			static std::thread startThread(
					const uint32_t thrIx,
					const int32_t socket,
					CbAddRunningHandler cbAddRunningHandler,
					CbRemoveRunningHandler cbRemoveRunningHandler,
					CbIncStreamingClientCount cbIncStreamingClientCount,
					CbDecStreamingClientCount cbDecStreamingClientCount,
					CbGetFrameFromQueue cbGetFrameFromQueue,
					CbSetFramerateInfo cbSetFramerateInfo,
					CbGetFramerateInfo cbGetFramerateInfo);

		private:
			httppriv::HandleClientDataStc gHndCltData;
			int32_t gClientSocket;
			std::string gRespMultipartPrefix;
			CbDecStreamingClientCount gCbDecStreamingClientCount;
			CbGetFrameFromQueue gCbGetFrameFromQueue;
			CbSetFramerateInfo gCbSetFramerateInfo;
			CbGetFramerateInfo gCbGetFramerateInfo;
			std::string gRequUriPath;
			std::string gRequUriQuery;
			static cputemp::CpuTemp gCpuTemp;

			//

			static void _startThread_internal(
					const uint32_t thrIx,
					const int32_t socket,
					CbAddRunningHandler cbAddRunningHandler,
					CbRemoveRunningHandler cbRemoveRunningHandler,
					CbIncStreamingClientCount cbIncStreamingClientCount,
					CbDecStreamingClientCount cbDecStreamingClientCount,
					CbGetFrameFromQueue cbGetFrameFromQueue,
					CbSetFramerateInfo cbSetFramerateInfo,
					CbGetFramerateInfo cbGetFramerateInfo);
			static void log(const uint32_t thrIx, const std::string &message);
			//
			void handleRequest(const char *buffer, const uint32_t bufSz);
			//
			bool checkApiKey(void *pHeaders);
			//
			std::string buildResponse(const std::string *pHttpContentType, const std::string *pContent);
			bool sendResponse(const std::string *pHttpContentType, const std::string *pContent);
			void buildJsonResult_procTrDeltaX(void *pJsonObj, const std::string key, std::map<fcapconstants::CamIdEn, cv::Point> &val);
			std::string buildJsonResult(const bool success);
			//
			void startStreaming();
			bool sendFrame(uint8_t *pData, const uint32_t bufferSz);
	};

}  // namespace http

#endif  // HTTP_CLIENTHANDLER_HPP_
