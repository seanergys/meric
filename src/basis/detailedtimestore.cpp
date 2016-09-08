
#include "detailedtimestore.h"
#include <cmath>

using namespace meric;

DetailedTimeStore::~DetailedTimeStore()
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

	std::cout << "blade samples: " << samples.nb_blade_values << "\n";
	std::cout << "VR samples: " << samples.nb_vr_values << "\n";

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
		size_t firstBlade = std::ceil(start * samples.nb_blade_values);
		size_t lastBlade = std::ceil(end * samples.nb_blade_values);
		size_t firstVR = std::ceil(start * samples.nb_vr_values);
		size_t lastVR = std::ceil(end * samples.nb_vr_values);
		std::vector<double> energy(PROBE_NUMBER, 0);
		for (size_t it = firstBlade; it < lastBlade; it++) {
			energy[0] += samples.blade_power->value[it];
		}
		for (size_t it = firstVR; it < lastVR; it++) {
			for (size_t vr = 0; vr < PROBE_NUMBER - 1; vr++) {
				energy[vr + 1] += samples.vr_power[vr].value[it];
			}
		}
		std::cout << "(" << start << " :: " << end << ") -> " << firstBlade << " :: " << lastBlade << "\n";
		std::cout << "(" << start << " :: " << end << ") -> " << firstVR << " :: " << lastVR << "\n";
		ss[_records[i].region] << energy[0] / (lastBlade - firstBlade);
		for (size_t it = 1; it < PROBE_NUMBER; it++) {
			ss[_records[i].region] << energy[it] / (lastVR - firstVR) << " ";
		}
		ss[_records[i].region] << _records[i].time - _records[starts.back()].time << "\n";
		starts.pop_back();
	}

	HDEEM::deleteSamples(samples);
	_regions.clear();
	_records.clear();
}




