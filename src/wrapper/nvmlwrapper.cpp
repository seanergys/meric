#include "nvmlwrapper.h"
#include "environmentwrapper.h"

using namespace meric;

unsigned long long NVML::COUNTERMAX = pow(2,32);
std::map<std::string, unsigned long long> NVML::counters;
unsigned long long NVML::sampleBeginTimestamp;
std::vector <std::vector<double>> NVML::samples = {};
std::thread NVML::samplingThread;

bool samplingRunning = true;

#ifdef HAVE_NVML_H
	std::vector <nvmlDevice_t> NVML::GPUhandles;
#endif

/**
 * sampling thread function for querrying the NVML power samples in regular intervals
 * in order to overcome the NVML buffer limits, the samples are saved every 5 seconds
 **/
void NVML::collectSamples(unsigned int sampling_frequency)
{
#ifdef HAVE_NVML_H
	while(samplingRunning)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(sampling_frequency));
		//usleep(1000000);
		for (int i = 0; i < GPUhandles.size(); i++)
		{
			nvmlSample_t *sampleArray;
			unsigned int sampleCount;
			nvmlValueType_t sampleValType;
			nvmlSamplingType_t power = NVML_TOTAL_POWER_SAMPLES;
			std::vector<double> deviceSamples = {};
			if (nvmlDeviceGetSamples(GPUhandles[i], power, sampleBeginTimestamp, &sampleValType, &sampleCount, NULL) != NVML_SUCCESS || !sampleCount)
			{
				std::cerr << "NVML error: cannot obtain sample buffer size\n";
			}
			sampleArray = new nvmlSample_t[sampleCount]; 
			if (nvmlDeviceGetSamples(GPUhandles[i], power, sampleBeginTimestamp, &sampleValType, &sampleCount, sampleArray) == NVML_SUCCESS && sampleCount > 0)
			{
				for(int j = 0; j < sampleCount; j++)
				{
					deviceSamples.push_back(static_cast<double>(sampleArray[i].sampleValue.uiVal) / 1000.0);
				}
			}
			samples[i].insert(samples[i].end(),deviceSamples.begin(),deviceSamples.end());
			delete[] sampleArray;
			sampleBeginTimestamp = sampleBeginTimestamp + sampling_frequency*1000;
		}
	}
#endif
}

/**
 * returns result value of the counter (std::string) based on its startValue and
 * stopValue and time between these two measurements
 * handles counter overflow
 **/
double NVML::getResultValue(unsigned long long startValue, unsigned long long stopValue, double runtime, int & cumulative, std::string counter, std::string regionName)
{
	cumulative = 1;
	if (stopValue < startValue)
	{	
		MERIC_INFO << "NVML OVERFLOW, MAX VALUE: " << COUNTERMAX << std::endl;
		MERIC_INFO << "NVML REPAIR " << startValue << " .. " << stopValue << " = " << ((COUNTERMAX - startValue) + stopValue) * getUnit(counter) << std::endl;
		return ((COUNTERMAX - startValue) + stopValue) * getUnit(counter);
	}
	else
	{
		return (stopValue - startValue) * getUnit(counter);
	}
}

/**
 * initialization of the NVML class
 * fills a vector of GPUs handles
 **/
void NVML::init(bool detailed)
{
	unsigned int deviceCount = 0;
#ifdef HAVE_NVML_H
	if (!GPUhandles.empty())
	{
		return;
	}
	if (nvmlDeviceGetCount(&deviceCount) == NVML_SUCCESS)
	{
		for (int i=0; i < deviceCount; i++)
		{
			nvmlDevice_t deviceHandle;
			if (nvmlDeviceGetHandleByIndex(i, &deviceHandle) == NVML_SUCCESS)
			{
				GPUhandles.push_back(deviceHandle);
			}
		}
		initMap(counters);
	}
#endif
}

void NVML::start(unsigned int sampling_frequency)
{
#ifdef HAVE_NVML_H
	samples.resize(GPUhandles.size(), std::vector<double>());
	struct timespec tp;
	clock_gettime(CLOCK_MONOTONIC,&tp);
	sampleBeginTimestamp = tp.tv_sec * 1e6 + tp.tv_nsec / 1000;
	samplingThread = std::thread{ [sampling_frequency]() { collectSamples(sampling_frequency); } };
#endif
}

void NVML::stop()
{
#ifdef HAVE_NVML_H
	samplingRunning = false;
	samplingThread.detach();
#endif
}

/**
 * read values of the counters (std::string) from the input dictionary and store
 * the values into the dictionary (unsigned long long)
 **/
void NVML::getCounterValues(std::map<std::string, unsigned long long> & record)
{
	for (int i=0; i<getGPUsPerNode(); i++)
	{
#ifdef HAVE_NVML_H
		unsigned long long energy;
		if (nvmlDeviceGetTotalEnergyConsumption(GPUhandles[i], &energy) == NVML_SUCCESS)
		{
			record[std::to_string(i)] = energy;
		}
		else
#endif
		{
			record[std::to_string(i)] = 0;
		}
	}
}




/**
 * each counter has a label, that will represent this counter in the output file
 * returns a label of the input counter
 * in NVML there is just one kind of lable, so it returns the input
 **/
std::string NVML::getCounterLabel(std::string counter)
{
	return "GPU_"+counter+" [J]";
}

/**
 * summary counter represents energy consumption of all the GPUs at one node
 * returns label of the "artificial" summary counter
 **/
std::string NVML::getSummaryCounterName()
{
	return "Energy consumption [J]";
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
	{
		dict[std::to_string(i)] = 0;
	}
}

