
#include "detailedenergystore.h"

using namespace meric;

void DetailedEnergyStore::addStart(const std::string &region)
{
	int regionID = getRegionID(region);

	std::array<double, PROBE_NUMBER> energy;
	timespec time;
	HDEEM::fillDetailedEnergy(energy, time);
	_records.push_back(DetailedEnergyRecord(regionID, energy, time));
	_stack.push_back(regionID);
}

void DetailedEnergyStore::addEnd()
{
	if (_stack.size()) {
		std::array<double, PROBE_NUMBER> energy;
		timespec time;
		HDEEM::fillDetailedEnergy(energy, time);
		_records.push_back(DetailedEnergyRecord(_stack.back(), energy, time));
		_stack.pop_back();
	} else {
		std::cerr << "MERIC ERROR: Incorrect regions. There are more 'ends' than 'starts'!\n";
	}
}

void DetailedEnergyStore::saveInterval(std::ofstream &ss, std::vector<size_t> &iStarts, size_t iEnd)
{
    ss << "# CALLTREE;init_" << _initCounter;
	for (size_t j = 0; j < iStarts.size(); j++) {
		ss << ";" <<  _name[_records[iStarts[j]].region] << "_" << _counters[_records[iStarts[j]].region];
	}
	ss << std::endl;

	ss << "# Job info" << std::endl
	   << "JOB ID," << _jobID << "." << _stepID  << std::endl
	   << "Nodelist," << _nodelist << std::endl
	   << "Runtime of function [s]," << _records[iEnd].time - _records[iStarts.back()].time << std::endl;

    //////////////////
    // Blade values //
    //////////////////
	ss << "# Blade summary" << std::endl
	   << "Energy consumption [J] (Stats structure)" << "," << _records[iEnd].energy[0] - _records[iStarts.back()].energy[0] << std::endl;

    ///////////////////////
    // VR sensors values //
    ///////////////////////

    std::vector<std::string> vrSensor = {"CPU0", "CPU1", "DDR AB", "DDR CD", "DDR EF", "DDR GH"};
    unsigned int vrSensorNum = vrSensor.size();
    
    for (unsigned int sensorID = 0; sensorID < vrSensorNum; sensorID++) {
        ss << "# Voltage regulator (" << vrSensor[sensorID] << ") summary" << std::endl
           << "Energy consumption [J] (Stats structure)" << "," << _records[iEnd].energy[sensorID+1] - _records[iStarts.back()].energy[sensorID+1] << std::endl;
    }


}




