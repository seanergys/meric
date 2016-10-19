
#include "countersstore.h"
#include <map>

#include "../wrapper/perfeventwrapper.h"

// #include <sys/time.h>
// #include <cmath>

using namespace meric;

void CountersStore::addStart(const std::string &region)
{
	int regionID = getRegionID(region);
	std::map<std::string, long> record;
	PERFEVENT::initMap(record);
	_records.push_back(CountersRecord(_regions[region], record));
	_stack.push_back(_regions[region]);
}

void CountersStore::addEnd()
{
	if (_stack.size()) {
		std::map<std::string, long> record;
		PERFEVENT::getValues(record);
		_records.push_back(CountersRecord(_stack.back(), record));
		_stack.pop_back();
	} else {
		std::cerr << "MERIC ERROR: Incorrect regions. There are more 'ends' than 'starts'!\n";
	}
}


void CountersStore::getValeuesSum(std::map <std::string, long> & result, size_t iStart, size_t iEnd)
{
	PERFEVENT::initMap(result);

	//iterate for each counter type
	for(std::map<std::string, long>::iterator counterIt = result.begin(); counterIt != result.end(); ++counterIt)
	{
		//for each record
		for (size_t i=iStart; i<iEnd; i++)
		{
			result[counterIt->first] += _records[i].record[counterIt->first];
		}
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
	std::map <std::string, long> result;
	getValeuesSum(result, iStarts.back(), iEnd);

	for(std::map<std::string, long>::iterator counterIt = result.begin(); counterIt != result.end(); ++counterIt)
	{
		ss << counterIt->first << ", " << counterIt->second << std::endl;
	}
	
}




