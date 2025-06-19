
#ifndef SRC_WRAPPER_COUNTERS_H_
#define SRC_WRAPPER_COUNTERS_H_

#include <cstdint>
#include <string>
#include <map>
#include <vector>

// From PAPI src https://bitbucket.org/icl/papi/src/6d0f9d3dfb45a2f4262e9f63963e34cd121cd3d1/src/components/perf_event/perf_helpers.h#lines-5
/* In case headers aren't new enough to have __NR_perf_event_open */
#ifndef __NR_perf_event_open
	#ifdef __powerpc__
		#define __NR_perf_event_open	319
	#elif defined(__x86_64__)
		#define __NR_perf_event_open	298
	#elif defined(__i386__)
		#define __NR_perf_event_open	336
	#elif defined(__arm__) || defined(__aarch64__)
		#define __NR_perf_event_open	364
	#endif
#endif

////////////////////////////////////////////////////////////////////////////////
/**
 * to add new PAPI counter:
 *	add here counter name
 *  insert a label for the counter, that will be used in output files
 *	init new conter inside PAPI::initMap(std::map<std::string, long> & dict)
 **/
namespace PAPI_COUNTERS {

const std::string COUNTER_AVX = "perf_raw::r04C6";
const std::string COUNTER_CACHE_MISSES = "PAPI_L3_TCM";
const std::string COUNTER_CACHE_ACCESS = "PAPI_L3_TCA";
const std::string COUNTER_HW_INSTRUCTIONS = "PAPI_TOT_INS";
const std::string COUNTER_FREQ = "PAPI_TOT_CYC";

static std::map<const std::string, std::string> counterLabel = {
	{ COUNTER_AVX,             "COUNTER_AVX"             },
	{ COUNTER_CACHE_MISSES,    "COUNTER_L3_MISSES"       },
	{ COUNTER_CACHE_ACCESS,    "COUNTER_L3_ACCESS"       },
	{ COUNTER_HW_INSTRUCTIONS, "COUNTER_HW_INSTRUCTIONS" },
	{ COUNTER_FREQ,            "COUNTER_CPU_CYCLES"      }
};

}
////////////////////////////////////////////////////////////////////////////////
/**
 * to add new PERFEVENT counter:
 *	add here counter description
 *  insert a label for the counter, that will be used in output files
 *	init new conter inside PERFEVENT::initMap(std::map<std::string, long> & dict)
 *	add counter name and type inside PERFEVENT::init()
 **/
namespace PERFEVENT_COUNTERS {

const std::string COUNTER_AVX = "COUNTER_AVX";
const std::string COUNTER_AVX_ALL = "COUNTER_AVX_ALL";
const std::string COUNTER_CACHE_MISSES = "COUNTER_CACHE_MISSES";
const std::string COUNTER_HW_INSTRUCTIONS = "COUNTER_HW_INSTRUCTIONS";
const std::string COUNTER_FREQ = "PERF_COUNT_HW_CPU_CYCLES";

static std::map<const std::string, std::string> counterLabel = {
#if defined(__x86_64__)
	{ COUNTER_AVX,             "COUNTER_AVX"             },
	{ COUNTER_AVX_ALL,         "COUNTER_AVX_ALL"         },
#endif
	{ COUNTER_CACHE_MISSES,    "COUNTER_CACHE_MISSES"    },
	{ COUNTER_HW_INSTRUCTIONS, "COUNTER_HW_INSTRUCTIONS" },
	{ COUNTER_FREQ,            "COUNTER_CPU_CYCLES"      }
};

}
////////////////////////////////////////////////////////////////////////////////
/**
 * to add new A64FX counter:
 *	add here counter description
 *  insert a label for the counter, that will be used in output files
 *	init new conter inside A64FX::initMap(std::map<std::string, long> & dict)
 *	add counter name and type inside A64FX::init()
 **/
