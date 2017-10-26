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

template <class TCounter> void CountersStore<TCounter>::getRatios(CountersRecord start, CountersRecord stop, double runtime, std::map <std::string, double> & ratios)
{
	TCounter::getRatios(start, stop, runtime, ratios);
}

template <class TCounter> void CountersStore<TCounter>::addStart(int region)
{
	std::map<std::string, long long> record;
	struct timespec time;

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
	}

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
		ss << counterIt->first << "," << value << std::endl;
	}

	TCounter::getSummaryValue(timespec2double(runtime), &summary);
	if (summary != ULLONG_MAX)
		ss << TCounter::getSummaryCounterName() << "," << summary << std::endl;


// Counters ratios
	std::map <std::string, double> ratios;
	TCounter::getRatios(_records[iStarts.back()], _records[iEnd], timespec2double(runtime), ratios);
	if (ratios.size())
	{
		ss << "# RATIOS: \n";
		for(auto const &element : ratios)
			ss << element.first<<"," << std::fixed << std::setprecision(5) << element.second <<std::endl;
	}
}

template class CountersStore <PERFEVENT>;
template class CountersStore <PAPI>;
template class CountersStore <RAPL>;


