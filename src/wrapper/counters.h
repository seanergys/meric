
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

static std::map<std::string, std::string> counterLabel = {
	{ "perf_raw::r04C6", "COUNTER_AVX" },
	{ "PAPI_L3_TCM", "COUNTER_L3_MISSES" },
	{ "PAPI_L3_TCA", "COUNTER_L3_ACCESS" },
	{ "PAPI_TOT_INS", "COUNTER_HW_INSTRUCTIONS" },
	{ "PAPI_TOT_CYC", "COUNTER_CPU_CYCLES" }
};

}

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

static std::map<std::string, std::string> counterLabel = {
	{ "COUNTER_AVX", "COUNTER_AVX" },
	{ "COUNTER_AVX_ALL", "COUNTER_AVX_ALL" },
	{ "COUNTER_CACHE_MISSES", "COUNTER_CACHE_MISSES" },
	{ "COUNTER_HW_INSTRUCTIONS", "COUNTER_HW_INSTRUCTIONS" },
	{ "PERF_COUNT_HW_CPU_CYCLES", "COUNTER_CPU_CYCLES" }
};

}

/**
 * to add new RAPL counter:
 * 	add here counter name as it is in "x86a_read -n"
 *  insert a label for the counter, that will be used in output files
 * 	init new counter inside void RAPL::initMap(std::map<std::string, long> & dict)
 * WARNING: RAPL counters are for each socket - in output file counters label will have a suffix
 * WARNING: all added counters must be ENERGY counters, RAPL time and RAPL power has different unit and RAPL::init() must be changed accordingly. Further explanation at the top of raplwrapper.cpp.
 **/
namespace RAPL_COUNTERS {

const std::string RAPL_PCKG_ENERGY = "Intel_RAPL_Pckg_Energy";
const std::string RAPL_RAM_ENERGY  = "Intel_RAPL_RAM_Energy";
const std::string RAPL_SUM_ENERGY  = "Energy summary [J]";	//PCKG+RAM = not a counter to measure
/*
const std::string RAPL_PP0_ENERGY  = "Intel_RAPL_PP0_Energy";	//Taurus doesn't support
const std::string RAPL_DRAM_CH0    = "Intel_DRAM_ENERGY_STATUS_CH0";
const std::string RAPL_DRAM_CH1    = "Intel_DRAM_ENERGY_STATUS_CH1";
const std::string RAPL_DRAM_CH2    = "Intel_DRAM_ENERGY_STATUS_CH2";
const std::string RAPL_DRAM_CH3    = "Intel_DRAM_ENERGY_STATUS_CH3";
*/

static std::map<std::string, std::string> counterLabel = {
	{ "Intel_RAPL_Pckg_Energy", "RAPL_PCKG [J]" },
	{ "Intel_RAPL_RAM_Energy", "RAPL_RAM [J]" },
	{ "Intel_RAPL_Pckg_Energy_0", "RAPL_PCKG_0 [J]" },
	{ "Intel_RAPL_Pckg_Energy_1", "RAPL_PCKG_1 [J]" },
	{ "Intel_RAPL_RAM_Energy_0", "RAPL_RAM_0 [J]" },
	{ "Intel_RAPL_RAM_Energy_1", "RAPL_RAM_1 [J]" },
	{ "Energy summary [J]", "Energy summary [J]" }
};
}


#endif /* SRC_WRAPPER_COUNTERS_H_ */
