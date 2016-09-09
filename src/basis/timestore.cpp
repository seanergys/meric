
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

void TimeStore::computePeriods()
{
	hdeem_status_t status;
	HDEEM::getStatus(status);
	timespec hdeem_period = status.stop_time_blade - status.start_time_blade;
	timespec meric_period = _end - _start;
	double hdeem_duration = hdeem_period.tv_sec + (double)hdeem_period.tv_nsec / 1e9;
	double meric_duration = meric_period.tv_sec + (double)meric_period.tv_nsec / 1e9;
	std::cout << "measurement error: " << hdeem_duration - meric_duration << "\n";
}

void TimeStore::saveInterval(std::ofstream &ss, std::vector<size_t> &iStarts, size_t iEnd)
{
	ss << "time ";
	for (size_t j = 0; j < iStarts.size(); j++) {
		ss << _name[_records[iStarts[j]].region] << " ";
	}

	double start = (_records[iStarts.back()].time - _start) / _meric_period;
	double end = (_records[iEnd].time - _start) / _meric_period;
	size_t first = std::ceil(start * _samples.nb_blade_values);
	size_t last = std::floor(end * _samples.nb_blade_values);

	double energy = 0;
	for (size_t it = first; it < last; it++) {
		energy += _samples.blade_power[it].value[0];
	}

	ss << _meric_duration * energy / (last - first) << " ("<< _records[iEnd].time - _records[iStarts.back()].time << "s)\n";
}
