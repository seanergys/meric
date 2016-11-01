/**
 * to add new PAPI counter:
 *	add here counter name
 *	init new conter inside PAPI::initMap(std::map<std::string, long> & dict)
 **/
namespace PAPI_COUNTERS {

const std::string COUNTER_AVX = "perf_raw::r04C6";
const std::string COUNTER_CACHE_MISSES = "PAPI_L3_TCM";
const std::string COUNTER_CACHE_ACCESS = "PAPI_L3_TCA";
const std::string COUNTER_HW_INSTRUCTIONS = "PAPI_TOT_INS";

}

/**
 * to add new PERFEVENT counter:
 *	add here counter description
 *	init new conter inside PERFEVENT::initMap(std::map<std::string, long> & dict)
 *	add counter name and type inside PERFEVENT::init()
 **/
namespace PERFEVENT_COUNTERS {

const std::string COUNTER_AVX = "COUNTER_AVX";
const std::string COUNTER_AVX_ALL = "COUNTER_AVX";
const std::string COUNTER_CAHE_MISSES = "COUNTER_CACHE_MISSES";
const std::string COUNTER_HW_INSTRUCTIONS = "COUNTER_HW_INSTRUCTIONS";

}
