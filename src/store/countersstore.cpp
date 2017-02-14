#include <map>
#include <time.h>

#include "countersstore.h"

#include "../wrapper/perfeventwrapper.h"
#include "../wrapper/papiwrapper.h"
#include "../wrapper/raplwrapper.h"

// #include <sys/time.h>
// #include <cmath>

#ifdef HAVE_MPI_H
#include <mpi.h>
#endif

using namespace meric;

void CountersStore::addStart(int region)
{
	std::map<std::string, long> record;
	if (wrapperType == PERFEVENT)
		PERFEVENT::getValues(record);
	else if (wrapperType == PAPI)
		PAPI::getValues(record);
	else if (wrapperType == RAPL)
		RAPL::getValues(record);
	struct timespec time;
	clock_gettime(CLOCK_REALTIME, &time);
	_records.push_back(CountersRecord(region, record, time));
}

void CountersStore::addEnd(int region)
{
	std::map<std::string, long> record;
	if (wrapperType == PERFEVENT)
		PERFEVENT::getValues(record);
	else if (wrapperType == PAPI)
		PAPI::getValues(record);
	else if (wrapperType == RAPL)
		RAPL::getValues(record);
	struct timespec time;
	clock_gettime(CLOCK_REALTIME, &time);
	_records.push_back(CountersRecord(region, record, time));
}

void CountersStore::fillRegionStats(std::vector<long> &stats, double *time, std::vector<size_t> &iStarts, size_t iEnd)
{
	for(std::map<std::string, long>::iterator counterIt = _records[iEnd].record.begin(); counterIt != _records[iEnd].record.end(); ++counterIt)
	{
		stats.push_back(counterIt->second - _records[iStarts.back()].record[counterIt->first]);
	}
	//last item is time
	*time = timespec2double(_records[iEnd].time - _records[iStarts.back()].time);
}

void CountersStore::saveInterval(std::ofstream &ss, std::vector<size_t> &iStarts, size_t iEnd)
{
	ss << "# CALLTREE;init_" << _initCounter;
	for (size_t j = 0; j < iStarts.size()-1; j++) {
		ss << ";" <<  _name[_records[iStarts[j]].region] << "_" << _counters[_records[iStarts[j]].region];
	}
	ss << std::endl;

	ss << "# Job info - "<< counterLabel << std::endl
	   << "JOB ID," << _jobID << "." << _stepID  << std::endl
	   << "Nodelist," << _nodelist << std::endl
	   << "Runtime of function [s]," << _records[iEnd].time - _records[iStarts.back()].time << std::endl;


	unsigned long long int summary = 0;
	unsigned long long int value;
	ss << "# COUNTERS - "<<counterLabel<<": \n";
	for(std::map<std::string, long>::iterator counterIt = _records[iEnd].record.begin(); counterIt != _records[iEnd].record.end(); ++counterIt)
	{
		value = counterIt->second - _records[iStarts.back()].record[counterIt->first];
		ss << counterIt->first << "," << value << std::endl;
		summary += value;
	}

	
//TODO - TEMPORARY solution	#19
	if (wrapperType == RAPL)
		ss << "Energy summary," << summary << std::endl;
	else if (wrapperType == PAPI)
	{
		ss << "# RATIOS: \n";
		if (_records[iEnd].record.count(PAPI_COUNTERS::COUNTER_AVX))
		{
			ss << "GLOPs," << std::fixed << std::setprecision(5) << CountersRates::FLOPS(
				_records[iEnd].record[PAPI_COUNTERS::COUNTER_AVX] - 
				_records[iStarts.back()].record[PAPI_COUNTERS::COUNTER_AVX], 
				timespec2double(_records[iEnd].time - _records[iStarts.back()].time)
				) << std::endl;
		}
		if (_records[iEnd].record.count(PAPI_COUNTERS::COUNTER_CACHE_MISSES) && _records[iEnd].record.count(PAPI_COUNTERS::COUNTER_HW_INSTRUCTIONS))
		{
			ss << "Computional Intensity," << 
				std::fixed << std::setprecision(5) << CountersRates::computionalIntansity(
					_records[iEnd].record[PAPI_COUNTERS::COUNTER_HW_INSTRUCTIONS] - 
					_records[iStarts.back()].record[PAPI_COUNTERS::COUNTER_HW_INSTRUCTIONS], 
					_records[iEnd].record[PAPI_COUNTERS::COUNTER_CACHE_MISSES] - 
					_records[iStarts.back()].record[PAPI_COUNTERS::COUNTER_CACHE_MISSES]
				) << std::endl;
		}
	}
	else if (wrapperType == PERFEVENT)
	{
		ss << "# RATIOS: \n";
		if (_records[iEnd].record.count(PERFEVENT_COUNTERS::COUNTER_AVX))
		{
			ss << "GFLOPs," << std::fixed << std::setprecision(5) << CountersRates::FLOPS(
				_records[iEnd].record[PERFEVENT_COUNTERS::COUNTER_AVX] - 
				_records[iStarts.back()].record[PERFEVENT_COUNTERS::COUNTER_AVX], 
				timespec2double(_records[iEnd].time - _records[iStarts.back()].time)
				) << std::endl;
		}
		if (_records[iEnd].record.count(PERFEVENT_COUNTERS::COUNTER_CACHE_MISSES) && _records[iEnd].record.count(PERFEVENT_COUNTERS::COUNTER_HW_INSTRUCTIONS))
		{
			ss << "Computional Intensity," << 
				std::fixed << std::setprecision(5) << CountersRates::computionalIntansity(
					_records[iEnd].record[PERFEVENT_COUNTERS::COUNTER_HW_INSTRUCTIONS] - 
					_records[iStarts.back()].record[PERFEVENT_COUNTERS::COUNTER_HW_INSTRUCTIONS], 
					_records[iEnd].record[PERFEVENT_COUNTERS::COUNTER_CACHE_MISSES] - 
					_records[iStarts.back()].record[PERFEVENT_COUNTERS::COUNTER_CACHE_MISSES]
				) << std::endl;
		}
	}
}




