
#include "detailedtimestore.h"
#include <cmath>

using namespace meric;

void DetailedTimeStore::saveInterval(std::ofstream &ss, std::vector<size_t> &iStarts, size_t iEnd)
{
	ss << "time ";
	for (size_t j = 0; j < iStarts.size(); j++) {
		ss << _name[_records[iStarts[j]].region] << " ";
	}
	double start = (_records[iStarts.back()].time - _start) / _meric_period;
	double end = (_records[iEnd].time - _start) / _meric_period;
	size_t firstBlade = std::ceil(start * _samples.nb_blade_values);
	size_t lastBlade = std::floor(end * _samples.nb_blade_values);
	size_t firstVR = std::ceil(start * _samples.nb_vr_values);
	size_t lastVR = std::floor(end * _samples.nb_vr_values);

	std::vector<double> energy(PROBE_NUMBER, 0);
	for (size_t it = firstBlade; it < lastBlade; it++) {
		energy[0] += _samples.blade_power[it].value[0];
	}
	for (size_t it = firstVR; it < lastVR; it++) {
		for (size_t vr = 0; vr < PROBE_NUMBER - 1; vr++) {
			energy[vr + 1] += _samples.vr_power[it].value[vr];
		}
	}

	ss << _meric_duration * energy[0] / (lastBlade - firstBlade) << " ";
	for (size_t it = 1; it < PROBE_NUMBER; it++) {
		ss << _meric_duration * energy[it] / (lastVR - firstVR) << " ";
	}
	ss << "(" << _records[iEnd].time - _records[iStarts.back()].time << "s)\n";
}




