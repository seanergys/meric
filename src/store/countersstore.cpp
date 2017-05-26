#include <map>
#include <time.h>

#include "countersstore.h"
#include "../wrapper/perfeventwrapper.h"
#include "../wrapper/papiwrapper.h"
#include "../wrapper/raplwrapper.h"

#ifdef HAVE_MPI_H
#include <mpi.h>
#endif

using namespace meric;

void CountersStore::addStart(int region)
{
	std::map<std::string, long long> record;
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
	std::map<std::string, long long> record;
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

void CountersStore::fillRegionStats(std::vector<unsigned long long> &stats, double *time, std::vector<size_t> &iStarts, size_t iEnd)
{
	*time = timespec2double(_records[iEnd].time - _records[iStarts.back()].time);
	for(std::map<std::string, long long>::iterator counterIt = _records[iEnd].record.begin(); counterIt != _records[iEnd].record.end(); ++counterIt)
	{
		if (wrapperType == PERFEVENT)
		{
			stats.push_back(PERFEVENT::getResultValue(_records[iStarts.back()].record[counterIt->first], counterIt->second, counterIt->first, *time));
		}
		else if (wrapperType == PAPI)
		{
			stats.push_back(PAPI::getResultValue(_records[iStarts.back()].record[counterIt->first], counterIt->second, counterIt->first, *time));
		}
		else if (wrapperType == RAPL)
		{
			stats.push_back(RAPL::getResultValue(_records[iStarts.back()].record[counterIt->first], counterIt->second, counterIt->first, *time));
		}
	}
}

void CountersStore::saveInterval(std::ofstream &ss, std::vector<size_t> &iStarts, size_t iEnd)
{
	ss << "# CALLTREE;init_" << _initCounter;
	for (size_t j = 0; j < iStarts.size()-1; j++) {
		ss << ";" <<  _name[_records[iStarts[j]].region] << "_" << _counters[_records[iStarts[j]].region];
	}
	ss << std::endl;

	timespec runtime = _records[iEnd].time - _records[iStarts.back()].time;
	ss << "# Job info - "<< counterLabel << std::endl
	   << "JOB ID," << _jobID << "." << _stepID  << std::endl
	   << "Nodelist," << _nodelist << std::endl
	   << "Runtime of function [s]," << runtime << std::endl;


	unsigned long long int summary = 0;
	unsigned long long int value;
	ss << "# COUNTERS - "<<counterLabel<<": \n";
	for(std::map<std::string, long long>::iterator counterIt = _records[iEnd].record.begin(); counterIt != _records[iEnd].record.end(); ++counterIt)
	{
		if (wrapperType == PERFEVENT)
		{
			value = PERFEVENT::getResultValue(_records[iStarts.back()].record[counterIt->first], counterIt->second, counterIt->first, timespec2double(runtime));
		}
		else if (wrapperType == PAPI)
		{
			value = PAPI::getResultValue(_records[iStarts.back()].record[counterIt->first], counterIt->second, counterIt->first, timespec2double(runtime));
		}
		else if (wrapperType == RAPL)
		{
			value = RAPL::getResultValue(_records[iStarts.back()].record[counterIt->first], counterIt->second, counterIt->first, timespec2double(runtime));
		}

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
				timespec2double(runtime)
				) << std::endl;
		}
		if (_records[iEnd].record.count(PAPI_COUNTERS::COUNTER_CACHE_MISSES) && _records[iEnd].record.count(PAPI_COUNTERS::COUNTER_HW_INSTRUCTIONS))
		{
			ss << "Computational Intensity," << 
				std::fixed << std::setprecision(5) << CountersRates::computationalIntensity(
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
				timespec2double(runtime)
				) << std::endl;
		}
		if (_records[iEnd].record.count(PERFEVENT_COUNTERS::COUNTER_CACHE_MISSES) && _records[iEnd].record.count(PERFEVENT_COUNTERS::COUNTER_HW_INSTRUCTIONS))
		{
			ss << "Computational Intensity," << 
				std::fixed << std::setprecision(5) << CountersRates::computationalIntensity(
					_records[iEnd].record[PERFEVENT_COUNTERS::COUNTER_HW_INSTRUCTIONS] - 
					_records[iStarts.back()].record[PERFEVENT_COUNTERS::COUNTER_HW_INSTRUCTIONS], 
					_records[iEnd].record[PERFEVENT_COUNTERS::COUNTER_CACHE_MISSES] - 
					_records[iStarts.back()].record[PERFEVENT_COUNTERS::COUNTER_CACHE_MISSES]
				) << std::endl;
		}
	}
}




