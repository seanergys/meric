
#include "detailedtimestore.h"
#include <cmath>

using namespace meric;

void DetailedTimeStore::saveInterval(std::ofstream &ss, std::vector<size_t> &iStarts, size_t iEnd)
{
	ss << "# CALLTREE;init_" << _initCounter;
	for (size_t j = 0; j < iStarts.size()-1; j++)
	{
		ss << ";" <<  _name[_records[iStarts[j]].region] << "_" << _counters[_records[iStarts[j]].region];
	}
	ss << std::endl;

//	ss << "# Job info - " << HDEEM::storeLabel << std::endl
//	   << "JOB ID," << _jobID << "." << _stepID  << std::endl
//	   << "Nodelist," << _nodelist << std::endl
//	   << "Runtime of function [s]," << _records[iEnd].time - _records[iStarts.back()].time << std::endl;

	size_t firstBlade, lastBlade;
	size_t firstVR, lastVR;
	double * energy;
	double * minWatts;
	double * maxWatts;
	double avgWatts = 0.0;
	double energyConsumption = 0.0;

	std::string msg = "";
	double duration = mutils::timeToDouble(_records[iEnd].time - _records[iStarts.back()].time);
	energy   = new double [HDEEM::PROBE_NUMBER];
	minWatts = new double [HDEEM::PROBE_NUMBER];
	maxWatts = new double [HDEEM::PROBE_NUMBER];
	msg = HDEEM::getData(mutils::timeToDouble(_records[iStarts.back()].time), duration, &firstBlade, &lastBlade, &firstVR, &lastVR, energy, minWatts, maxWatts);

	if (msg != "")
		std::cerr << msg <<  _name[_records[iStarts.back()].region]<< std::endl;

	//////////////////
	// Blade values //
	//////////////////
    
	ss << "# Store CONTINUAL 1 - "<< HDEEM::storeLabel << std::endl;

	if(lastBlade > 0 && lastBlade > firstBlade)
	{
		if (printSamples)
		{
			ss << "# Blade (" << HDEEM::storeLabel <<") samples [W]"<< std::endl;
			std::vector<double> samples;
			HDEEM::getSamplesList(0, firstBlade, lastBlade, samples);
			size_t it = firstBlade;
			for (size_t i = 0; i < samples.size(); i++)
				ss << it++ << "," << samples[i] << std::endl;
		}
		avgWatts = energy[0]/(lastBlade-firstBlade);
		energyConsumption = duration * avgWatts;
	} else 
	{
		firstBlade = 0;
		lastBlade = 1;
		std::cerr << "MERIC ERROR: No samples, probably too short ("<<duration<<"s) region "<< _name[_records[iStarts.back()].region]<<"\n";
	}
	
	ss << "# Blade summary - " << HDEEM::storeLabel << std::endl
	   << "Runtime of function [s]," << duration << std::endl
	   << "Energy consumption [J]" << "," << energyConsumption << std::endl
       << "Maximal power [W]" << "," << maxWatts[0] << std::endl
       << "Minimal power [W]" << "," << minWatts[0] << std::endl
       << "Average power [W]" << "," << avgWatts << std::endl
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
				ss << "# Voltage regulator (" << HDEEM::vrSensorLabel[sensorID] << ") samples [W]" << std::endl;
				std::vector<double> samples;
				HDEEM::getSamplesList(sensorID, firstVR, lastVR, samples);
				size_t it = firstVR;
				for (size_t i = 0; i < samples.size(); i++)
					ss << it++ << "," << samples[i] << std::endl;

			}
	        avgWatts = energy[sensorID]/(lastVR-firstVR);
	        energyConsumption = duration * avgWatts;

		    ss << "# Voltage regulator (" << HDEEM::vrSensorLabel[sensorID] << ") summary" <<std::endl
		       << "Energy consumption [J]" << "," << energyConsumption << std::endl
		       << "Maximal power [W]" << "," << maxWatts[sensorID] << std::endl
		       << "Minimal power [W]" << "," << minWatts[sensorID] << std::endl
		       << "Average power [W]" << "," << avgWatts << std::endl
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




