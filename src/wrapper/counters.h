
#ifndef SRC_WRAPPER_COUNTERS_H_
#define SRC_WRAPPER_COUNTERS_H_

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
	{ COUNTER_AVX,             "COUNTER_AVX"             },
	{ COUNTER_AVX_ALL,         "COUNTER_AVX_ALL"         },
	{ COUNTER_CACHE_MISSES,    "COUNTER_CACHE_MISSES"    },
	{ COUNTER_HW_INSTRUCTIONS, "COUNTER_HW_INSTRUCTIONS" },
	{ COUNTER_FREQ,            "COUNTER_CPU_CYCLES"      }
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

#if defined HAVE_MSR_CORE_H && defined SRC_WRAPPER_MSRCOUNTERSWRAPPER_H_
static std::map<const std::string, uint64_t> counterLabel = {
	{ APERF,    IA32_APERF                },
	{ MPERF,    IA32_MPERF                },
	{ UCF_FREQ, MSR_U_PMON_UCLK_FIXED_CTR }
};
#endif
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
const std::string RAPL_SUM_ENERGY  = "Energy consumption [J]";	//PCKG+RAM = not a counter to measure
/*
const std::string RAPL_PP0_ENERGY  = "Intel_RAPL_PP0_Energy";	//Haswell-EP doesn't support
const std::string RAPL_DRAM_CH0    = "Intel_DRAM_ENERGY_STATUS_CH0";
const std::string RAPL_DRAM_CH1    = "Intel_DRAM_ENERGY_STATUS_CH1";
const std::string RAPL_DRAM_CH2    = "Intel_DRAM_ENERGY_STATUS_CH2";
const std::string RAPL_DRAM_CH3    = "Intel_DRAM_ENERGY_STATUS_CH3";
*/

const std::string RAPL_PCKG_POWER_LIMIT = "Intel_RAPL_PKG_POWER_LIMIT_1";
const std::string RAPL_RAM_POWER_LIMIT  = "Intel_RAPL_RAM_POWER_LIMIT";
const std::string RAPL_POWER_LIMIT_TIMEWINDOW = "Intel_RAPL_POWER_UNIT_TIMEWINDOW";

static std::map<const std::string, std::string> counterLabel = {
	{ RAPL_PCKG_ENERGY,      "RAPL_PCKG [J]"          },
	{ RAPL_RAM_ENERGY,       "RAPL_RAM [J]"           },
	{ RAPL_PCKG_ENERGY+"_0", "RAPL_PCKG_0 [J]"        },
	{ RAPL_PCKG_ENERGY+"_1", "RAPL_PCKG_1 [J]"        },
	{ RAPL_RAM_ENERGY+"_0",  "RAPL_RAM_0 [J]"         },
	{ RAPL_RAM_ENERGY+"_1",  "RAPL_RAM_1 [J]"         },
	{ RAPL_SUM_ENERGY,       "Energy consumption [J]" }
};
}
////////////////////////////////////////////////////////////////////////////////
namespace MSR_RAPL_COUNTERS {

const std::string RAPL_PCKG_ENERGY = "MSR_PKG_ENERGY_STATUS";
const std::string RAPL_RAM_ENERGY  = "MSR_DRAM_ENERGY_STATUS";
const std::string RAPL_SUM_ENERGY  = "Energy consumption [J]";	//PCKG+RAM = not a counter to measure
/*
const std::string RAPL_PP0_ENERGY  = "MSR_PP0_ENERGY_STATUS"; //not available
const std::string RAPL_PP1_ENERGY  = "MSR_PP1_ENERGY_STATUS";
*/

const std::string RAPL_PCKG_POWER_LIMIT = "MSR_PKG_POWER_LIMIT";
const std::string RAPL_RAM_POWER_LIMIT  = "MSR_DRAM_POWER_LIMIT";
const std::string RAPL_POWER_LIMIT_TIMEWINDOW = "MSR_RAPL_POWER_UNIT_TIMEWINDOW";

static std::map<const std::string, std::string> counterLabel = {
	{ RAPL_PCKG_ENERGY,      "RAPL_PCKG [J]"          },
	{ RAPL_RAM_ENERGY,       "RAPL_RAM [J]"           },
	{ RAPL_PCKG_ENERGY+"_0", "RAPL_PCKG_0 [J]"        },
	{ RAPL_PCKG_ENERGY+"_1", "RAPL_PCKG_1 [J]"        },
	{ RAPL_RAM_ENERGY+"_0",  "RAPL_RAM_0 [J]"         },
	{ RAPL_RAM_ENERGY+"_1",  "RAPL_RAM_1 [J]"         },
	{ RAPL_SUM_ENERGY,       "Energy consumption [J]" }
};

#ifdef HAVE_MSR_CORE_H
#include <master.h>
static std::map<const std::string, uint64_t> counterDefines = {
	{ RAPL_PCKG_ENERGY,      MSR_PKG_ENERGY_STATUS  },
	{ RAPL_RAM_ENERGY,       MSR_DRAM_ENERGY_STATUS },
	{ RAPL_PCKG_ENERGY+"_0", MSR_PKG_ENERGY_STATUS  },
	{ RAPL_PCKG_ENERGY+"_1", MSR_PKG_ENERGY_STATUS  },
	{ RAPL_RAM_ENERGY+"_0",  MSR_DRAM_ENERGY_STATUS },
	{ RAPL_RAM_ENERGY+"_1",  MSR_DRAM_ENERGY_STATUS },
	{ RAPL_SUM_ENERGY,       0x0                    }
};
#endif

}
////////////////////////////////////////////////////////////////////////////////
namespace GEOPM_RAPL_COUNTERS {

const std::string RAPL_PCKG_ENERGY = "MSR::PKG_ENERGY_STATUS#";
const std::string RAPL_RAM_ENERGY  = "MSR::DRAM_ENERGY_STATUS#";
const std::string RAPL_SUM_ENERGY  = "Energy consumption [J]";	//PCKG+RAM = not a counter to measure

const std::string RAPL_PCKG_POWER_LIMIT = "MSR::PKG_POWER_LIMIT#";
const std::string RAPL_PCKG_POWER_LIMIT_1 = "MSR::PKG_POWER_LIMIT:PL1_POWER_LIMIT";
const std::string RAPL_RAM_POWER_LIMIT  = "MSR::DRAM_POWER_LIMIT:POWER_LIMIT";
const std::string RAPL_POWER_LIMIT_TIMEWINDOW = "MSR::PKG_POWER_LIMIT:PL1_TIME_WINDOW";

static std::map<const std::string, std::string> counterLabel = {
	{ RAPL_PCKG_ENERGY,      "RAPL_PCKG [J]"          },
	{ RAPL_RAM_ENERGY,       "RAPL_RAM [J]"           },
	{ RAPL_PCKG_ENERGY+"_0", "RAPL_PCKG_0 [J]"        },
	{ RAPL_PCKG_ENERGY+"_1", "RAPL_PCKG_1 [J]"        },
	{ RAPL_RAM_ENERGY+"_0",  "RAPL_RAM_0 [J]"         },
	{ RAPL_RAM_ENERGY+"_1",  "RAPL_RAM_1 [J]"         },
	{ RAPL_SUM_ENERGY,       "Energy consumption [J]" }
};
}


#endif /* SRC_WRAPPER_COUNTERS_H_ */
