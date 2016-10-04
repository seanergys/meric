
#include "debugaggregatestore.h"

namespace meric {

template <class TStore>
void DebugAggregateStore<TStore>::saveInterval(std::ofstream &ss, std::vector<size_t> &iStarts, size_t iEnd)
{
	DebugRegionStats stats;
	this->_store.fillRegionStats(stats, iStarts.back(), iEnd);

	MPI_Gather(&stats, sizeof(DebugRegionStats), MPI_BYTE, _buffer.data(), sizeof(DebugRegionStats), MPI_BYTE, 0, this->_comm);


    ss << "# CALLTREE;init_" << this->_store._initCounter;
    for (size_t j = 0; j < iStarts.size()-1; j++) {
        ss << ";" <<  this->_store._name[this->_store._records[iStarts[j]].region] << "_" << this->_store._counters[this->_store._records[iStarts[j]].region];
    }
    ss << '\n';

    ss << "# Job info" << std::endl
       << "JOB ID," << this->_store._jobID << "." << this->_store._stepID  << '\n'
       << "Nodelist," << this->_store._nodelist << '\n';

        double avgEnergy = 0,
               maxEnergy = 0,
               minEnergy = _buffer[0].energy[0],
               sumEnergy = 0;
        double avgEnergyStatsStruct = 0,
               maxEnergyStatsStruct = 0,
               minEnergyStatsStruct = _buffer[0].energyStatsStructure[0],
               sumEnergyStatsStruct = 0;
        double avgDuration = 0,
               maxDuration = 0,
               minDuration = _buffer[0].mericDuration,
               sumDuration = 0;
        double avgHdeemDuration = 0,
               maxHdeemDuration = 0,
               minHdeemDuration = _buffer[0].hdeemDuration,
               sumHdeemDuration = 0;

        for (size_t i = 0; i < this->_size; i++) {
            if(maxEnergy < _buffer[i].energy[0]) {
                maxEnergy = _buffer[i].energy[0];
            } else if (minEnergy > _buffer[i].energy[0]) {
                minEnergy = _buffer[i].energy[0];
            }

            avgEnergy += _buffer[i].energy[0];

            if(maxEnergyStatsStruct < _buffer[i].energyStatsStructure[0]) {
                maxEnergyStatsStruct = _buffer[i].energyStatsStructure[0];
            } else if (minEnergyStatsStruct > _buffer[i].energyStatsStructure[0]) {
                minEnergyStatsStruct = _buffer[i].energyStatsStructure[0];
            }

            avgEnergyStatsStruct += _buffer[i].energyStatsStructure[0];

            if(maxDuration < _buffer[i].mericDuration) {
                maxDuration = _buffer[i].mericDuration;
            } else if (minDuration > _buffer[i].mericDuration) {
                minDuration = _buffer[i].mericDuration;
            }

            avgDuration += _buffer[i].mericDuration;

            if(maxHdeemDuration < _buffer[i].hdeemDuration) {
                maxHdeemDuration = _buffer[i].hdeemDuration;
            } else if (minHdeemDuration > _buffer[i].hdeemDuration) {
                minHdeemDuration = _buffer[i].hdeemDuration;
            }

            avgHdeemDuration += _buffer[i].hdeemDuration;

        }
        sumEnergy = avgEnergy;
        sumEnergyStatsStruct = avgEnergyStatsStruct;
        sumDuration = avgDuration;
        sumHdeemDuration = avgHdeemDuration;

        avgEnergy /= this->_size;
        avgEnergyStatsStruct /= this->_size;
        avgDuration /= this->_size;
        avgHdeemDuration /= this->_size;

    ss << "Average runtime of function [s] (clockTime)," << avgDuration << '\n'
       << "Maximal runtime of function [s] (clockTime)," << maxDuration << '\n'
       << "Minimal runtime of function [s] (clockTime)," << minDuration << '\n'
       << "Sum of runtime of function across all nodes [s] (clockTime)," << sumDuration << '\n'
       << "Average runtime of function [s] (hdeemTime)," << avgHdeemDuration << '\n'
       << "Maximal runtime of function [s] (hdeemTime)," << maxHdeemDuration << '\n'
       << "Minimal runtime of function [s] (hdeemTime)," << minHdeemDuration << '\n'
       << "Sum of runtime of function across all nodes [s] (hdeemTime)," << sumHdeemDuration << '\n';

    //////////////////
    // Blade values //
    //////////////////
    ss << "# Blade summary" << '\n'
       << "Average energy consumption [J] (Samples)," << avgEnergy << '\n'
       << "Maximal energy consumption [J] (Samples)," << maxEnergy << '\n'
       << "Minimal energy consumption [J] (Samples)," << minEnergy << '\n'
       << "Average energy consumption [J] (Stats structure)," << avgEnergyStatsStruct << '\n'
       << "Maximal energy consumption [J] (Stats structure)," << maxEnergyStatsStruct << '\n'
       << "Minimal energy consumption [J] (Stats structure)," << minEnergyStatsStruct << '\n';


    ///////////////////////
    // VR sensors values //
    ///////////////////////
    std::vector<std::string> vrSensor = {"CPU0", "CPU1", "DDR AB", "DDR CD", "DDR EF", "DDR GH"};
    unsigned int vrSensorNum = vrSensor.size();

    for (unsigned int sensorID = 1; sensorID < vrSensorNum; sensorID++) {
        ss << "# Voltage regulator (" << vrSensor[sensorID-1] << ") summary" << '\n';

        avgEnergy = 0;
        maxEnergy = 0;
        minEnergy = _buffer[0].energy[sensorID];

        avgEnergyStatsStruct = 0;
        maxEnergyStatsStruct = 0;
        minEnergyStatsStruct = _buffer[0].energyStatsStructure[sensorID];


        std::cout << minEnergy << std::endl;

        // Iterate through nodes
        for (size_t i = 0; i < this->_size; i++) {
            if(maxEnergy < _buffer[i].energy[sensorID]) {
                maxEnergy = _buffer[i].energy[sensorID];
            } else if (minEnergy > _buffer[i].energy[sensorID]) {
                minEnergy = _buffer[i].energy[sensorID];
            }
            avgEnergy += _buffer[i].energy[sensorID];

            if(maxEnergyStatsStruct < _buffer[i].energyStatsStructure[sensorID]) {
                maxEnergyStatsStruct = _buffer[i].energyStatsStructure[sensorID];
            } else if (minEnergyStatsStruct > _buffer[i].energyStatsStructure[sensorID]) {
                minEnergyStatsStruct = _buffer[i].energyStatsStructure[sensorID];
            }

            avgEnergyStatsStruct += _buffer[i].energyStatsStructure[sensorID];

        }
        avgEnergy /= this->_size;
        avgEnergyStatsStruct /= this->_size;

        ss << "Average energy consumption [J] (Samples)," << avgEnergy << '\n'
           << "Maximal energy consumption [J] (Samples)," << maxEnergy << '\n'
           << "Minimal energy consumption [J] (Samples)," << minEnergy << '\n'
           << "Average energy consumption [J] (Stats structure)," << avgEnergyStatsStruct << '\n'
           << "Maximal energy consumption [J] (Stats structure)," << maxEnergyStatsStruct << '\n'
           << "Minimal energy consumption [J] (Stats structure)," << minEnergyStatsStruct << '\n';
    }

}

template <class TStore>
void DebugAggregateStore<TStore>::saveInterval(std::vector<size_t> &iStarts, size_t iEnd)
{
	DebugRegionStats stats;
	this->_store.fillRegionStats(stats, iStarts.back(), iEnd);

	MPI_Gather(&stats, sizeof(DebugRegionStats), MPI_BYTE, _buffer.data(), sizeof(DebugRegionStats), MPI_BYTE, 0, this->_comm);
}

}