namespace A64FX_COUNTERS {

const std::string ENERGY_CORE = "EA_CORE";
const std::string ENERGY_L2 = "EA_L2";
const std::string ENERGY_MEM = "EA_MEM";

static std::map<const std::string, std::string> counterLabel = {
	{ ENERGY_CORE, "CORE [J]" },
	{ ENERGY_L2,   "L2 [J]" },
	{ ENERGY_MEM,  "MEM [J]" }
};

static std::map<const std::string, const uint64_t> counterDefines = {
	{ ENERGY_CORE, 0x01e0 },
	{ ENERGY_L2,   0x03e0 },
	{ ENERGY_MEM,  0x03e8 }
};

}
////////////////////////////////////////////////////////////////////////////////
/**
 * to add new MSR register to read:
 *  the class is ready for reading the whole register without any shift or bitwise xor
 *  the counterLabel map works the opposite way than in case of the others
 *  a label of the register is a key for the register definition
 *  adder register must be inited in MSR::initMap(std::map<std::string, uint64_t> & dict) 
 *  accordig it is related to core/socket/node
 **/
namespace MSR_COUNTERS {

const std::string APERF = "APERF";
const std::string MPERF = "MPERF";
const std::string UCF_FREQ = "UCF_FREQ_CNT";
const std::string RAPL_PKG_ACTIVE = "RAPL_PKG_ACTIVE_RATIO";
const std::string CORE_THERM_STATUS = "CORE_THERM_STATUS";
const std::string PACKAGE_THERM_STATUS = "PACKAGE_THERM_STATUS";

#if defined SRC_WRAPPER_MSRS_H_
static std::map<const std::string, uint64_t> counterDefines = {
	#ifdef IA32_APERF
	{ APERF,                IA32_APERF                },
	#endif
	#ifdef IA32_MPERF
	{ MPERF,                IA32_MPERF                },
	#endif
	#ifdef U_MSR_PMON_FIXED_CTR
	{ UCF_FREQ,             U_MSR_PMON_FIXED_CTR      },
	#endif
	#ifdef MSR_PKG_PERF_STATUS
	{ RAPL_PKG_ACTIVE,      MSR_PKG_PERF_STATUS       },
	#endif
	#ifdef IA32_THERM_STATUS
	{ CORE_THERM_STATUS,    IA32_THERM_STATUS         },
	#endif
	#ifdef IA32_PACKAGE_THERM_STATUS
	{ PACKAGE_THERM_STATUS, IA32_PACKAGE_THERM_STATUS }
	#endif
};
#endif
}
////////////////////////////////////////////////////////////////////////////////
/**
 * OCC power counters
 **/
namespace OCC_COUNTERS {

/**
 * PWRSYS  = Bulk power of the system/node (500us)
 * PWRGPU  = Power consumption for GPUs per socket (OCC) read from APSS (500us)
 * PWRPROC = Power consumption for this Processor (500us)
 * PWRVDD  = Power consumption for this Processor's Vdd (calculated from AVSBus readings) (1ms)
 * PWRVDN  = Power consumption for this Processor's Vdn (nest) (calculated from AVSBus readings) (1ms)
 * PWRMEM  = Power consumption for Memory for this Processor read from APSS (500us)
 * PWRAPSSCH0 - PWRAPSSCH15 = Power Provided by APSS channel x (where x=0...15) (500us)
 *	APSS – Analog Power Subsystem Sweep, provide real time power measurements of voltage rails.
 **/
static std::map<const std::string, std::string> counterLabel = {
	{ "PWRSYS",  "Energy consumption [J]" },
	{ "PWRPROC", "CPU [J]"                       },
	{ "PWRMEM",  "MEM [J]"                       },
	{ "PWRGPU",  "GPU [J]"                       }
};
}
////////////////////////////////////////////////////////////////////////////////
/**
 * to add new RAPL counter:
 * 	add here counter name as it is in "x86a_read -n"
 *  insert a label for the counter, that will be used in output files
 * 	init new counter inside void RAPL::initMap(std::map<std::string, long> & dict)
 * WARNING: RAPL counters are for each socket - in output file counters label will have a suffix
 * WARNING: all added counters must be ENERGY counters, RAPL time and RAPL power has different unit and RAPL::init() must be changed accordingly. Further explanation at the top of raplwrapper.cpp.
 **/
