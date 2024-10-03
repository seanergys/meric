#include "rocmwrapper.h"
#include "environmentwrapper.h"

using namespace meric;

unsigned long long ROCM::COUNTERMAX = pow(2, 32);
std::map<std::string, unsigned long long> ROCM::counters;
std::vector<uint32_t> ROCM::GPUindices;
double ROCM::resolution = 0.0f;

void ROCM::init(bool detailed) {
    uint32_t deviceCount = 0;

	#ifdef HAVE_ROCM_SMI_ROCM_SMI_H
	rsmi_status_t ret;
    // Initialize ROCm SMI
    ret = rsmi_init(0);
    if (ret != RSMI_STATUS_SUCCESS) {
        std::cerr << "ROCm SMI initialization failed!" << std::endl;
        return;
    }

    // Get the number of devices
    ret = rsmi_num_monitor_devices(&deviceCount);
    if (ret != RSMI_STATUS_SUCCESS) {
        std::cerr << "ROCm SMI Failed to get the number of monitorable devices!" << std::endl;
        return;
    }

	// Populate the resolution field
	uint64_t energy;
	float precision;
	uint64_t timestamp;
	ret = rsmi_dev_energy_count_get(0, &energy, &precision, &timestamp);
	if (ret == RSMI_STATUS_SUCCESS) {
		ROCM::resolution = precision;
	}

    // Populate GPUindices
    for (uint32_t i = 0; i < deviceCount; i++) {
        GPUindices.push_back(i);
    }
	rsmi_shut_down();
    // Initialize the counter map
    initMap(counters);
	#endif
}

void ROCM::getCounterValues(std::map<std::string, unsigned long long> &record) {
    #ifdef HAVE_ROCM_SMI_ROCM_SMI_H
	rsmi_status_t ret = rsmi_init(0);
	#endif
	for (size_t i = 0; i < ROCM::GPUindices.size(); i++) {
        #ifdef HAVE_ROCM_SMI_ROCM_SMI_H	
		uint64_t energy;
		float precision;
		uint64_t timestamp;
        ret = rsmi_dev_energy_count_get(GPUindices[i], &energy, &precision, &timestamp);
        if (ret == RSMI_STATUS_SUCCESS) {
            record[std::to_string(i)] = energy;
        } else 
        #endif
			record[std::to_string(i)] = 0;
    }
	#ifdef HAVE_ROCM_SMI_ROCM_SMI_H
	rsmi_shut_down();
	#endif
}

void ROCM::initMap(std::map<std::string, unsigned long long> &dict) {
    for (size_t i = 0; i < GPUindices.size(); i++) {
        dict[std::to_string(i)] = 0;
    }
}

double ROCM::getResultValue(unsigned long long startValue, unsigned long long stopValue, double runtime, int &cumulative, std::string counter, std::string regionName) {
    cumulative = 1;
    if (stopValue < startValue) {
        MERIC_INFO << "ROCM SMI OVERFLOW, MAX VALUE: " << COUNTERMAX << std::endl;
		MERIC_INFO << "ROCM REPAIR " << startValue << " .. " << stopValue << " = " << ((COUNTERMAX - startValue) + stopValue) * getUnit(counter) << std::endl;
        return ((COUNTERMAX - startValue) + stopValue) * getUnit(counter);
    } else {
        return (stopValue - startValue) * getUnit(counter);
    }
}

std::string ROCM::getSummaryCounterName() {
    return "Energy Consumption [J]";
}

std::string ROCM::getCounterLabel(std::string counter) {
    return "GPU_"+counter+" [J]";
}

