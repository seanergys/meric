#include "rocmwrapper.h"
#include "environmentwrapper.h"

using namespace meric;

uint64_t ROCM::COUNTERMAX = __UINT64_MAX__;
std::map<std::string, uint64_t> ROCM::counters;
std::vector<uint32_t> ROCM::GPUindices;
double ROCM::unit = 1.0;

#if defined HAVE_ROCM_SMI_ROCM_SMI_H && HIP_VERSION_MAJOR <= 6 && HIP_VERSION_MINOR < 2
bool ROCM::HIP_PRIOR_6_2 = true;
#else
bool ROCM::HIP_PRIOR_6_2 = false;
#endif

void ROCM::init(bool detailed)
{


	#ifdef HAVE_ROCM_SMI_ROCM_SMI_H
	uint32_t deviceCount = 0;
	rsmi_status_t ret;
	// Initialize ROCm SMI
	ret = rsmi_init(0);
	if (ret != RSMI_STATUS_SUCCESS)
	{
		std::cerr << "MERIC ERROR: ROCm SMI initialization failed!\n";
		return;
	}

	// Get the number of devices
	ret = rsmi_num_monitor_devices(&deviceCount);
	if (ret != RSMI_STATUS_SUCCESS)
	{
		std::cerr << "MERIC ERROR: ROCm SMI failed to get the number of monitorable devices!\n";
		return;
	}

	// Read energy to identify counter unit
	uint64_t energy;
	float precision;
	uint64_t timestamp;
	ret = rsmi_dev_energy_count_get(0, &energy, &precision, &timestamp);
	if (ret == RSMI_STATUS_SUCCESS)
	{
		unit = precision * 1e-6;
	}

	// Populate GPUindices
	for (uint32_t i = 0; i < deviceCount; i++)
	{
		GPUindices.push_back(i);
	}
	
	if (HIP_PRIOR_6_2)
	{
		rsmi_shut_down();
	}
	// Initialize the counter map
	initMap(counters);
	#endif
}

void ROCM::close()
{
#ifdef HAVE_ROCM_SMI_ROCM_SMI_H
	if (HIP_PRIOR_6_2)
	{
		rsmi_shut_down();
	}
#endif
}

/**
 * For ROCm prior 6.2 it is neccessary to init and shut down with every energy reading
 * https://github.com/ROCm/rocm_smi_lib/issues/180
 **/
void ROCM::getCounterValues(std::map<std::string, uint64_t> &record)
{
	if (GPUindices.empty())
	{
		return;
	}

	#ifdef HAVE_ROCM_SMI_ROCM_SMI_H
	if (HIP_PRIOR_6_2 && rsmi_init(0) != RSMI_STATUS_SUCCESS)
	{
		std::cerr << "MERIC ERROR: ROCm SMI initialization failed!\n";
		return;
	}
	for (size_t i = 0; i < GPUindices.size(); i++)
	{
		uint64_t energy;
		float precision;
		uint64_t timestamp;
		if (rsmi_dev_energy_count_get(GPUindices[i], &energy, &precision, &timestamp) == RSMI_STATUS_SUCCESS)
		{
			record[std::to_string(i)] = energy;
		}
		else
		{
			record[std::to_string(i)] = 0;
			std::cerr << "MERIC ERROR: ROCm SMI failed to read energy consumption of GPU "+std::to_string(i)+"\n";
		}
	}
	if (HIP_PRIOR_6_2)
	{
		rsmi_shut_down();
	}
	#endif
}

void ROCM::initMap(std::map<std::string, uint64_t> &dict)
{
	for (size_t i = 0; i < GPUindices.size(); i++)
	{
		dict[std::to_string(i)] = 0;
	}
}

double ROCM::getResultValue(uint64_t startValue, uint64_t stopValue, double runtime, int &cumulative, std::string counter, std::string regionName)
{
	cumulative = 1;
	if (stopValue < startValue)
	{
		MERIC_INFO << "ROCM SMI OVERFLOW, MAX VALUE: " << COUNTERMAX << std::endl;
		MERIC_INFO << "ROCM REPAIR " << startValue << " .. " << stopValue << " = " << ((COUNTERMAX - startValue) + stopValue) * getUnit(counter) << std::endl;
		return ((COUNTERMAX - startValue) + stopValue) * getUnit(counter);
	}
	else
	{
		return (stopValue - startValue) * getUnit(counter);
	}
}

std::string ROCM::getSummaryCounterName()
{
	return "Energy Consumption [J]";
}

std::string ROCM::getCounterLabel(std::string counter)
{
	return "GPU_"+counter+" [J]";
}

