
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
	_hdeem_period = status.stop_time_blade - status.start_time_blade;
	_meric_period = _end - _start;
	_hdeem_duration = _hdeem_period.tv_sec + (double)_hdeem_period.tv_nsec / 1e9;
	_meric_duration = _meric_period.tv_sec + (double)_meric_period.tv_nsec / 1e9;
	std::cout << "measurement error: " << _hdeem_duration - _meric_duration << "\n";
}

void TimeStore::saveInterval(std::ofstream &ss, std::vector<size_t> &iStarts, size_t iEnd)
{
    std::cout << "TESTS 2" << std::endl;

	ss << "time ";
	for (size_t j = 0; j < iStarts.size(); j++) {
		ss << _name[_records[iStarts[j]].region] << " ";
	}

	double start = (_records[iStarts.back()].time - _start) / _meric_period;
	double end = (_records[iEnd].time - _start) / _meric_period;
	size_t first = std::ceil(start * _samples.nb_blade_values);
	size_t last = std::floor(end * _samples.nb_blade_values);

    ss << "# Blade samples" << std::endl;

	double energy = 0;
	for (size_t it = first; it < last; it++) {
        double valTmp = _samples.blade_power[it].value[0]; 
		energy += valTmp;
        ss << it << "," << valTmp << std::endl;
	}

    ss.seekp(0, std::ios::beg);

    std::cout << "TEST" << std::endl
              << start << " " << end << std::endl
              << first << " " << last << std::endl
              << _samples.blade_power[0].value[0]
              << std::endl;

	ss << _meric_duration * energy / (last - first) << " (" << _records[iEnd].time - _records[iStarts.back()].time << "s)\n";
}
