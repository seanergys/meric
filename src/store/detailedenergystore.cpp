
#include "detailedenergystore.h"

using namespace meric;

void DetailedEnergyStore::addStart(int region)
{
	std::array<double, HDEEM::PROBE_NUMBER> energy;
	struct timespec time;

	HDEEM::getDetailedCounterValues(energy);
	clock_gettime(CLOCK_REALTIME, &time);

	_records.push_back(DetailedEnergyRecord(region, energy, time));
}

void DetailedEnergyStore::addEnd(int region)
{
	std::array<double, HDEEM::PROBE_NUMBER> energy;
	struct timespec time;

	clock_gettime(CLOCK_REALTIME, &time);
	HDEEM::getDetailedCounterValues(energy);
	
	_records.push_back(DetailedEnergyRecord(region, energy, time));
}

void DetailedEnergyStore::saveInterval(std::ofstream &ss, std::vector<size_t> &iStarts, size_t iEnd)
{
    ss << "# CALLTREE;init_" << _initCounter;
	for (size_t j = 0; j < iStarts.size()-1; j++) {
		ss << ";" <<  _name[_records[iStarts[j]].region] << "_" << _counters[_records[iStarts[j]].region];
	}
	ss << std::endl;

	double duration = mutils::timeToDouble(_records[iEnd].time - _records[iStarts.back()].time);
//	ss << "# Job info - hdeem" << std::endl
//	   << "JOB ID," << _jobID << "." << _stepID  << std::endl
//	   << "Nodelist," << _nodelist << std::endl
//	   << "Runtime of function [s]," << duration << std::endl;

    //////////////////
    // Blade values //
    //////////////////
    ss << "# Store CONTINUAL 0 - hdeem" << std::endl;
	ss << "# Blade summary" << std::endl
	   << "Runtime of function [s]," << duration << std::endl
	   << "Energy consumption [J]" << "," << _records[iEnd].energy[0] - _records[iStarts.back()].energy[0] << std::endl;

	if (_records[iEnd].energy[0] - _records[iStarts.back()].energy[0] <= 0)
	{
		std::cerr << "MERIC ERROR: No energy consumption, probably too short ("<<duration<<"s) region "<< _name[_records[iStarts.back()].region]<<"\n";
	}
	else if (duration < 0.01)
	{
		std::string info = " detailed";
		if (duration > 0.001)
			info = "";
		std::cerr << "MERIC WARNING: very short (" <<duration<<"s)"<< info <<" region " << _name[_records[iStarts.back()].region] << " results might be skewed\n";
	}

    ///////////////////////
    // VR sensors values //
    ///////////////////////

    std::vector<std::string> vrSensor = {"CPU0", "CPU1", "DDR AB", "DDR CD", "DDR EF", "DDR GH"};
    unsigned int vrSensorNum = vrSensor.size();
    
    for (unsigned int sensorID = 0; sensorID < vrSensorNum; sensorID++)
    {
		ss << "# Voltage regulator (" << vrSensor[sensorID] << ") summary" << std::endl
		   << "Energy consumption [J]" << "," << _records[iEnd].energy[sensorID+1] - _records[iStarts.back()].energy[sensorID+1] << std::endl;
		
		if (_records[iEnd].energy[sensorID+1] - _records[iStarts.back()].energy[sensorID+1] <= 0)
		{
			std::cerr << "MERIC WARNING: No energy consumption, probably too short ("<<duration<<"s) detailed region "<< _name[_records[iStarts.back()].region]<<" - " << vrSensor[sensorID]<<"\n";
		}
    }


}




