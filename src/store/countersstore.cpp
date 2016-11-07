
#include "countersstore.h"
#include <map>
#include <time.h>

#include "../wrapper/perfeventwrapper.h"

// #include <sys/time.h>
// #include <cmath>

using namespace meric;

void CountersStore::addStart(const std::string &region)
{
	int regionID = getRegionID(region);
	std::map<std::string, long> record;
	PERFEVENT::getValues(record);
	struct timespec time;
	clock_gettime(CLOCK_REALTIME, &time);
	_records.push_back(CountersRecord(_regions[region], record, time));
	_stack.push_back(_regions[region]);
}

void CountersStore::addEnd()
{
	if (_stack.size()) {
		std::map<std::string, long> record;
		PERFEVENT::getValues(record);
		struct timespec time;
		clock_gettime(CLOCK_REALTIME, &time);
		_records.push_back(CountersRecord(_stack.back(), record, time));
		_stack.pop_back();
	} else {
		std::cerr << "MERIC ERROR: Incorrect regions. There are more 'ends' than 'starts'!\n";
	}
}


void CountersStore::saveInterval(std::ofstream &ss, std::vector<size_t> &iStarts, size_t iEnd)
{
//	std::cout << "<"<<iStarts.back() <<" - " <<iEnd << ">\n";
	ss << "# CALLTREE;init_" << _initCounter;
	for (size_t j = 0; j < iStarts.size()-1; j++) {
		ss << ";" <<  _name[_records[iStarts[j]].region] << "_" << _counters[_records[iStarts[j]].region];
	}
	ss << std::endl;

	ss << "# Job info" << std::endl
	   << "JOB ID," << _jobID << "." << _stepID  << std::endl
	   << "Nodelist," << _nodelist << std::endl;
//	   << "Runtime of function [s]," << _records[iEnd].time - _records[iStarts.back()].time << std::endl;


	ss << "# COUNTERS: \n";
	for(std::map<std::string, long>::iterator counterIt = _records[iEnd].record.begin(); counterIt != _records[iEnd].record.end(); ++counterIt)
	{
		ss << counterIt->first << "," << counterIt->second - _records[iStarts.back()].record[counterIt->first] << std::endl;
	}

	
/*
Packed SP MFLOP/s = 1.0E-06*(AVX_INSTS_CALC*8)/runtime;
Packed DP MFLOP/s = 1.0E-06*(AVX_INSTS_CALC*4)/runtime;
1.0E-06*(result[COUNTER_AVX]*8)/(_records[iEnd].time - _records[iStarts.back()].time)
*/
	
}




