
#include "countersstore.h"
#include <map>

// #include <sys/time.h>
// #include <cmath>

using namespace meric;

void CountersStore::addStart(const std::string &region)
{
	int regionID = getRegionID(region);
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


std::map <std::string, long> CountersStore::getValeuesSum()
{
	std::map <std::string, long> result;
	PERFEVENT::initMap(result);

	for(std::map<std::string, long>::iterator counterIt = result.begin(); counterIt != result.end(); ++counterIt)
	{
		for(std::vector<CountersRecord>::iterator recordIt = _records.begin(); recordIt != _records.end(); ++recordIt)
		{	
			result[counterIt->first] += recordIt->second;
		}
	}
	
	return result;
}


void CountersStore::saveInterval(std::ofstream &ss, std::vector<size_t> &iStarts, size_t iEnd)
{
	std::map <std::string, long> result = getValeuesSum();

	ss << "COUNTERS: \n";
/*	for (size_t j = 0; j < iStarts.size()-1; j++) {
		ss << ";" <<  _name[_records[iStarts[j]].region] << "_" << _counters[_records[iStarts[j]].region];
	}
	ss << std::endl;
*/

	for(std::map<std::string, long>::iterator counterIt = result.begin(); counterIt != result.end(); ++counterIt)
	{
		ss << result[counterIt->first] << ", " << counterIt->second << std::endl;
	}
	

/*
	ss << "# Job info" << std::endl
	   << "JOB ID," << _jobID << "." << _stepID  << std::endl
	   << "Nodelist," << _nodelist << std::endl
	   << "Runtime of function [s]," << _records[iEnd].time - _records[iStarts.back()].time << std::endl;

	ss << "# Blade summary" << std::endl
	   << "Energy consumption [J] (Stats structure)" << "," << _records[iEnd].energy - _records[iStarts.back()].energy << std::endl;
*/
}




