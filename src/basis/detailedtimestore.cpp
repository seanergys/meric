
#include "detailedtimestore.h"

using namespace meric;

DetailedTimeStore::~DetailedTimeStore()
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
		ss[_records[i].region] << "detailed time ";
		for (size_t j = 0; j < starts.size(); j++) {
			ss[_records[i].region] << name[_records[starts[j]].region] << " ";
		}
		ss[_records[i].region] << subTimes(_records[i].time, _records[starts.back()].time) << "\n";
		starts.pop_back();
	}
	_regions.clear();
	_records.clear();
}




