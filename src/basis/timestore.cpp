
#include "timestore.h"
#include <cmath>

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
	clock_gettime(CLOCK_REALTIME, &_end);
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

	timespec hdeem_period = status.stop_time_blade - status.start_time_blade;
	timespec meric_perion = _end - _start;
    std::cout << "measurement delay: " << hdeem_period - meric_perion << "\n";

    std::cout << "samples: " << samples.nb_blade_values << "\n";

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
		double start = (_records[starts.back()].time / meric_perion);
		double end = (_records[i].time / meric_perion);
		size_t first = std::ceil(start * samples.nb_blade_values);
		size_t last = std::ceil(end * samples.nb_blade_values);
		double energy = 0;
		for (size_t it = first; it < last; it++) {
			energy += samples.blade_power->value[it];
		}
		std::cout << "(" << start << " :: " << end << ") -> " << first << " :: " << last << "\n";
		ss[_records[i].region] << energy / (last - first) << " " << _records[i].time - _records[starts.back()].time << "\n";
		starts.pop_back();
	}

	HDEEM::deleteSamples(samples);
}
