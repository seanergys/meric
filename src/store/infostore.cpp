
#include "infostore.h"
#include <sys/time.h>

using namespace meric;

void InfoStore::addStart(int region, RegionParameters configuration)
{
	struct timespec time;
	clock_gettime(CLOCK_REALTIME, &time);
	
	_records.push_back(InfoRecord(region, time, configuration));
	_timeStack.push_back(time);
}

void InfoStore::addEnd(int region)
{
	struct timespec time;
	clock_gettime(CLOCK_REALTIME, &time);

	_records.push_back(InfoRecord(region, time, _records.back().configuration)); //this configuration is not relevant - must be used configuration of the region start
}

double InfoStore::getRegionDuration()
{
	double regionDuration = mutils::timeToDouble(_records.back().time - _timeStack.back());
	_timeStack.pop_back();
	return regionDuration;
}

void InfoStore::fillRegionStats(InfoStats &stats, size_t iStarts, size_t iEnd)
{
	stats.duration = mutils::timeToDouble(_records[iEnd].time - _records[iStarts].time);
	stats.startTime = mutils::timeToDouble(_records[iStarts].time);
}
	
void InfoStore::saveInterval(std::ofstream &ss, std::vector<size_t> &iStarts, size_t iEnd)
{
	ss << "# CALLTREE;init_" << _initCounter;
	for (size_t j = 0; j < iStarts.size()-1; j++) {
		ss << ";" <<  _name[_records[iStarts[j]].region] << "_" << _counters[_records[iStarts[j]].region];
	}
	ss << std::endl;

	double duration = mutils::timeToDouble(_records[iEnd].time - _records[iStarts.back()].time);
	ss << "# Job info" << std::endl
	   << "JOB ID," << _jobID << "."  << _stepID  << std::endl
	   << "Runtime of function [s],"  << std::fixed << std::setprecision  (6) << duration << std::endl
	   << "Function start timestamp," << std::fixed << std::setprecision (15) << mutils::timeToDouble(_records[iStarts.back()].time) << std::endl;
}


