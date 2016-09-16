
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
	_stack.push_back(_regions[region]);

}

void EnergyStore::addEnd()
{
	if (_stack.size()) {
		double energy;
		timespec time;

		HDEEM::fillEnergy(energy, time);
		_records.push_back(EnergyRecord(_stack.back(), time, energy));
		_stack.pop_back();
	} else {
		std::cerr << "MERIC ERROR: Incorrect regions. There are more 'ends' than 'starts'!\n";
	}
}

void EnergyStore::saveInterval(std::ofstream &ss, std::vector<size_t> &iStarts, size_t iEnd)
{
	ss << "energy ";
	for (size_t j = 0; j < iStarts.size(); j++) {
		ss << _name[_records[iStarts[j]].region] << " ";
	}
	ss << "= " << _records[iEnd].energy - _records[iStarts.back()].energy
	   << " (" << _records[iEnd].time - _records[iStarts.back()].time << "s)\n";
}




