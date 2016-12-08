#include <map>
#include <time.h>

#include "countersstore.h"
#include "countersrates.h"

#include "../wrapper/perfeventwrapper.h"
#include "../wrapper/papiwrapper.h"
#include "../wrapper/raplwrapper.h"
#include "../wrapper/counters.h"

// #include <sys/time.h>
// #include <cmath>

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


void CountersStore::saveInterval(std::ofstream &ss, std::vector<size_t> &iStarts, size_t iEnd)
{
	ss << "# CALLTREE;init_" << _initCounter;
	for (size_t j = 0; j < iStarts.size()-1; j++) {
		ss << ";" <<  _name[_records[iStarts[j]].region] << "_" << _counters[_records[iStarts[j]].region];
	}
	ss << std::endl;

	ss << "# Job info" << std::endl
	   << "JOB ID," << _jobID << "." << _stepID  << std::endl
	   << "Nodelist," << _nodelist << std::endl//;
	   << "Runtime of function [s]," << _records[iEnd].time - _records[iStarts.back()].time << std::endl;


	ss << "# COUNTERS: \n";
	for(std::map<std::string, long>::iterator counterIt = _records[iEnd].record.begin(); counterIt != _records[iEnd].record.end(); ++counterIt)
	{
		ss << counterIt->first << "," << counterIt->second - _records[iStarts.back()].record[counterIt->first] << std::endl;
	}

	
//TODO - TEMPORARY solution
	ss << "# RATIOS: \n";
	if (wrapperType == PAPI && _records[iEnd].record.count(PAPI_COUNTERS::COUNTER_AVX))
		ss << "FLOPs," << CountersRates::FLOPS(_records[iEnd].record[PAPI_COUNTERS::COUNTER_AVX] - _records[iStarts.back()].record[PAPI_COUNTERS::COUNTER_AVX], _records[iEnd].time - _records[iStarts.back()].time) << std::endl;
	else if (wrapperType == PERFEVENT && _records[iEnd].record.count(PERFEVENT_COUNTERS::COUNTER_AVX))
		ss << "FLOPs," << CountersRates::FLOPS(_records[iEnd].record[PERFEVENT_COUNTERS::COUNTER_AVX] - _records[iStarts.back()].record[PERFEVENT_COUNTERS::COUNTER_AVX], _records[iEnd].time - _records[iStarts.back()].time) << std::endl;
}




