#include "nvmlwrapper.h"
#include "environmentwrapper.h"

using namespace meric;

unsigned long long NVML::COUNTERMAX = pow(2,32);
std::map<std::string, unsigned long long> NVML::counters;

#ifdef HAVE_NVML_H
	std::vector <nvmlDevice_t> NVML::GPUhandles;
#endif

/**
 * returns result value of the counter (std::string) based on its startValue and
 * stopValue and time between these two measurements
 * handles counter overflow
 **/
double NVML::getResultValue(unsigned long long startValue, unsigned long long stopValue, double runtime, std::string counter, std::string regionName)
{
	if (stopValue < startValue)
	{	
		MERIC_INFO << "NVML OVERFLOW, MAX VALUE: " << COUNTERMAX << std::endl;
		MERIC_INFO << "NVML REPAIR " << startValue << " .. " << stopValue << " = " << ((COUNTERMAX - startValue) + stopValue) * getUnit(counter) << std::endl;
		return ((COUNTERMAX - startValue) + stopValue) * getUnit(counter);
	}
	else
		return (stopValue - startValue) * getUnit(counter);
}

/**
 * initialization of the NVML class
 * fills a vector of GPUs handles
 **/
void NVML::init()
{
	unsigned int deviceCount = 0;
	#ifdef HAVE_NVML_H
	if (nvmlDeviceGetCount(&deviceCount) == NVML_SUCCESS)
	{
		for (int i=0; i < deviceCount; i++)
		{
			nvmlDevice_t deviceHandle;
			if (nvmlDeviceGetHandleByIndex(i, &deviceHandle) == NVML_SUCCESS)
				GPUhandles.push_back(deviceHandle);
		}
	}
	#endif
	initMap(counters);
}

/**
 * read values of the counters (std::string) from the input dictionary and store
 * the values into the dictionary (unsigned long long)
 **/
void NVML::getCounterValues(std::map<std::string, unsigned long long> & record)
{
	for (int i=0; i<getGPUsPerNode(); i++)
	{
		unsigned long long energy;
#ifdef HAVE_NVML_H
		if (nvmlDeviceGetTotalEnergyConsumption(GPUhandles[i], &energy) == NVML_SUCCESS)
			record[std::to_string(i)] = energy;
		else
#endif
			record[std::to_string(i)] = 0;
	}
}

/**
 * each counter has a label, that will represent this counter in the output file
 * returns a label of the input counter
 * in NVML there is just one kind of lable, so it returns the input
 **/
std::string NVML::getCounterLabel(std::string counter)
{
	return "Energy_GPU_"+counter+" [J]";
}

/**
 * summary counter represents energy consumption of all the GPUs at one node
 * returns label of the "artificial" summary counter
 **/
std::string NVML::getSummaryCounterName()
{
//const std::string NVML_SUM_ENERGY  = "NVML Energy consumption [J]";
	if (getGPUsPerNode() > 0)
		return "NVML Energy consumption [J]";
	else
		return "";
}

/**
 * fill-in the NVML counters into the dictionary, that will keep the energy consumption of each GPU
 * since NVML supports overall energy cosumption of a GPU only, the GPU id (std::string) 
 * keeps the energy consumption of the GPU
 * corresponding counter label is created in getCounterLabel(std::string counter)
 **/
void NVML::initMap(std::map<std::string, unsigned long long> & dict)
{
	for (int i=0; i<getGPUsPerNode(); i++)
		dict[std::to_string(i)] = 0;
}




