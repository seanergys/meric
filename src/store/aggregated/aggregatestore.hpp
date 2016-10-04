
#include "aggregatestore.h"

namespace meric {

template <class TStore>
void AggregateStore<TStore>::saveInterval(std::ofstream &ss, std::vector<size_t> &iStarts, size_t iEnd)
{
	RegionStats stats;
	this->_store.fillRegionStats(stats, iStarts.back(), iEnd);

	MPI_Gather(&stats, sizeof(RegionStats), MPI_BYTE, _buffer.data(), sizeof(RegionStats), MPI_BYTE, 0, this->_comm);

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
               minEnergy = _buffer[0].energy;
        double avgDuration = 0,
               maxDuration = 0,
               minDuration = _buffer[0].duration;

        for (size_t i = 0; i < this->_size; i++) {
            if(maxEnergy < _buffer[i].energy) {
                maxEnergy = _buffer[i].energy;
            } else if (minEnergy > _buffer[i].energy) {
                minEnergy = _buffer[i].energy;
            }

            avgEnergy += _buffer[i].energy;

            if(maxDuration < _buffer[i].duration) {
                maxDuration = _buffer[i].duration;
            } else if (minDuration > _buffer[i].duration) {
                minDuration = _buffer[i].duration;
            }

            avgDuration += _buffer[i].duration;
        }
        avgEnergy /= this->_size;
        avgDuration /= this->_size;

    ss << "Average runtime of function [s]," << avgDuration << '\n'
       << "Maximal runtime of function [s]," << maxDuration << '\n'
       << "Minimal runtime of function [s]," << minDuration << '\n'

       << "# Blade summary" << '\n'
       << "Average energy consumption [J]," << avgEnergy << '\n'
       << "Maximal energy consumption [J]," << maxEnergy << '\n'
       << "Minimal energy consumption [J]," << minEnergy << '\n';
       
}

template <class TStore>
void AggregateStore<TStore>::saveInterval(std::vector<size_t> &iStarts, size_t iEnd)
{
	RegionStats stats;
	this->_store.fillRegionStats(stats, iStarts.back(), iEnd);

	MPI_Gather(&stats, sizeof(RegionStats), MPI_BYTE, _buffer.data(), sizeof(RegionStats), MPI_BYTE, 0, this->_comm);
}

}


