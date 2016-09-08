
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
	timespec meric_period = _end - _start;
	double hdeem_duration = hdeem_period.tv_sec + (double)hdeem_period.tv_nsec / 1e9;
	double meric_duration = meric_period.tv_sec + (double)meric_period.tv_nsec / 1e9;
	std::cout << "measurement delay: " << hdeem_duration - meric_duration << "\n";

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
		double start = (_records[starts.back()].time - _start) / meric_period;
		double end = (_records[i].time - _start) / meric_period;
		size_t first = std::ceil(start * samples.nb_blade_values);
		size_t last = std::floor(end * samples.nb_blade_values);
		double energy = 0;
		for (size_t it = first; it < last; it++) {
			energy += samples.blade_power[it].value[0];
		}
		ss[_records[i].region]
		   << meric_duration * energy / (last - first)
		   << " ("<< _records[i].time - _records[starts.back()].time << "s)\n";
		starts.pop_back();
	}

	HDEEM::deleteSamples(samples);
}
