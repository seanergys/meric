#include <map>
#include <time.h>
#include <climits>

#include "countersstore.h"
#include "../wrapper/perfeventwrapper.h"
#include "../wrapper/papiwrapper.h"
#include "../wrapper/msrcounterswrapper.h"
#include "../wrapper/raplwrapper.h"
#include "../wrapper/nvmlwrapper.h"

#ifdef HAVE_MPI_H
#include <mpi.h>
#endif

using namespace meric;

template <class TCounter, typename TType, typename RType> void CountersStore<TCounter, TType, RType>::addStart(int region, RegionParameters configuration)
{
	std::map<std::string, TType> record;
	struct timespec time;

	TCounter::getCounterValues(record);
	clock_gettime(CLOCK_REALTIME, &time);

	_records.push_back(CountersRecord<TType>(region, record, time, configuration));
}

template <class TCounter, typename TType, typename RType> void CountersStore<TCounter, TType, RType>::addEnd(int region)
{
	std::map<std::string, TType> record;
	struct timespec time;

	clock_gettime(CLOCK_REALTIME, &time);
	TCounter::getCounterValues(record);

	_records.push_back(CountersRecord<TType>(region, record, time, _records.back().configuration));
}

template <class TCounter, typename TType, typename RType> void CountersStore<TCounter, TType, RType>::fillRegionStats(std::vector<RType> &stats, std::map <std::string, double> &ratios, size_t iStarts, size_t iEnd)
{
	RType summary = 0;
	RType value;
	double time = mutils::timeToDouble(_records[iEnd].time - _records[iStarts].time);
	for(auto const &counterIt : _records[iEnd].record)
	{	 //TODO in case of msrcounters it doesn't make sence share the values since they are not used
		value = TCounter::getResultValue(_records[iStarts].record[counterIt.first], counterIt.second, time, counterIt.first, _name[_records[iStarts].region]);
		stats.push_back(value);
		summary += value;
	}

	if (TCounter::getSummaryCounterName() != "")
	{
		stats.push_back(summary);
	}

	TCounter::getRatios(_records[iStarts].record, _records[iEnd].record, time, ratios);
}

template <class TCounter, typename TType, typename RType> void CountersStore<TCounter, TType, RType>::saveInterval(std::ofstream &ss, std::vector<size_t> &iStarts, size_t iEnd)
{
	ss << "# CALLTREE;init_" << _initCounter;
	for (size_t j = 0; j < iStarts.size()-1; j++) {
		ss << ";" <<  _name[_records[iStarts[j]].region] << "_" << _counters[_records[iStarts[j]].region];
	}
	ss << std::endl;

	double runtime = mutils::timeToDouble(_records[iEnd].time - _records[iStarts.back()].time);
//	ss << "# Job info - "<< TCounter::getSourceLabel() << std::endl
//	   << "JOB ID," << _jobID << "." << _stepID  << std::endl
//	   << "Nodelist," << _nodelist << std::endl
//	   << "Runtime of function [s]," << runtime << std::endl;


	RType summary = 0;
	RType value;
	ss << "# COUNTERS - "<< TCounter::getSourceLabel() <<":\n"
	   << "Runtime of function [s]," << runtime << std::endl;
	for(auto const &counterIt : _records[iEnd].record)
	{
		std::string label = TCounter::getCounterLabel(counterIt.first);
		if (label != "")
		{
			value = TCounter::getResultValue(_records[iStarts.back()].record[counterIt.first], counterIt.second, runtime, counterIt.first, _name[_records[iStarts.back()].region]);
			summary += value;
			ss << label << "," << std::fixed << std::setprecision(5) << value << std::endl;
		}
	}

	if (TCounter::getSummaryCounterName() != "")
	{
		ss << TCounter::getSummaryCounterName() << "," << std::fixed << std::setprecision(5) << summary << std::endl;
	}

	// Counters ratios
	std::map <std::string, double> ratios;
	TCounter::getRatios(_records[iStarts.back()].record, _records[iEnd].record, runtime, ratios);
	if (ratios.size())
	{
		ss << "# RATIOS:\n";
		for(auto const &element : ratios)
			ss << element.first<<"," << std::fixed << std::setprecision(5) << element.second <<std::endl;
	}
}

template class CountersStore <PERFEVENT, unsigned long long int, unsigned long long int>;
template class CountersStore <PAPI, unsigned long long int, unsigned long long int>;
template class CountersStore <MSR, uint64_t, uint64_t>;
template class CountersStore <RAPL, unsigned long long int, double>;
template class CountersStore <HDEEM, double, double>;
template class CountersStore <NVML, unsigned long long, double>;

