
#include "timeenergystore.h"
#include <cmath>

using namespace meric;

void TimeEnergyStore::addStart(int region)
{
	timespec time;
	std::array<double, HDEEM::PROBE_NUMBER> energy;

	HDEEM::fillDetailedEnergy(energy);
	HDEEM::fillTime(time);

	_records.push_back(TimeEnergyRecord(region, time, energy));
}

void TimeEnergyStore::addEnd(int region)
{
	timespec time;
	std::array<double, HDEEM::PROBE_NUMBER> energy;

	HDEEM::fillTime(time);
    HDEEM::fillDetailedEnergy(energy);

	_records.push_back(TimeEnergyRecord(region, time, energy));
}


void TimeEnergyStore::fillRegionStats(DebugRegionStats &stats, size_t iStarts, size_t iEnd)
{
	stats.mericDuration = mutils::timeToDouble(_records[iEnd].clockTime - _records[iStarts].clockTime);
	size_t firstBlade = std::ceil(mutils::timeToDouble(_records[iStarts].clockTime - _start) * _samples_freq) + _timeAdjustment;
	size_t lastBlade  = std::floor(stats.mericDuration * _samples_freq) + firstBlade;
	if (lastBlade > _samples.nb_blade_values)
	{
		MERIC_INFO << _name[_records[iStarts].region] << " LAST SAMPLE miss: " << lastBlade << ", max: " << _samples.nb_blade_values << std::endl;
		lastBlade = _samples.nb_blade_values;
	}
	MERIC_INFO << _name[_records[iStarts].region] << " FIRST: "<< firstBlade  << std::endl;
	MERIC_INFO << _name[_records[iStarts].region] << " LAST:  "<< lastBlade << std::endl;

	size_t firstVR    = firstBlade/10 + HDEEM::VRDELAY; //10x lower freq
	size_t lastVR     = lastBlade/10  + HDEEM::VRDELAY;
	if (lastVR > _samples.nb_vr_values)
		lastVR = _samples.nb_vr_values;
	
	std::vector<double> energy(HDEEM::PROBE_NUMBER, 0);
	for (size_t it = firstBlade; it < lastBlade; it++) {
		energy[0] += _samples.blade_power[it].value[0];
	}
	for (size_t it = firstVR; it < lastVR; it++) {
		for (size_t vr = 0; vr < HDEEM::PROBE_NUMBER - 1; vr++) {
			energy[vr + 1] += _samples.vr_power[it].value[vr];
		}
	}

    // Init energy with 0
    for(size_t it = 0; it < HDEEM::PROBE_NUMBER; it++) {
        stats.energy[it] = 0;
    }

    if(lastBlade > 0 && lastBlade > firstBlade) {
        stats.energy[0] = stats.mericDuration * energy[0] / (lastBlade - firstBlade);
    }

    if(lastVR > 0 && lastVR > firstVR) {
        for (size_t it = 1; it < HDEEM::PROBE_NUMBER; it++) {
            stats.energy[it] = stats.mericDuration * energy[it] / (lastVR - firstVR);
        }
    }
 
    for(size_t it = 0; it < HDEEM::PROBE_NUMBER; it++) {
        stats.energyStatsStructure[it] = _records[iEnd].energy[it] - _records[iStarts].energy[it];
    }
}

void TimeEnergyStore::saveInterval(std::ofstream &ss, std::vector<size_t> &iStarts, size_t iEnd)
{
	ss << "# CALLTREE;init_" << _initCounter;
	for (size_t j = 0; j < iStarts.size()-1; j++) {
		ss << ";" <<  _name[_records[iStarts[j]].region] << "_" << _counters[_records[iStarts[j]].region];
	}
	ss << std::endl;

	ss << "# Job info - hdeem" << std::endl
	   << "JOB ID," << _jobID << "." << _stepID  << std::endl
	   << "Nodelist," << _nodelist << std::endl
	   << "Runtime of function [s]," << _records[iEnd].clockTime - _records[iStarts.back()].clockTime << std::endl;

    double duration = mutils::timeToDouble(_records[iEnd].clockTime - _records[iStarts.back()].clockTime);
	size_t first = std::ceil(mutils::timeToDouble(_records[iStarts.back()].clockTime - _start) * _samples_freq) + _timeAdjustment;
	size_t last  = std::floor(duration * _samples_freq) + first;
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

	if(printSamples)
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
    	std::cerr << "MERIC ERROR: No HDEEM samples, probably too short ("<<duration<<"s) region "<< _name[_records[iStarts.back()].region]<<"\n";
        first = 0;
        last = 1;
    }
	
	if (_records[iEnd].energy[0] - _records[iStarts.back()].energy[0] <= 0)
	{
		std::cerr << "MERIC ERROR: No energy consumption, probably too short ("<<_records[iEnd].clockTime - _records[iStarts.back()].clockTime<<"s) region "<< _name[_records[iStarts.back()].region]<<"\n";
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

	first = first/10 + HDEEM::VRDELAY; //10x lower freq
	last  = last/10  + HDEEM::VRDELAY;
	if (last > _samples.nb_vr_values)
		last = _samples.nb_vr_values;


	double firstPrinted = first;
	double lastPrinted = last;
	if(last < 1 || last <= first) 
	{
		std::cerr << "MERIC WARNING: No HDEEM samples, probably too short ("<<duration<<"s) detailed region "<< _name[_records[iStarts.back()].region]<<"\n";
		firstPrinted = 0;
		lastPrinted = 1;
	}

    for (unsigned int sensorID = 0; sensorID < vrSensorNum; sensorID++)
    {
    	if (printSamples)
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
                if(printSamples)
	                ss << it << "," << valTmp << std::endl;
            }
            
            avgWatts = energy/(last-first);
            energyConsumption = duration * energy / (last - first);
        } 
        
        ss << "# Voltage regulator (" << vrSensor[sensorID] << ") summary" << std::endl
           << "Energy consumption [J] (Stats structure)" << "," << _records[iEnd].energy[sensorID+1] - _records[iStarts.back()].energy[sensorID+1] << std::endl
           << "Energy consumption [J] (Samples)" << "," << energyConsumption << std::endl
           << "Maximal power [W] (Samples)" << "," << maxWatts << std::endl
           << "Minimal power [W] (Samples)" << "," << minWatts << std::endl
           << "Average power [W] (Samples)" << "," << avgWatts << std::endl
           << "ID of first VR sample" << "," << firstPrinted << std::endl
           << "ID of last VR sample" << "," << lastPrinted-1 << std::endl;
		
		if (_records[iEnd].energy[sensorID+1] - _records[iStarts.back()].energy[sensorID+1] <= 0)
		{
			std::cerr << "MERIC WARNING: No energy consumption, probably too short ("<<_records[iEnd].clockTime - _records[iStarts.back()].clockTime<<"s) detailed region "<< _name[_records[iStarts.back()].region]<<" - " << vrSensor[sensorID]<<"\n";
		}
    }

}




