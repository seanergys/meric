
#include "detailedtimestore.h"
#include <cmath>

using namespace meric;

void DetailedTimeStore::saveInterval(std::ofstream &ss, std::vector<size_t> &iStarts, size_t iEnd)
{
	ss << "# CALLTREE;init_" << _initCounter;
	for (size_t j = 0; j < iStarts.size()-1; j++) {
		ss << ";" <<  _name[_records[iStarts[j]].region] << "_" << _counters[_records[iStarts[j]].region];
	}
	ss << std::endl;

	ss << "# Job info - hdeem" << std::endl
	   << "JOB ID," << _jobID << "." << _stepID  << std::endl
	   << "Nodelist," << _nodelist << std::endl
       //<< "Start of function," << _records[iStarts.back()].time << std::endl
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
        std::cerr << "MERIC ERROR: No HDEEM samples, probably too short ("<<duration<<"s) region "<< _name[_records[iStarts.back()].region]<<"\n";
    }

	ss << "# Blade summary" << std::endl
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

	double firstPrinted = first;
	double lastPrinted = last; 
	if(last < 1 || last <= first) 
	{
		std::cerr << "MERIC ERROR: No HDEEM samples, probably too short ("<<duration<<"s) detailed region "<< _name[_records[iStarts.back()].region]<<"\n";
		firstPrinted = 0;
		lastPrinted = 1;
	}
	
	if ((1+last-first) < duration*900) //hdeem should take 1000 samples per second
	{
		std::cerr << "MERIC WARNING: LOW amount of HDEEM samples in region " << _name[_records[iStarts.back()].region]<<"\n";
	}
    
    for (unsigned int sensorID = 0; sensorID < vrSensorNum; sensorID++) {
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
        }

        ss << "# Voltage regulator (" << vrSensor[sensorID] << ") summary" << std::endl
           << "Energy consumption [J] (Samples)" << "," << energyConsumption << std::endl
           << "Maximal power [W] (Samples)" << "," << maxWatts << std::endl
           << "Minimal power [W] (Samples)" << "," << minWatts << std::endl
           << "Average power [W] (Samples)" << "," << avgWatts << std::endl
           << "ID of first VR sample" << "," << firstPrinted << std::endl
           << "ID of last VR sample" << "," << lastPrinted-1 << std::endl;
    }


}




