
#include "timeenergystore.h"
#include <cmath>

using namespace meric;

void TimeEnergyStore::addStart(const std::string &region)
{
	int regionID = getRegionID(region);

	timespec hdeemTime, clockTime;
	double energy;

	HDEEM::fillTime(clockTime);
	HDEEM::fillEnergy(energy, hdeemTime);
	_records.push_back(TimeEnergyRecord(regionID, hdeemTime, clockTime, energy));
	_stack.push_back(regionID);
}

void TimeEnergyStore::addEnd()
{
	if (_stack.size()) {
		timespec hdeemTime, clockTime;
		double energy;

		HDEEM::fillTime(clockTime);
		HDEEM::fillEnergy(energy, hdeemTime);
		_records.push_back(TimeEnergyRecord(_stack.back(), hdeemTime, clockTime, energy));
		_stack.pop_back();
	} else {
		std::cerr << "MERIC ERROR: Incorrect regions. There are more 'ends' than 'starts'!\n";
	}
}

void TimeEnergyStore::saveInterval(std::ofstream &ss, std::vector<size_t> &iStarts, size_t iEnd)
{
	ss << "# CALLTREE;init_" << _initCounter;
	for (size_t j = 0; j < iStarts.size(); j++) {
		ss << ";" <<  _name[_records[iStarts[j]].region] << "_" << _counters[_records[iStarts[j]].region];
	}
	ss << std::endl;

	ss << "# Job info" << std::endl
	   << "JOB ID," << _jobID << "." << _stepID  << std::endl
	   << "Nodelist," << _nodelist << std::endl
	   << "Runtime of function [s]," << _records[iEnd].clockTime - _records[iStarts.back()].clockTime << std::endl;

	double start = (_records[iStarts.back()].clockTime - _start) / _meric_period;
	double end = (_records[iEnd].clockTime - _start) / _meric_period;
	size_t first = std::ceil(start * _samples.nb_blade_values);
	size_t last = std::floor(end * _samples.nb_blade_values);

	ss << "# Blade samples" << std::endl;

	double energy = 0;
	double maxWatts = 0;
	double minWatts = 0;
	for (size_t it = first; it < last; it++) {
		double valTmp = _samples.blade_power[it].value[0];
		if (valTmp > maxWatts) {
			maxWatts = valTmp;
		} else if (valTmp < minWatts) {
			minWatts = valTmp;
		}

		energy += valTmp;
		ss << it << "," << valTmp << std::endl;
	}
	double avgWatts = energy/(last-first);


	ss << "# Blade summary" << std::endl
	   << "Energy consumption [J] (Stats structure)" << ", " << _records[iEnd].energy - _records[iStarts.back()].energy << std::endl
	   << "Energy consumption [J] (Samples)" << "," << _meric_duration * energy / (last - first) << std::endl;
}