namespace X86ADAPT_RAPL_COUNTERS {

const std::string RAPL_PCKG_ENERGY = "Intel_RAPL_Pckg_Energy";
const std::string RAPL_RAM_ENERGY  = "Intel_RAPL_RAM_Energy";
const std::string RAPL_SUM_ENERGY  = "Energy consumption [J]";	//Intel PCKG+RAM = not a counter to measure
/*
const std::string RAPL_PP0_ENERGY  = "Intel_RAPL_PP0_Energy";	//Haswell-EP doesn't support
const std::string RAPL_DRAM_CH0    = "Intel_DRAM_ENERGY_STATUS_CH0";
const std::string RAPL_DRAM_CH1    = "Intel_DRAM_ENERGY_STATUS_CH1";
const std::string RAPL_DRAM_CH2    = "Intel_DRAM_ENERGY_STATUS_CH2";
const std::string RAPL_DRAM_CH3    = "Intel_DRAM_ENERGY_STATUS_CH3";
const std::string RAPL_PLATFORM_ENERGY = ?
*/

const std::string RAPL_PCKG_POWER_LIMIT = "Intel_RAPL_PKG_POWER_LIMIT_1";
const std::string RAPL_RAM_POWER_LIMIT  = "Intel_RAPL_RAM_POWER_LIMIT";
// const std::string RAPL_PLATFORM_POWER_LIMIT = ?
const std::string RAPL_POWER_LIMIT_TIMEWINDOW = "Intel_RAPL_POWER_UNIT_TIMEWINDOW";

static std::map<const std::string, std::string> counterLabel = {
	{ RAPL_PCKG_ENERGY,      "PCKG [J]" },
	{ RAPL_RAM_ENERGY,       "RAM [J]"  },
	{ RAPL_SUM_ENERGY,       RAPL_SUM_ENERGY }
};
}
////////////////////////////////////////////////////////////////////////////////
namespace MSR_RAPL_COUNTERS {

//not a counter to measure
// Intel = PCKG+RAM
// AMD = PCKG
const std::string RAPL_SUM_ENERGY  = "Energy consumption [J]";

#ifdef _INTEL_
	const std::string RAPL_PCKG_ENERGY = "MSR_PKG_ENERGY_STATUS";
	const std::string RAPL_RAM_ENERGY  = "MSR_DRAM_ENERGY_STATUS";
//	const std::string RAPL_PP0_ENERGY  = "MSR_PP0_ENERGY_STATUS"; //client CPUs only
//	const std::string RAPL_PP1_ENERGY  = "MSR_PP1_ENERGY_STATUS"; //client CPUs only
	const std::string RAPL_PLATFORM_ENERGY = "MSR_PLATFORM_ENERGY_COUNTER";
#else // _AMD_
	const std::string RAPL_PCKG_ENERGY = "MSR_PKG_ENERGY_STATUS";
	const std::string RAPL_CORE_ENERGY = "MSR_CORE_ENERGY_STATUS";
#endif

// TODO _INTEL_
const std::string RAPL_PCKG_POWER_LIMIT = "MSR_PKG_POWER_LIMIT";
const std::string RAPL_RAM_POWER_LIMIT  = "MSR_DRAM_POWER_LIMIT";
const std::string RAPL_PLATFORM_POWER_LIMIT = "MSR_PLATFORM_POWER_LIMIT";
const std::string RAPL_POWER_LIMIT_TIMEWINDOW = "MSR_RAPL_POWER_UNIT_TIMEWINDOW";

static std::map<const std::string, std::string> counterLabel = {
#ifdef _INTEL_
	{ RAPL_PCKG_ENERGY,      "PCKG [J]" },
	{ RAPL_RAM_ENERGY,       "RAM [J]"  },
	{ RAPL_SUM_ENERGY,       RAPL_SUM_ENERGY }
#else // _AMD_
	{ RAPL_PCKG_ENERGY,      "PCKG [J]" },
	{ RAPL_CORE_ENERGY,      "CORE [J]" },
	{ RAPL_SUM_ENERGY,       RAPL_SUM_ENERGY }
#endif
};

#ifdef SRC_WRAPPER_MSRS_H_
static std::map<const std::string, uint64_t> counterDefines = {
	#ifdef MSR_PKG_ENERGY_STATUS
	{ RAPL_PCKG_ENERGY,      MSR_PKG_ENERGY_STATUS  },
	#endif
	#ifdef MSR_DRAM_ENERGY_STATUS
	{ RAPL_RAM_ENERGY,       MSR_DRAM_ENERGY_STATUS },
	#endif
	#ifdef MSR_PLATFORM_ENERGY_COUNTER
	{ RAPL_PLATFORM_ENERGY,  MSR_PLATFORM_ENERGY_COUNTER },
	#endif
	#ifdef MSR_CORE_ENERGY_STATUS
	{ RAPL_CORE_ENERGY,      MSR_CORE_ENERGY_STATUS },
	#endif
	{ RAPL_SUM_ENERGY,       0x0                    }
};
#endif

}
////////////////////////////////////////////////////////////////////////////////
namespace SYSFS_RAPL_COUNTERS {
const std::string RAPL_SUM_ENERGY  = "ENERGY_CONSUMPTION";

#ifdef _INTEL_
	const std::string RAPL_PCKG_ENERGY = "package";
	const std::string RAPL_RAM_ENERGY  = "dram";
//	const std::string RAPL_PP0_ENERGY  = ""; //client CPUs only
//	const std::string RAPL_PP1_ENERGY  = ""; //client CPUs only
	const std::string RAPL_PLATFORM_ENERGY = "psys";
#else // _AMD_
	const std::string RAPL_PCKG_ENERGY       = "package";
	const std::string RAPL_PCKG_CORES_ENERGY = "core";
#endif

//TODO
const std::string RAPL_PCKG_POWER_LIMIT = "";
const std::string RAPL_RAM_POWER_LIMIT  = "";
const std::string RAPL_PLATFORM_POWER_LIMIT = "";
const std::string RAPL_POWER_LIMIT_TIMEWINDOW = "";

static std::map<const std::string, std::pair<std::string, std::string> > counterLabel = {
#ifdef _INTEL_
	{ RAPL_PCKG_ENERGY,      { "PCKG", "[J]" } },
	{ RAPL_RAM_ENERGY,       { "RAM",  "[J]" } },
	{ RAPL_SUM_ENERGY,       { "Energy consumption", "[J]" } },
#else // _AMD_
	{ RAPL_PCKG_ENERGY,       { "PCKG", "[J]" } },
	{ RAPL_PCKG_CORES_ENERGY, { "PCKG_ACTIVE_CORES_AVG", "[J]" } },
	{ RAPL_SUM_ENERGY,        { "Energy consumption", "[J]" } },
#endif
};
}
////////////////////////////////////////////////////////////////////////////////
namespace PERF_RAPL_COUNTERS {
struct CounterDef_t {
	enum Domain_t {
		DOMAIN_NONE       = 0,
		DOMAIN_GLOBAL     = 1,
		DOMAIN_PER_SOCKET = 2,
		DOMAIN_PER_CORE   = 3,
		DOMAIN_PER_THREAD = 4,
	};

