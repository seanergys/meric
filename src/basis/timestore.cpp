
#include "timestore.h"

using namespace meric;

void TimeStore::addStart(const std::string &region)
{
	int regionID = getRegionID(region);

	timespec time;
	HDEEM::fillTime(time);
	_records.push_back(TimeRecord(regionID, time));
	_stack.push_back(regionID);
}

void TimeStore::addEnd()
{
	if (_stack.size()) {
		timespec time;
		HDEEM::fillTime(time);
		_records.push_back(TimeRecord(_stack.back(), time));
		_stack.pop_back();
	} else {
		std::cerr << "MERIC ERROR: Incorrect regions. There are more 'ends' than 'starts'!\n";
	}
}

TimeStore::~TimeStore()
{
	std::vector<std::ofstream> ss(_regions.size());
	std::vector<std::string> name(_regions.size());
	for (auto it = _regions.begin(); it != _regions.end(); ++it) {
		ss[it->second].open(it->first.c_str());
		name[it->second] = it->first;
	}

	hdeem_global_reading_t samples;
	hdeem_status_t status;
	HDEEM::getStatus(status);
	HDEEM::getSamples(samples);

	std::cout << "measurement: " << status.start_time_blade << " to " << status.stop_time_blade << "\n";

	std::vector<size_t> starts;
	for (size_t i = 0; i < _records.size(); i++) {
		if (!starts.size() || _records[starts.back()].region != _records[i].region) {
			starts.push_back(i);
			continue;
		}
		ss[_records[i].region] << "time ";
		for (size_t j = 0; j < starts.size(); j++) {
			ss[_records[i].region] << name[_records[starts[j]].region] << " ";
		}
		ss[_records[i].region]
		   << subTimes(_records[i].time, _records[starts.back()].time) << "\n";
		starts.pop_back();
	}

	HDEEM::deleteSamples(samples);
}
