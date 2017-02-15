
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
//	_meric_duration = _meric_period.tv_sec + (double)_meric_period.tv_nsec / 1e9;

//	_start = status.start_time_blade;
	_hdeem_freq = 1000; //status.total_blade_values /_meric_duration;
	_timeAdjustment = 1000 * mutils::timeToDouble(_start - status.start_time_blade) -1;
	MERIC_INFO << "TIME ADJUSTMENT: " << _timeAdjustment << std::endl; 
//	MERIC_INFO << "DURATION: " << _meric_duration << std::endl;
	MERIC_INFO << "DURATION hdeem: " << _hdeem_duration << std::endl;
	MERIC_INFO << "HDEEM freq: "<< status.total_blade_values /_hdeem_duration <<std::endl;
//	MERIC_INFO << "REAL freq:  " << status.total_blade_values /_meric_duration << std::endl;
	MERIC_INFO << "SAMPLES hdeem: " << status.total_blade_values << std::endl;
}

void TimeStore::fillRegionStats(RegionStats &stats, size_t iStarts, size_t iEnd)
{
	stats.duration = mutils::timeToDouble(_records[iEnd].time - _records[iStarts].time);
	size_t first = std::ceil(mutils::timeToDouble(_records[iStarts].time - _start) * _hdeem_freq) + _timeAdjustment;
	size_t last  = std::floor(stats.duration * _hdeem_freq) + first;
	if (last > _samples.nb_blade_values)
	{
		MERIC_INFO << _name[_records[iStarts].region] << " LAST SAMPLE miss: " << last << ", max: " << _samples.nb_blade_values << std::endl;
		last = _samples.nb_blade_values;
	}
	if (last <= first)
	{
		std::cerr << "MERIC ERROR: No HDEEM samples, probably too short ("<<stats.duration<<"s) region "<< _name[_records[iStarts].region]<<"\n";
	}

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
	stats.duration = mutils::timeToDouble(_records[iEnd].time - _records[iStarts].time);
	size_t firstBlade = std::ceil(mutils::timeToDouble(_records[iStarts].time - _start) * _hdeem_freq) + _timeAdjustment;
	size_t lastBlade  = std::floor(stats.duration * _hdeem_freq) + firstBlade;
	if (lastBlade > _samples.nb_blade_values)
	{
		MERIC_INFO << _name[_records[iStarts].region] << " LAST SAMPLE miss: " << lastBlade << ", max: " << _samples.nb_blade_values << std::endl;
		lastBlade = _samples.nb_blade_values;
	}
	
	size_t firstVR    = firstBlade/10 + VRDELAY; //10x lower freq
	size_t lastVR     = lastBlade/10  + VRDELAY;
	if (lastVR > _samples.nb_vr_values)
		lastVR = _samples.nb_vr_values;

	if (lastBlade <= firstBlade)
	{
		std::cerr << "MERIC ERROR: No HDEEM samples, probably too short ("<<stats.duration<<"s) region "<< _name[_records[iStarts].region]<<"\n";
	}
	else if (lastVR <= firstVR)
	{
		std::cerr << "MERIC WARNING: No HDEEM samples, probably too short ("<<stats.duration<<"s) detailed region "<< _name[_records[iStarts].region]<<"\n";
	}

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
	timespec time1, time2;
	clock_gettime(CLOCK_REALTIME, &time1);
		
    ss << "# CALLTREE;init_" << _initCounter;
	for (size_t j = 0; j < iStarts.size()-1; j++) {
		ss << ";" <<  _name[_records[iStarts[j]].region] << "_" << _counters[_records[iStarts[j]].region];
	}
	ss << std::endl;

	ss << "# Job info - hdeem" << std::endl
	   << "JOB ID," << _jobID << "." << _stepID  << std::endl
	   << "Nodelist," << _nodelist << std::endl
	   << "Runtime of function [s]," << _records[iEnd].time - _records[iStarts.back()].time << std::endl;

	double duration = mutils::timeToDouble(_records[iEnd].time - _records[iStarts.back()].time);
	size_t first = std::ceil(mutils::timeToDouble(_records[iStarts.back()].time - _start) * _hdeem_freq) + _timeAdjustment;
	size_t last  = std::floor(duration * _hdeem_freq) + first;
	if (last > _samples.nb_blade_values)
	{
		MERIC_INFO << _name[_records[iStarts.back()].region] << " LAST SAMPLE miss: " << last << ", max: " << _samples.nb_blade_values << std::endl;
		last = _samples.nb_blade_values;
	}
	
	MERIC_INFO << _name[_records[iStarts.back()].region] << " FIRST: "<< first  << std::endl;
	MERIC_INFO << _name[_records[iStarts.back()].region] << " LAST:  "<< last << std::endl;

    //////////////////
    // Blade values //
    //////////////////

	ss << "# Store CONTINUAL 1" << std::endl;

	if (printSamples)
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
            if (printSamples)
	            ss << it << "," << valTmp << std::endl;
        }
        avgWatts = energy/(last-first);
        energyConsumption = duration * energy / (last - first);
    } else {
        first = 0;
        last = 1;
        std::cerr << "MERIC ERROR: No HDEEM samples, probably too short ("<<duration<<"s) region "<< _name[_records[iStarts.back()].region]<<"\n";
    }
	
	ss << "# Blade summary" << std::endl
	   << "Energy consumption [J] (Samples)" << "," << energyConsumption << std::endl
       << "Maximal power [W] (Samples)" << "," << maxWatts << std::endl
       << "Minimal power [W] (Samples)" << "," << minWatts << std::endl
       << "Average power [W] (Samples)" << "," << avgWatts << std::endl
       << "ID of first blade sample" << "," << first << std::endl
       << "ID of last blade sample" << "," << last-1 << std::endl;

	clock_gettime(CLOCK_REALTIME, &time2);
	MERIC_INFO << "MERIC record save: " << mutils::timeToDouble(time2 - time1) <<" s\n";

}
