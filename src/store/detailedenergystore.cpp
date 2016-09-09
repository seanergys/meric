
#include "detailedenergystore.h"

using namespace meric;

void DetailedEnergyStore::addStart(const std::string &region)
{
	int regionID = getRegionID(region);

	std::array<double, PROBE_NUMBER> energy;
	timespec time;
	HDEEM::fillDetailedEnergy(energy, time);
	_records.push_back(DetailedEnergyRecord(regionID, energy, time));
	_stack.push_back(regionID);
}

void DetailedEnergyStore::addEnd()
{
	if (_stack.size()) {
		std::array<double, PROBE_NUMBER> energy;
		timespec time;
		HDEEM::fillDetailedEnergy(energy, time);
		_records.push_back(DetailedEnergyRecord(_stack.back(), energy, time));
		_stack.pop_back();
	} else {
		std::cerr << "MERIC ERROR: Incorrect regions. There are more 'ends' than 'starts'!\n";
	}
}

void DetailedEnergyStore::saveInterval(std::ofstream &ss, std::vector<size_t> &iStarts, size_t iEnd)
{
	ss << "energy ";
	for (size_t j = 0; j < iStarts.size(); j++) {
		ss << _name[_records[iStarts[j]].region] << " ";
	}

	for (size_t p = 0; p < PROBE_NUMBER; p++) {
		ss << _records[iEnd].energy[p] - _records[iStarts.back()].energy[p] << " ";
	}
	ss << "(" << _records[iEnd].time - _records[iStarts.back()].time << "s)\n";
}