	enum Flags_t {
		FLAG_NONE_BIT 		  = 0,
		FLAG_ACCUMULATIVE_BIT = 1 << 0
	};

	std::vector<std::string> name;
	std::string label;
	std::string unit;
	Domain_t 	domain;
	uint32_t    flags;
	uint32_t    type;
	uint64_t    config;
};

const std::string RAPL_SUM_ENERGY = "ENERGY_CONSUMPTION";

static std::map<const std::string, CounterDef_t> counterDefs = {
#ifdef _INTEL_
	{ "PCKG", { {}, "PCKG", "[J]", CounterDef_t::DOMAIN_PER_SOCKET, CounterDef_t::FLAG_ACCUMULATIVE_BIT, 214, 0x2 } },
	{ "RAM", { {}, "RAM", "[J]", CounterDef_t::DOMAIN_PER_SOCKET, CounterDef_t::FLAG_NONE_BIT, 214, 0x3 } },
	{ RAPL_SUM_ENERGY, { {}, "Energy consumption", "[J]", CounterDef_t::DOMAIN_NONE, CounterDef_t::FLAG_NONE_BIT, 0, 0x0 }},
#else // _AMD_
	{ "PCKG", { { "power", "energy-pkg" }, "PCKG", "[J]", CounterDef_t::DOMAIN_PER_SOCKET, CounterDef_t::FLAG_ACCUMULATIVE_BIT, 0, 0x0 } },
	{ "CORE", { { "power_core", "energy-core" }, "CORE", "[J]", CounterDef_t::DOMAIN_PER_CORE, CounterDef_t::FLAG_NONE_BIT, 0, 0x0 }},
	{ RAPL_SUM_ENERGY, { {}, "Energy consumption", "[J]", CounterDef_t::DOMAIN_NONE, CounterDef_t::FLAG_NONE_BIT, 0, 0x0 }},
#endif
};
}
////////////////////////////////////////////////////////////////////////////////
namespace GEOPM_RAPL_COUNTERS {

const std::string RAPL_PCKG_ENERGY = "MSR::PKG_ENERGY_STATUS#";
const std::string RAPL_RAM_ENERGY  = "MSR::DRAM_ENERGY_STATUS#";
// const std::string RAPL_PLATFORM_ENERGY = "MSR::PLATFORM_ENERGY_COUNTER#";
const std::string RAPL_SUM_ENERGY  = "Energy consumption [J]";	//PCKG+RAM = not a counter to measure

#ifdef _AMD_
const std::string RAPL_CORE_ENERGY = "";
#endif

const std::string RAPL_PCKG_POWER_LIMIT = "MSR::PKG_POWER_LIMIT#";
const std::string RAPL_PCKG_POWER_LIMIT_1 = "MSR::PKG_POWER_LIMIT:PL1_POWER_LIMIT";
const std::string RAPL_PLATFORM_POWER_LIMIT = "MSR::PLATFORM_POWER_LIMIT#";
const std::string RAPL_RAM_POWER_LIMIT  = "MSR::DRAM_POWER_LIMIT:POWER_LIMIT";
const std::string RAPL_POWER_LIMIT_TIMEWINDOW = "MSR::PKG_POWER_LIMIT:PL1_TIME_WINDOW";

static std::map<const std::string, std::string> counterLabel = {
	{ RAPL_PCKG_ENERGY,      "PCKG [J]" },
	{ RAPL_RAM_ENERGY,       "RAM [J]"  },
	{ RAPL_SUM_ENERGY,       RAPL_SUM_ENERGY }
};
}
////////////////////////////////////////////////////////////////////////////////
namespace HWMON_COUNTERS {
const std::string HWMON_SUM_ENERGY  = "ENERGY_CONSUMPTION";
#ifdef _AMD_
const std::string HWMON_CORE_ENERGY = "Ecore";
const std::string HWMON_PCKG_ENERGY = "Esocket";
#endif

static std::map<const std::string, std::pair<std::string, std::string> > counterLabels = {
#ifdef _AMD_
	{ HWMON_CORE_ENERGY, { "CORE", "[J]" } },
	{ HWMON_PCKG_ENERGY, { "PCKG", "[J]" } },
#endif
	{ HWMON_SUM_ENERGY,  { "Energy consumption", "[J]" } }
};
}


#endif /* SRC_WRAPPER_COUNTERS_H_ */
