#ifndef CPU_TEMP_HPP_
#define CPU_TEMP_HPP_

#include <chrono>
#include <iostream>

namespace cputemp {

	class CpuTemp {
		public:
			CpuTemp();
			~CpuTemp();
			float getTemperature();

		private:
			const std::string PATH_TEMPL = "/sys/devices/virtual/thermal/thermal_zone##NO##";
			const std::string FN_TYPE = "type";
			const std::string FN_TEMP = "temp";
			const std::string TYPE_CPU_CPU = "cpu_thermal";
			const std::string TYPE_CPU_SOC = "soc-thermal";
			const std::string TYPE_CPU_X86 = "x86_pkg_temp";
			//
			std::string gZoneFn;
			std::chrono::system_clock::time_point gLastCheck;
			float gLastTemp;

			//

			static void log(const std::string &message);
			static std::string stringReplace(const std::string &str, const std::string &from, const std::string &to);
			//
			void findThermalZone();
	};

}  // namespace cputemp

#endif  // CPU_TEMP_HPP_
