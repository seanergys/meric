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

template <class TCounter, typename TType> std::string CountersStore<TCounter, TType>::getCountersLabel()
{
	return TCounter::getCountersLabel();
}

template <class TCounter, typename TType> std::string CountersStore<TCounter, TType>::getCounterLabel(std::string counter)
{
	return TCounter::getCounterLabel(counter);
}

template <class TCounter, typename TType> void CountersStore<TCounter, TType>::getRatios(std::map<std::string, TType>& start, std::map<std::string, TType>& stop, double runtime, std::map <std::string, double> & ratios)
{
	TCounter::getRatios(start, stop, runtime, ratios);
}

template <class TCounter, typename TType> void CountersStore<TCounter, TType>::addStart(int region)
{
	std::map<std::string, TType> record;
	struct timespec time;

	TCounter::getCounterValues(record);
	clock_gettime(CLOCK_REALTIME, &time);

	_records.push_back(CountersRecord<TType>(region, record, time));
}

template <class TCounter, typename TType> void CountersStore<TCounter, TType>::addEnd(int region)
{
	std::map<std::string, TType> record;
	struct timespec time;

	clock_gettime(CLOCK_REALTIME, &time);
	TCounter::getCounterValues(record);

	_records.push_back(CountersRecord<TType>(region, record, time));
}

template <class TCounter, typename TType> void CountersStore<TCounter, TType>::fillRegionStats(std::vector<unsigned long long> &stats, double *time, size_t iStarts, size_t iEnd)
{
	unsigned long long int summary = 0;
	TType value;
	*time = mutils::timeToDouble(_records[iEnd].time - _records[iStarts].time);
	for(auto const &counterIt : _records[iEnd].record)
	{
		value = TCounter::getResultValue(_records[iStarts].record[counterIt.first], counterIt.second, *time, counterIt.first);
		stats.push_back(value);
		summary += value;
	}

	if (TCounter::getSummaryCounterName() != "")
	{
		TCounter::getSummaryValue(*time, &summary);
		stats[distance(_records[iEnd].record.begin(),_records[iEnd].record.find(TCounter::getSummaryCounterName()))] = summary;
	}
}

template <class TCounter, typename TType> void CountersStore<TCounter, TType>::saveInterval(std::ofstream &ss, std::vector<size_t> &iStarts, size_t iEnd)
{
	ss << "# CALLTREE;init_" << _initCounter;
	for (size_t j = 0; j < iStarts.size()-1; j++) {
		ss << ";" <<  _name[_records[iStarts[j]].region] << "_" << _counters[_records[iStarts[j]].region];
	}
	ss << std::endl;

	double runtime = mutils::timeToDouble(_records[iEnd].time - _records[iStarts.back()].time);
	ss << "# Job info - "<< TCounter::getCountersLabel() << std::endl
	   << "JOB ID," << _jobID << "." << _stepID  << std::endl
	   << "Nodelist," << _nodelist << std::endl
	   << "Runtime of function [s]," << runtime << std::endl;


	unsigned long long int summary = 0;
	TType value;
	ss << "# COUNTERS - "<< TCounter::getCountersLabel() <<": \n";
	for(auto const &counterIt : _records[iEnd].record)
	{
		if (TCounter::getSummaryCounterName() == counterIt.first)
			continue;
		value = TCounter::getResultValue(_records[iStarts.back()].record[counterIt.first], counterIt.second, runtime, counterIt.first);
		summary += value;
		ss << TCounter::getCounterLabel(counterIt.first) << "," << value << std::endl;
	}

	TCounter::getSummaryValue(runtime, &summary);
	if (summary != ULLONG_MAX)
		ss << TCounter::getSummaryCounterName() << "," << summary << std::endl;


// Counters ratios
	std::map <std::string, double> ratios;
	TCounter::getRatios(_records[iStarts.back()].record, _records[iEnd].record, runtime, ratios);
	if (ratios.size())
	{
		ss << "# RATIOS: \n";
		for(auto const &element : ratios)
			ss << element.first<<"," << std::fixed << std::setprecision(5) << element.second <<std::endl;
	}
}

template class CountersStore <PERFEVENT, unsigned long long int>;
template class CountersStore <PAPI, unsigned long long int>;
template class CountersStore <RAPL, unsigned long long int>;
template class CountersStore <HDEEM, double>;

