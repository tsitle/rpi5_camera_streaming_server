#include <iostream>
#include <fstream>
#include <string>  // for stof()
#include <stdexcept>

#include "../shared.hpp"
#include "cputemp.hpp"

using namespace std::chrono_literals;

namespace cputemp {

	CpuTemp::CpuTemp() :
				gLastCheck(std::chrono::system_clock::now()),
				gLastTemp(0.0) {
		findThermalZone();
	}

	CpuTemp::~CpuTemp() {
	}

	float CpuTemp::getTemperature() {
		if (gZoneFn.empty()) {
			return 0.0;
		}
		std::chrono::system_clock::time_point now = std::chrono::system_clock::now();
		uint32_t msPast = std::chrono::duration_cast<std::chrono::milliseconds>(now - gLastCheck).count();

		if (gLastTemp > 1.0 && msPast < 5000) {
			return gLastTemp;
		}

		float resF = 0.0;
		std::ifstream fsTemp(gZoneFn);
		std::string line;

		if (std::getline(fsTemp, line) && line.length() > 2) {
			line = line.substr(0, 2) + "." + line.substr(2);
			try {
				resF = std::stof(line);
			} catch (std::invalid_argument &ex) {
				log("Error: invalid format '" + line + "'");
			}
		}
		fsTemp.close();
		//
		gLastCheck = std::chrono::system_clock::now();
		gLastTemp = resF;
		/**log("CPU Temp " + std::to_string(resF));**/
		return resF;
	}

	// -----------------------------------------------------------------------------
	// -----------------------------------------------------------------------------

	void CpuTemp::log(const std::string &message) {
		std::cout << "CPUTEMP: " << message << std::endl;
	}

	std::string CpuTemp::stringReplace(const std::string &str, const std::string &from, const std::string &to) {
		std::string resS = str;
		size_t start_pos = resS.find(from);

		if (start_pos == std::string::npos) {
			return resS;
		}
		resS.replace(start_pos, from.length(), to);
		return resS;
	}

	// -----------------------------------------------------------------------------

	void CpuTemp::findThermalZone() {
		for (uint16_t no = 0; no < 16; no++) {
			const std::string tzPath = stringReplace(PATH_TEMPL, "##NO##", std::to_string(no));
			//
			std::string fn = tzPath + std::string("/") + FN_TYPE;
			bool ex = fcapshared::Shared::fileExists(fn);
			if (! ex) {
				break;
			}
			//
			std::ifstream inpFileStream(fn);
			std::string line;
			bool valid = false;
			try {
				while (std::getline(inpFileStream, line)) {
					line = stringReplace(line, "-", "_");
					if (line.compare(stringReplace(TYPE_CPU_CPU, "-", "_")) == 0) {
						valid = true;
					} else if (line.compare(stringReplace(TYPE_CPU_SOC, "-", "_")) == 0) {
						valid = true;
					} else if (line.compare(stringReplace(TYPE_CPU_X86, "-", "_")) == 0) {
						valid = true;
					}
					break;
				}
			} catch (std::exception &err) {
				log("__Error while reading file '" + fn + "'");
			}
			inpFileStream.close();
			if (! valid) {
				continue;
			}
			//
			fn = tzPath + std::string("/") + FN_TEMP;
			if (fcapshared::Shared::fileExists(fn)) {
				gZoneFn = fn;
				break;
			}
		}
	}

}  // namespace cputemp
