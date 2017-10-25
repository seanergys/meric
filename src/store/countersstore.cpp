#include <map>
#include <time.h>
#include <climits>

#include "countersstore.h"
#include "../wrapper/perfeventwrapper.h"
#include "../wrapper/papiwrapper.h"
#include "../wrapper/raplwrapper.h"

#ifdef HAVE_MPI_H
#include <mpi.h>
#endif

using namespace meric;

template <class TCounter> std::string CountersStore<TCounter>::getCountersLabel()
{
	return TCounter::getCountersLabel();
}

template <class TCounter> void CountersStore<TCounter>::addStart(int region)
{
	std::map<std::string, long long> record;
	struct timespec time;
	
/*	if (wrapperType == PERFEVENT)
		PERFEVENT::getCounterValues(record);
	else if (wrapperType == PAPI)
		PAPI::getCounterValues(record);
	else if (wrapperType == RAPL)
		RAPL::getCounterValues(record);
*/
	TCounter::getCounterValues(record);
	clock_gettime(CLOCK_REALTIME, &time);

	_records.push_back(CountersRecord(region, record, time));
}

template <class TCounter> void CountersStore<TCounter>::addEnd(int region)
{
	std::map<std::string, long long> record;
	struct timespec time;

	clock_gettime(CLOCK_REALTIME, &time);
	TCounter::getCounterValues(record);
/*	if (wrapperType == PERFEVENT)
		PERFEVENT::getCounterValues(record);
	else if (wrapperType == PAPI)
		PAPI::getCounterValues(record);
	else if (wrapperType == RAPL)
		RAPL::getCounterValues(record);
*/

	_records.push_back(CountersRecord(region, record, time));
}

template <class TCounter> void CountersStore<TCounter>::fillRegionStats(std::vector<unsigned long long> &stats, double *time, std::vector<size_t> &iStarts, size_t iEnd)
{
	int summaryIndex = -1;
	unsigned long long int summary = 0;
	unsigned long long int value;
	*time = timespec2double(_records[iEnd].time - _records[iStarts.back()].time);
	for(std::map<std::string, long long>::iterator counterIt = _records[iEnd].record.begin(); counterIt != _records[iEnd].record.end(); ++counterIt)
	{
		if (TCounter::getSummaryCounterName() == counterIt->first)
		{
			summaryIndex = summaryIndex = stats.size();
			stats.push_back(0);
		}
		else
		{
			value = TCounter::getResultValue(_records[iStarts.back()].record[counterIt->first], counterIt->second, counterIt->first);
			stats.push_back(value);
			summary += value;
		}
/*		if (wrapperType == PERFEVENT)
		{
			stats.push_back(PERFEVENT::getResultValue(_records[iStarts.back()].record[counterIt->first], counterIt->second, counterIt->first));
		}
		else if (wrapperType == PAPI)
		{
			stats.push_back(PAPI::getResultValue(_records[iStarts.back()].record[counterIt->first], counterIt->second, counterIt->first));
		}
		else if (wrapperType == RAPL)
		{
			stats.push_back(RAPL::getResultValue(_records[iStarts.back()].record[counterIt->first], counterIt->second, counterIt->first));
		}
*/
	}

/*	if (wrapperType == RAPL)	//DONE for all counters (y)
	{
		if ((summary = RAPL::getSummaryValue(*time)) != ULLONG_MAX)
		{
			stats.back() = summary;
		}
	}
*/

	if (summaryIndex != -1)
	{
		TCounter::getSummaryValue(*time, &summary);
		stats[summaryIndex] = summary;
	}
}

template <class TCounter> void CountersStore<TCounter>::saveInterval(std::ofstream &ss, std::vector<size_t> &iStarts, size_t iEnd)
{
	ss << "# CALLTREE;init_" << _initCounter;
	for (size_t j = 0; j < iStarts.size()-1; j++) {
		ss << ";" <<  _name[_records[iStarts[j]].region] << "_" << _counters[_records[iStarts[j]].region];
	}
	ss << std::endl;

	timespec runtime = _records[iEnd].time - _records[iStarts.back()].time;
	ss << "# Job info - "<< TCounter::getCountersLabel() << std::endl
	   << "JOB ID," << _jobID << "." << _stepID  << std::endl
	   << "Nodelist," << _nodelist << std::endl
	   << "Runtime of function [s]," << runtime << std::endl;


	unsigned long long int summary = 0;
	unsigned long long int value;
	ss << "# COUNTERS - "<< TCounter::getCountersLabel() <<": \n";
	for(std::map<std::string, long long>::iterator counterIt = _records[iEnd].record.begin(); counterIt != _records[iEnd].record.end(); ++counterIt)
	{
			if (TCounter::getSummaryCounterName() == counterIt->first)
				continue;
			value = TCounter::getResultValue(_records[iStarts.back()].record[counterIt->first], counterIt->second, counterIt->first);
			summary += value;
/*		if (wrapperType == PERFEVENT)
		{
			if (PERFEVENT::summaryCounter() == counterIt->first)
				continue;
			value = PERFEVENT::getResultValue(_records[iStarts.back()].record[counterIt->first], counterIt->second, counterIt->first);
		}
		else if (wrapperType == PAPI)
		{
			if (PAPI::summaryCounter() == counterIt->first)
				continue;
			value = PAPI::getResultValue(_records[iStarts.back()].record[counterIt->first], counterIt->second, counterIt->first);
		}
		else if (wrapperType == RAPL)
		{
			if (RAPL::summaryCounter() == counterIt->first)
				continue;
			value = RAPL::getResultValue(_records[iStarts.back()].record[counterIt->first], counterIt->second, counterIt->first);
		}
*/
		ss << counterIt->first << "," << value << std::endl;
	}

	TCounter::getSummaryValue(timespec2double(runtime), &summary);
	if (summary != ULLONG_MAX)
		ss << TCounter::getSummaryCounterName() << "," << summary << std::endl;

/*	
	if (wrapperType == RAPL)
	{
		if ((summary = RAPL::getSummaryValue(timespec2double(runtime))) != ULLONG_MAX)
			ss << RAPL::summaryCounter() << "," << summary << std::endl;
	}
*/

////////////////////////////////////////////////////////////////////////////////

/*	
	//TODO - TEMPORARY solution	#19
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
*/
}

template class CountersStore <PERFEVENT>;
template class CountersStore <PAPI>;
template class CountersStore <RAPL>;


