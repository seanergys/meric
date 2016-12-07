
#include "timestore.h"
#include <cmath>

using namespace meric;

void TimeStore::addStart(int region)
{
	timespec time;
	HDEEM::fillTime(time);
	_records.push_back(TimeRecord(region, time));
}

void TimeStore::addEnd(int region)
{
	timespec time;
	HDEEM::fillTime(time);
	_records.push_back(TimeRecord(region, time));
}

void TimeStore::computePeriods()
{
	hdeem_status_t status;
	HDEEM::getStatus(status);
	_hdeem_period = status.stop_time_blade - status.start_time_blade;
	_meric_period = _end - _start;
	_hdeem_duration = _hdeem_period.tv_sec + (double)_hdeem_period.tv_nsec / 1e9;
	_meric_duration = _meric_period.tv_sec + (double)_meric_period.tv_nsec / 1e9;
}

void TimeStore::fillRegionStats(RegionStats &stats, size_t iStarts, size_t iEnd)
{
	double start = (_records[iStarts].time - _start) / _meric_period;
	double end = (_records[iEnd].time - _start) / _meric_period;
	size_t first = std::ceil(start * _samples.nb_blade_values);
	size_t last = std::floor(end * _samples.nb_blade_values);
	stats.duration = mutils::timeToDouble(_records[iEnd].time - _records[iStarts].time);

	double energy = 0;
	for (size_t it = first; it < last; it++) {
		energy += _samples.blade_power[it].value[0];
	}

    stats.energy = 0;
    if(last > 0 && last > first) {
    	stats.energy = stats.duration * energy / (last - first);
    }
}

void TimeStore::fillRegionStats(DetailedRegionStats &stats, size_t iStarts, size_t iEnd)
{
	double start = (_records[iStarts].time - _start) / _meric_period;
	double end = (_records[iEnd].time - _start) / _meric_period;
	size_t firstBlade = std::ceil(start * _samples.nb_blade_values);
	size_t lastBlade = std::floor(end * _samples.nb_blade_values);
	size_t firstVR = std::ceil(start * _samples.nb_vr_values);
	size_t lastVR = std::floor(end * _samples.nb_vr_values);
	stats.duration = mutils::timeToDouble(_records[iEnd].time - _records[iStarts].time);

	std::vector<double> energy(PROBE_NUMBER, 0);
	for (size_t it = firstBlade; it < lastBlade; it++) {
		energy[0] += _samples.blade_power[it].value[0];
	}
	for (size_t it = firstVR; it < lastVR; it++) {
		for (size_t vr = 0; vr < PROBE_NUMBER - 1; vr++) {
			energy[vr + 1] += _samples.vr_power[it].value[vr];
		}
	}


    for(size_t i=0; i < PROBE_NUMBER; i++) {
        stats.energy[i] = 0;
    }
    if(lastBlade > 0 && lastBlade > firstBlade) {
        stats.energy[0] = stats.duration * energy[0] / (lastBlade - firstBlade);
    }
    if(lastVR > 0 && lastVR > firstVR) {
        for (size_t it = 1; it < PROBE_NUMBER; it++) {
            stats.energy[it] = stats.duration * energy[it] / (lastVR - firstVR);
        }
    }

}

void TimeStore::saveInterval(std::ofstream &ss, std::vector<size_t> &iStarts, size_t iEnd)
{
    ss << "# CALLTREE;init_" << _initCounter;
	for (size_t j = 0; j < iStarts.size()-1; j++) {
		ss << ";" <<  _name[_records[iStarts[j]].region] << "_" << _counters[_records[iStarts[j]].region];
	}
	ss << std::endl;

	ss << "# Job info" << std::endl
	   << "JOB ID," << _jobID << "." << _stepID  << std::endl
	   << "Nodelist," << _nodelist << std::endl
	   << "Runtime of function [s]," << _records[iEnd].time - _records[iStarts.back()].time << std::endl;

	double start = (_records[iStarts.back()].time - _start) / _meric_period;
	double end = (_records[iEnd].time - _start) / _meric_period;
	size_t first = std::ceil(start * _samples.nb_blade_values);
	size_t last = std::floor(end * _samples.nb_blade_values);
    double duration = mutils::timeToDouble(_records[iEnd].time - _records[iStarts.back()].time);

    //////////////////
    // Blade values //
    //////////////////

	ss << "# Store CONTINUAL 1" << std::endl;
	ss << "# Blade samples" << std::endl;

	double energy = 0;
	double maxWatts = 0;
    double minWatts = 0;
    double avgWatts = 0;
    double energyConsumption = 0;

    if(last > 0 && last > first) {
        maxWatts = _samples.blade_power[first].value[0];
        minWatts = maxWatts;
        for (size_t it = first; it < last; it++) {
            double valTmp = _samples.blade_power[it].value[0];
            if (valTmp > maxWatts) {
                maxWatts = valTmp;
            } else if (valTmp < minWatts) {
                minWatts = valTmp;
            }

            energy += valTmp;
            ss << it << "," << valTmp << std::endl;
        }
        avgWatts = energy/(last-first);
        energyConsumption = duration * energy / (last - first);
    } else {
        first = 0;
        last = 1;
    }

	ss << "# Blade summary" << std::endl
	   << "Energy consumption [J] (Samples)" << "," << energyConsumption << std::endl
       << "Maximal power [W] (Samples)" << "," << maxWatts << std::endl
       << "Minimal power [W] (Samples)" << "," << minWatts << std::endl
       << "Average power [W] (Samples)" << "," << avgWatts << std::endl
       << "ID of first blade sample" << "," << first << std::endl
       << "ID of last blade sample" << "," << last-1 << std::endl;

}
