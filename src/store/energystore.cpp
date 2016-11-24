
#include "energystore.h"
#include <sys/time.h>
#include <cmath>

using namespace meric;

void EnergyStore::addStart(const std::string &region)
{
	int regionID = getRegionID(region);
	double energy;
	timespec time;

	HDEEM::fillEnergy(energy, time);
	_records.push_back(EnergyRecord(_regions[region], time, energy));
}

void EnergyStore::addEnd(int region)
{
	double energy;
	timespec time;

	HDEEM::fillEnergy(energy, time);
	_records.push_back(EnergyRecord(region, time, energy));
}

void EnergyStore::saveInterval(std::ofstream &ss, std::vector<size_t> &iStarts, size_t iEnd)
{
	ss << "# CALLTREE;init_" << _initCounter;
	for (size_t j = 0; j < iStarts.size()-1; j++) {
		ss << ";" <<  _name[_records[iStarts[j]].region] << "_" << _counters[_records[iStarts[j]].region];
	}
	ss << std::endl;

	ss << "# Job info" << std::endl
	   << "JOB ID," << _jobID << "." << _stepID  << std::endl
	   << "Nodelist," << _nodelist << std::endl
	   << "Runtime of function [s]," << _records[iEnd].time - _records[iStarts.back()].time << std::endl;

	ss << "# Blade summary" << std::endl
	   << "Energy consumption [J] (Stats structure)" << "," << _records[iEnd].energy - _records[iStarts.back()].energy << std::endl;

}




