
#include "energystore.h"

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


EnergyStore::~EnergyStore()
{
	std::vector<std::ofstream> ss(_regions.size());
	std::vector<std::string> name(_regions.size());
	for (auto it = _regions.begin(); it != _regions.end(); ++it) {
		ss[it->second].open(it->first.c_str());
		name[it->second] = it->first;
	}

	std::vector<size_t> starts;
	for (size_t i = 0; i < _records.size(); i++) {
		if (!starts.size() || _records[starts.back()].region != _records[i].region) {
			starts.push_back(i);
			continue;
		}
		ss[_records[i].region] << "energy ";
		for (size_t j = 0; j < starts.size(); j++) {
			ss[_records[i].region] << name[_records[starts[j]].region] << " ";
		}
		ss[_records[i].region] << "= " << _records[i].energy - _records[starts.back()].energy << "\n";
		starts.pop_back();
	}
}



