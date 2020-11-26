#include "nvmlwrapper.h"
#include "environmentwrapper.h"

using namespace meric;

unsigned long long NVML::COUNTERMAX = pow(2,32);
std::vector <nvmlDevice_t> NVML::GPUhandles;
std::map<std::string, unsigned long long> NVML::counters;

/**
 * returns result value of the counter (std::string) based on its startValue and
 * stopValue and time between these two measurements
 * handles counter overflow
 **/
double NVML::getResultValue(unsigned long long startValue, unsigned long long stopValue, double runtime, std::string counter)
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
	unsigned int deviceCount;
	nvmlReturn_t ret = nvmlDeviceGetCount(&deviceCount);
	for (int i=0; i < deviceCount; i++)
	{
		nvmlDevice_t deviceHandle;
		if (nvmlDeviceGetHandleByIndex(i, &deviceHandle) == NVML_SUCCESS)
			GPUhandles.push_back(deviceHandle);
	}
	
	initMap(counters);
}

/**
 * read values of the counters (std::string) from the input dictionary and store
 * the values into the dictionary (unsigned long long)
 **/
void NVML::getCounterValues(std::map<std::string, unsigned long long> & record)
{
	for (int i=0; i<GPUhandles.size(); i++)
	{
		unsigned long long energy;
		if (nvmlDeviceGetTotalEnergyConsumption(GPUhandles[i], &energy) == NVML_SUCCESS)
			record["Energy_GPU_"+std::to_string(i)] = energy;
		else
			record["Energy_GPU_"+std::to_string(i)] = 0;
	}
}

/**
 * each counter has a label, that will represent this counter in the output file
 * returns a label of the input counter
 * in NVML there is just one kind of lable, so it returns the input
 **/
std::string NVML::getCounterLabel(std::string counter)
{
	return counter;
}

/**
 * summary counter represents energy consumption of all the GPUs at one node
 * returns label of the "artificial" summary counter
 **/
std::string NVML::getSummaryCounterName()
{
//const std::string NVML_SUM_ENERGY  = "NVML Energy consumption [J]";
	if (GPUhandles.size() > 0)
		return "NVML Energy consumption [J]";
	else
		return "";
}

/**
 * fill-in the NVML counters into the dictionary, that will keep the energy consumption of each GPU
 **/
void NVML::initMap(std::map<std::string, unsigned long long> & dict)
{
	for (int i=0; i<GPUhandles.size(); i++)
		dict["Energy_GPU_"+std::to_string(i)] = 0;
/*
// fill this vector
	std::vector <std::string> names;
	names.push_back(RAPL_PCKG_ENERGY);
	names.push_back(RAPL_RAM_ENERGY);
*/
}




