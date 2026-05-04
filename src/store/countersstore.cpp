#include <map>
#include <time.h>
#include <climits>

#include "countersstore.h"
#include "../wrapper/perfeventwrapper.h"
#include "../wrapper/papiwrapper.h"
#include "../wrapper/msrcounterswrapper.h"
#include "../wrapper/raplwrapper.h"
#include "../wrapper/nvmlwrapper.h"
#include "../wrapper/rocmwrapper.h"
#include "../wrapper/occwrapper.h"
#include "../wrapper/a64fxwrapper.h"
#include "../wrapper/e4wrapper.h"

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
	{
		int cumulative = 0; //identifies whether we should store a summary value of the counter, currently one cumulative = 1 is used, but in general there could be several cumulative counters, each with its summary counter label -> such solution requires RType summuary to be an array and TCounter::getSummaryCounterName() must take id of the summary counter as an argument
		if (TCounter::getCounterLabel(counterIt.first) != "")
		{
			value = TCounter::getResultValue(_records[iStarts].record[counterIt.first], counterIt.second, time, cumulative, counterIt.first, _name[_records[iStarts].region]);
			stats.push_back(value);
			if (cumulative > 0)
				summary += value;
		}
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

	RType summary = 0;
	RType value;
	ss << "# COUNTERS - "<< TCounter::getSourceLabel() << ":\n";
	for(auto const &counterIt : _records[iEnd].record)
	{
		std::string label = TCounter::getCounterLabel(counterIt.first);
		if (label != "")
		{
			int cumulative = 0;
			value = TCounter::getResultValue(_records[iStarts.back()].record[counterIt.first], counterIt.second, runtime, cumulative, counterIt.first, _name[_records[iStarts.back()].region]);
			if (cumulative > 0)
				summary += value;
			ss << label << "," << std::fixed << std::setprecision(6) << value << std::endl;
		}
	}

	if (TCounter::getSummaryCounterName() != "")
	{
		ss << TCounter::getSummaryCounterName() << "," << std::fixed << std::setprecision(6) << summary << std::endl;
	}

	// Counters ratios
	std::map <std::string, double> ratios;
	TCounter::getRatios(_records[iStarts.back()].record, _records[iEnd].record, runtime, ratios);
	if (ratios.size())
	{
		ss << "# RATIOS:\n";
		for(auto const &element : ratios)
			ss << element.first << "," << std::fixed << std::setprecision(6) << element.second << std::endl;
	}
}

template class meric::CountersStore <PERFEVENT, unsigned long long int, unsigned long long int>;
template class meric::CountersStore <PAPI, unsigned long long int, unsigned long long int>;
template class meric::CountersStore <MSR, uint64_t, uint64_t>;
template class meric::CountersStore <RAPL, RAPL::CounterValue_t, double>;
template class meric::CountersStore <HWMON, HWMON::CounterValue_t, double>;
template class meric::CountersStore <HDEEM, double, double>;
template class meric::CountersStore <NVML, unsigned long long, double>;
template class meric::CountersStore <E4, unsigned long long, double>;
template class meric::CountersStore <ROCM, uint64_t, double>;
template class meric::CountersStore <OCC, uint64_t, uint64_t>;
template class meric::CountersStore <A64FX, unsigned long long int, double>;

