
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
	size_t firstBlade, lastBlade;
	size_t firstVR, lastVR;
	double * energy;
	double * min;
	double * max;
	std::string msg = "";
	stats.mericDuration = mutils::timeToDouble(_records[iEnd].clockTime - _records[iStarts].clockTime);
	energy = new double [HDEEM::PROBE_NUMBER];
	min = new double [HDEEM::PROBE_NUMBER];
	max = new double [HDEEM::PROBE_NUMBER];
	msg = HDEEM::getData(mutils::timeToDouble(_records[iStarts].clockTime - _start), stats.mericDuration, &firstBlade, &lastBlade, &firstVR, &lastVR, energy, min, max);


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

	ss << "# Job info - " << HDEEM::storeLabel << std::endl
	   << "JOB ID," << _jobID << "." << _stepID  << std::endl
	   << "Nodelist," << _nodelist << std::endl
	   << "Runtime of function [s]," << _records[iEnd].clockTime - _records[iStarts.back()].clockTime << std::endl;

	size_t firstBlade, lastBlade;
	size_t firstVR, lastVR;
	double * energy;
	double * minWatts;
	double * maxWatts;
	double avgWatts = 0.0;
	double energyConsumption = 0.0;

	std::string msg = "";
	double duration = mutils::timeToDouble(_records[iEnd].clockTime - _records[iStarts.back()].clockTime);
	energy   = new double [HDEEM::PROBE_NUMBER];
	minWatts = new double [HDEEM::PROBE_NUMBER];
	maxWatts = new double [HDEEM::PROBE_NUMBER];
	msg = HDEEM::getData(mutils::timeToDouble(_records[iStarts.back()].clockTime - _start), duration, &firstBlade, &lastBlade, &firstVR, &lastVR, energy, minWatts, maxWatts);

	if (msg != "")
		std::cerr << msg <<  _name[_records[iStarts.back()].region]<< std::endl;

    //////////////////
    // Blade values //
    //////////////////

	if(printSamples)
		ss << "# Blade samples" << std::endl;

	if(lastBlade > 0 && lastBlade > firstBlade)
	{
		if (printSamples)
		{
			ss << "# Blade samples" << std::endl;
			std::vector<double> samples;
			HDEEM::getSamplesList(0, firstBlade, lastBlade, samples);
			for (size_t it = firstBlade; it < lastBlade; it++)
				ss << it << "," << samples[it] << std::endl;
		}
		avgWatts = energy[0]/(lastBlade-firstBlade);
		energyConsumption = duration * avgWatts;
	} else 
	{
		firstBlade = 0;
		lastBlade = 1;
		std::cerr << "MERIC ERROR: No samples, probably too short ("<<duration<<"s) region "<< _name[_records[iStarts.back()].region]<<"\n";
	}
	
	if (_records[iEnd].energy[0] - _records[iStarts.back()].energy[0] <= 0)
	{
		std::cerr << "MERIC ERROR: No energy consumption, probably too short ("<<_records[iEnd].clockTime - _records[iStarts.back()].clockTime<<"s) region "<< _name[_records[iStarts.back()].region]<<"\n";
	}
	
	ss << "# Blade summary" << std::endl
	   << "Energy consumption [J] (Stats structure)" << "," << _records[iEnd].energy[0] - _records[iStarts.back()].energy[0] << std::endl
	   << "Energy consumption [J] (Samples)" << "," << energyConsumption << std::endl
       << "Maximal power [W] (Samples)" << "," << maxWatts[0] << std::endl
       << "Minimal power [W] (Samples)" << "," << minWatts[0] << std::endl
       << "Average power [W] (Samples)" << "," << avgWatts << std::endl
       << "ID of first blade sample" << "," << firstBlade << std::endl
       << "ID of last blade sample" << "," << lastBlade-1 << std::endl;


    ///////////////////////
    // VR sensors values //
    ///////////////////////
	if(lastVR > 0 && lastVR > firstVR)
	{
		for (unsigned int sensorID = 1; sensorID < HDEEM::PROBE_NUMBER; sensorID++)
		{
			if(printSamples)
			{
				ss << "# Voltage regulator (" << HDEEM::vrSensorLabel[sensorID] << ") samples" << std::endl;
				std::vector<double> samples;
				HDEEM::getSamplesList(sensorID, firstVR, lastVR, samples);
				for (size_t it = firstVR; it < lastVR; it++)
					ss << it << "," << samples[it] << std::endl;
			}
	        avgWatts = energy[sensorID]/(lastVR-firstVR);
	        energyConsumption = duration * avgWatts;
           
			if (_records[iEnd].energy[sensorID+1] - _records[iStarts.back()].energy[sensorID+1] <= 0)
			{
				std::cerr << "MERIC WARNING: No energy consumption, probably too short ("<<_records[iEnd].clockTime - _records[iStarts.back()].clockTime<<"s) detailed region "<< _name[_records[iStarts.back()].region]<<" - " << HDEEM::vrSensorLabel[sensorID]<<"\n";
			}
           
		    ss << "# Voltage regulator (" << HDEEM::vrSensorLabel[sensorID] << ") summary" <<std::endl
		       << "Energy consumption [J] (Stats structure)" << "," << _records[iEnd].energy[sensorID+1] - _records[iStarts.back()].energy[sensorID+1] << std::endl
		       << "Energy consumption [J] (Samples)" << "," << energyConsumption << std::endl
		       << "Maximal power [W] (Samples)" << "," << maxWatts[sensorID] << std::endl
		       << "Minimal power [W] (Samples)" << "," << minWatts[sensorID] << std::endl
		       << "Average power [W] (Samples)" << "," << avgWatts << std::endl
		       << "ID of first VR sample" << "," << firstVR << std::endl
		       << "ID of last VR sample" << "," << lastVR-1 << std::endl;
		}
	} else if (firstBlade != 0 || lastBlade != 1) //no previous error about region length
	{
		std::cerr << "MERIC ERROR: No samples, probably too short ("<<duration<<"s) detailed region "<< _name[_records[iStarts.back()].region]<<"\n";
	}

	delete [] energy;
	delete [] minWatts;
	delete [] maxWatts;

}




