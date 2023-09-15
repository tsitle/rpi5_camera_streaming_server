#include <stdio.h>

#include "subproc.hpp"

namespace framesubproc {

	FrameSubProcessor::FrameSubProcessor() :
			gCamId(fcapconstants::CamIdEn::CAM_0) {
		gStaticOptionsStc = fcapcfgfile::CfgFile::getStaticOptions();
	}

	FrameSubProcessor::~FrameSubProcessor() {
	}

	void FrameSubProcessor::setCamId(fcapconstants::CamIdEn camId) {
		gCamId = camId;
	}

	// -----------------------------------------------------------------------------
	// -----------------------------------------------------------------------------

	void FrameSubProcessor::log(const std::string &spName, const std::string &message) {
		std::cout << "FSUBPROC: [CAM" << (int)gCamId << "] [" + spName + "] " << message << std::endl;
	}

}  // namespace framesubproc
