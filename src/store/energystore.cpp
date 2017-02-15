
#include "energystore.h"
#include <sys/time.h>
#include <cmath>

using namespace meric;

void EnergyStore::addStart(int region)
{
	double energy;
	timespec time;

	HDEEM::fillEnergy(energy);
	HDEEM::fillTime(time);
	_records.push_back(EnergyRecord(region, time, energy));
}

void EnergyStore::addEnd(int region)
{
	double energy;
	timespec time;

	HDEEM::fillTime(time);
	HDEEM::fillEnergy(energy);
	_records.push_back(EnergyRecord(region, time, energy));
}

void EnergyStore::fillRegionStats(RegionStats &stats, size_t iStarts, size_t iEnd)
{
	stats.energy = _records[iEnd].energy - _records[iStarts].energy;
	stats.duration = mutils::timeToDouble(_records[iEnd].time - _records[iStarts].time);

	if (stats.energy <= 0)
	{
		std::cerr << "MERIC ERROR: No energy consumption, probably too short ("<<stats.duration<<"s) region "<< _name[_records[iStarts].region]<<"\n";
	}
	else if (stats.duration < 0.01)
	{
		std::string info = " detailed";
		if (stats.duration > 0.001)
			info = "";
		std::cerr << "MERIC WARNING: very short ("<<stats.duration<< "s)"<< info <<" region" << _name[_records[iStarts].region] << " results might be skewed\n";
	}
}
	
void EnergyStore::saveInterval(std::ofstream &ss, std::vector<size_t> &iStarts, size_t iEnd)
{
	ss << "# CALLTREE;init_" << _initCounter;
	for (size_t j = 0; j < iStarts.size()-1; j++) {
		ss << ";" <<  _name[_records[iStarts[j]].region] << "_" << _counters[_records[iStarts[j]].region];
	}
	ss << std::endl;

	double duration = mutils::timeToDouble(_records[iEnd].time - _records[iStarts.back()].time);
	ss << "# Job info - hdeem" << std::endl
	   << "JOB ID," << _jobID << "." << _stepID  << std::endl
	   << "Nodelist," << _nodelist << std::endl
	   << "Runtime of function [s]," << duration << std::endl;

	ss << "# Store CONTINUAL 0" << std::endl;
	ss << "# Blade summary" << std::endl
	   << "Energy consumption [J] (Stats structure)" << "," << _records[iEnd].energy - _records[iStarts.back()].energy << std::endl;
	   
	if (_records[iEnd].energy - _records[iStarts.back()].energy <= 0)
	{
		std::cerr << "MERIC ERROR: No energy consumption, probably too short ("<<duration<<"s) region "<< _name[_records[iStarts.back()].region]<<"\n";
	}
	else if (duration < 0.001)
	{
		std::cerr << "MERIC WARNING: very short region ("<<duration<< "s) " << _name[_records[iStarts.back()].region] << " results might be skewed\n";
	}

}




