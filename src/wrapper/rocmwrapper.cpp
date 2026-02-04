#include "rocmwrapper.h"
#include "environmentwrapper.h"

using namespace meric;

uint64_t ROCM::COUNTERMAX = __UINT64_MAX__;
std::map<std::string, uint64_t> ROCM::counters;
std::vector<uint32_t> ROCM::GPUindices;
std::vector<std::vector<double>> ROCM::samples = {};
std::thread ROCM::samplingThread;

double ROCM::unit = 1.0;
bool rsmiSamplingRunning = true;

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
		// ret = rsmi_init(0);
	}
	// Initialize the counter map
	initMap(counters);
#endif
}

bool ROCM::isAvailable()
{
#ifdef HAVE_ROCM_SMI_ROCM_SMI_H
	uint32_t deviceCount;
	uint64_t energy;
	float precision;
	uint64_t timestamp;
	// energy read cause segfault if rocm smi not initialized, which we do not know in this location
	// for that reason additional device count read is issued
	if (rsmi_num_monitor_devices(&deviceCount) == RSMI_STATUS_SUCCESS && deviceCount > 0 && rsmi_dev_energy_count_get(0, &energy, &precision, &timestamp) == RSMI_STATUS_SUCCESS)
		return true;
#else
	return false;
#endif // HAVE_ROCM_SMI_ROCM_SMI_H
}

void ROCM::start(unsigned int sampling_frequency)
{
#ifdef HAVE_ROCM_SMI_ROCM_SMI_H
	samples.resize(GPUindices.size(), std::vector<double>());
	samplingThread = std::thread{[sampling_frequency]()
								 { collectSamples(sampling_frequency); }};
#endif
}

void ROCM::stop()
{
	rsmiSamplingRunning = false;
	samplingThread.join();
	for (int i = 0; i < GPUindices.size(); i++)
	{
		MERIC_INFO << "GPU index: " << i << " power samples: " << std::endl;
		for (int j = 0; j < samples[i].size(); j++)
		{
			MERIC_INFO << samples[i][j] << " [W]\n";
		}
	}
}

void ROCM::collectSamples(unsigned int sampling_frequency)
{
#ifdef HAVE_ROCM_SMI_ROCM_SMI_H
	while (rsmiSamplingRunning)
	{
		if (HIP_PRIOR_6_2)
		{
			rsmi_init(0);
		}
		for (int i = 0; i < GPUindices.size(); i++)
		{
			uint64_t power;
			if (rsmi_dev_power_ave_get(i, 0, &power) == RSMI_STATUS_SUCCESS)
			{
				samples[i].push_back(static_cast<double>(power) / 1e6);
				std::cout << "Power on gpu[" << i << "]: " << power << std::endl;
			}
		}
		if (HIP_PRIOR_6_2)
		{
			rsmi_shut_down();
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(sampling_frequency));
	}
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
	if (HIP_PRIOR_6_2)
	{
		rsmi_status_t ret = rsmi_init(0);
		if (ret != RSMI_STATUS_SUCCESS)
		{
			std::cerr << "MERIC ERROR: ROCm SMI initialization failed!\n";
			return;
		}
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
			std::cerr << "MERIC ERROR: ROCm SMI failed to read energy consumption of GPU " + std::to_string(i) + "\n";
		}
	}
	if (HIP_PRIOR_6_2)
	{
		rsmi_shut_down();
		// rsmi_init(0);
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
	return "Energy consumption [J]";
}

std::string ROCM::getCounterLabel(std::string counter)
{
	return "GPU_" + counter + " [J]";
}
