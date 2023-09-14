#include <stdio.h>

#include "subproc.hpp"

namespace framesubproc {

	FrameSubProcessor::FrameSubProcessor() :
			gCamId(fcapconstants::CamIdEn::CAM_0) {
	}

	FrameSubProcessor::~FrameSubProcessor() {
	}

	void FrameSubProcessor::setCamId(fcapconstants::CamIdEn camId) {
		gCamId = camId;
	}

	// -----------------------------------------------------------------------------
	// -----------------------------------------------------------------------------

	void FrameSubProcessor::log(const std::string &message) {
		std::cout << "FSUBPROC: [CAM" << (int)gCamId << "] " << message << std::endl;
	}

}  // namespace framesubproc
