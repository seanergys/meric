
#include "timeenergystore.h"
#include <cmath>

using namespace meric;

void TimeEnergyStore::addStart(const std::string &region)
{
	int regionID = getRegionID(region);

	timespec hdeemTime, clockTime;
    std::array<double, PROBE_NUMBER> energy;

    HDEEM::fillDetailedEnergy(energy, hdeemTime);
	HDEEM::fillTime(clockTime);

	_records.push_back(TimeEnergyRecord(regionID, hdeemTime, clockTime, energy));
	_stack.push_back(regionID);
}

void TimeEnergyStore::addEnd()
{
	if (_stack.size()) {
		timespec hdeemTime, clockTime;
	    std::array<double, PROBE_NUMBER> energy;

        HDEEM::fillDetailedEnergy(energy, hdeemTime);
		HDEEM::fillTime(clockTime);

		_records.push_back(TimeEnergyRecord(_stack.back(), hdeemTime, clockTime, energy));
		_stack.pop_back();
	} else {
		std::cerr << "MERIC ERROR: Incorrect regions. There are more 'ends' than 'starts'!\n";
	}
}


void TimeEnergyStore::fillRegionStats(DebugRegionStats &stats, size_t iStarts, size_t iEnd)
{
	double start = (_records[iStarts].clockTime - _start) / _meric_period;
	double end = (_records[iEnd].clockTime - _start) / _meric_period;
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

	stats.mericDuration = mutils::timeToDouble(_records[iEnd].clockTime - _records[iStarts].clockTime);
	stats.hdeemDuration = mutils::timeToDouble(_records[iEnd].hdeemTime - _records[iStarts].hdeemTime);

    // Init energy with 0
    for(size_t it = 0; it < PROBE_NUMBER; it++) {
        stats.energy[it] = 0;
    }

    if(lastBlade > 0 && lastBlade > firstBlade) {
        stats.energy[0] = stats.mericDuration * energy[0] / (lastBlade - firstBlade);
    }

    if(lastVR > 0 && lastVR > firstVR) {
        for (size_t it = 1; it < PROBE_NUMBER; it++) {
            stats.energy[it] = stats.mericDuration * energy[it] / (lastVR - firstVR);
        }
    }
 
    for(size_t it = 0; it < PROBE_NUMBER; it++) {
        stats.energyStatsStructure[it] = _records[iEnd].energy[it] - _records[iStarts].energy[it];
    }
}

void TimeEnergyStore::saveInterval(std::ofstream &ss, std::vector<size_t> &iStarts, size_t iEnd)
{
	ss << "# CALLTREE;init_" << _initCounter;
	for (size_t j = 0; j < iStarts.size(); j++) {
		ss << ";" <<  _name[_records[iStarts[j]].region] << "_" << _counters[_records[iStarts[j]].region];
	}
	ss << std::endl;

	ss << "# Job info" << std::endl
	   << "JOB ID," << _jobID << "." << _stepID  << std::endl
	   << "Nodelist," << _nodelist << std::endl
	   << "Runtime of function [s]," << _records[iEnd].clockTime - _records[iStarts.back()].clockTime << std::endl;

	double start = (_records[iStarts.back()].clockTime - _start) / _meric_period;
	double end = (_records[iEnd].clockTime - _start) / _meric_period;
	size_t first = std::ceil(start * _samples.nb_blade_values);
	size_t last = std::floor(end * _samples.nb_blade_values);
    double duration = mutils::timeToDouble(_records[iEnd].clockTime - _records[iStarts.back()].clockTime);

    //////////////////
    // Blade values //
    //////////////////

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
	   << "Energy consumption [J] (Stats structure)" << "," << _records[iEnd].energy[0] - _records[iStarts.back()].energy[0] << std::endl
	   << "Energy consumption [J] (Samples)" << "," << energyConsumption << std::endl
       << "Maximal power [W] (Samples)" << "," << maxWatts << std::endl
       << "Minimal power [W] (Samples)" << "," << minWatts << std::endl
       << "Average power [W] (Samples)" << "," << avgWatts << std::endl
       << "ID of first blade sample" << "," << first << std::endl
       << "ID of last blade sample" << "," << last-1 << std::endl;


    ///////////////////////
    // VR sensors values //
    ///////////////////////

    std::vector<std::string> vrSensor = {"CPU0", "CPU1", "DDR AB", "DDR CD", "DDR EF", "DDR GH"};
    unsigned int vrSensorNum = vrSensor.size();
    first = std::ceil(start * _samples.nb_vr_values);
    last  = std::floor(end * _samples.nb_vr_values);
    
    for (unsigned int sensorID = 0; sensorID < vrSensorNum; sensorID++) {
        double firstPrinted = first;
        double lastPrinted = last;

        ss << "# Voltage regulator (" << vrSensor[sensorID] << ") samples" << std::endl;
        energy = 0;
        maxWatts = 0;
        minWatts = 0;
        avgWatts = 0;
        energyConsumption = 0;

        if(last > 0 && last > first) {
            maxWatts = _samples.vr_power[first].value[sensorID];
            minWatts = maxWatts;
            for(size_t it = first; it < last; it++) {
                double valTmp = _samples.vr_power[it].value[sensorID];

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
            firstPrinted = 0;
            lastPrinted = 1;
        }

        ss << "# Voltage regulator (" << vrSensor[sensorID] << ") summary" << std::endl
           << "Energy consumption [J] (Stats structure)" << "," << _records[iEnd].energy[sensorID+1] - _records[iStarts.back()].energy[sensorID+1] << std::endl
           << "Energy consumption [J] (Samples)" << "," << energyConsumption << std::endl
           << "Maximal power [W] (Samples)" << "," << maxWatts << std::endl
           << "Minimal power [W] (Samples)" << "," << minWatts << std::endl
           << "Average power [W] (Samples)" << "," << avgWatts << std::endl
           << "ID of first VR sample" << "," << firstPrinted << std::endl
           << "ID of last VR sample" << "," << lastPrinted-1 << std::endl;
    }

}




