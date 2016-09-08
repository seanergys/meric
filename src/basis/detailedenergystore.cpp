
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

DetailedEnergyStore::~DetailedEnergyStore()
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
		ss[_records[i].region] << "= ";
		for (size_t p = 0; p < PROBE_NUMBER; p++) {
			ss[_records[i].region] << _records[i].energy[p] - _records[starts.back()].energy[p] << " ";
		}
		ss[_records[i].region] << "(" << subTimes(_records[i].time, _records[starts.back()].time) << ")\n";
		starts.pop_back();
	}
}




