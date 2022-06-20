
#ifndef SRC_MERIC_DISTRIBUTED_DISTRIBUTEDMERIC_H_
#define SRC_MERIC_DISTRIBUTED_DISTRIBUTEDMERIC_H_

#include <mpi.h>

#include "../meric.h"

#include "../../store/aggregated/samplesaggregatestore.h"
#include "../../store/aggregated/debugaggregatestore.h"
#include "../../store/aggregated/countersaggregatestore.h"
#include "../../store/aggregated/infoaggregatestore.h"

namespace meric {

class DistributedMeric: public Meric {

public:
	DistributedMeric();
	~DistributedMeric();

	void start(const std::string &region)
	{

		if (Meric::IGNORE)
			return;
		if (ignoreRegion(region))
		{
			#pragma omp master
			{
				_stack.push_back(getRegionID(region));
				MERIC_INFO << "region '" << region << "' IGNORE start"<< std::endl;
			}
			return;
		}

		if (OMPbarriers)
		{
			#pragma omp barrier
		}
		#pragma omp single nowait //does not work with _stack, so it does not have to be master thread
		{
			if (_storeList.size() && _storeList[0]->getMPIWrite())
			{
				MERIC_INFO << "region '" << region << "' starting"<< std::endl;
			}
			SocketsRegionParameters change = regionValue(currentState, region, _nodeId);
			if (MPIbarriers)
				MPI_Barrier(_currentNode);
			setRegionEnvironment(change, !_MPInodeRank);
		}
		#pragma omp master
		{
			Meric::start(region);
			_stack.push_back(getRegionID(region));
		}
		if (OMPbarriers)
		{
			#pragma omp barrier
		}
	}
	
	double stop(const std::string region = "")
	{
		double regionRuntime = 0.0;
		if (Meric::IGNORE)
			return 0.0;
		std::string regionName = getRegionName(_stack.size(), _stack.back());
		if (region != "" && region != regionName)
		{
			#pragma omp single nowait
				std::cerr << "MERIC WARNING: probably corrupted instrumentation\n";
		}
		if (ignoreRegion(regionName))
		{
			#pragma omp master
			{
				MERIC_INFO << "region '" + regionName + "' IGNORE stop" << std::endl;
				_stack.pop_back();
			}
			return 0.0;
		}

		if (OMPbarriers)
		{
			#pragma omp barrier
		}
		#pragma omp master
		{
			if (_storeList.size() && _storeList[0]->getMPIWrite())
			{
				MERIC_INFO << "region '" << regionName << "' stopping"<< std::endl;
			}
			SocketsRegionParameters change = regionValue(currentState, getRegionName(_stack.size()-1, _stack.back()-1), _nodeId);
			if (MPIbarriers)
				MPI_Barrier(_currentNode);
			setRegionEnvironment(change, !_MPInodeRank);
		}
		#pragma omp master
		{
			if (!_stack.size())
			{
				std::cerr << "MERIC ERROR: Incorrect regions. There are more 'ends' than 'starts'!\n";
				regionRuntime = -1.0;
			}
			else
			{
				regionRuntime = Meric::stop(regionName+" - "+std::to_string(_MPInodeRank));
				_stack.pop_back();
			}
		}
		if (OMPbarriers)
		{
			#pragma omp barrier
		}

		return regionRuntime;
	}

	double stopStart(const std::string &startRegionName)
	{
		if (Meric::IGNORE)
			return 0.0;
		
		double regionRuntime = 0.0;
		const std::string stopRegionName = getRegionName(_stack.size(), _stack.back());
		if (OMPbarriers)
		{
			#pragma omp barrier
		}
		#pragma omp master
		{
			if (!ignoreRegion(startRegionName))
			{
				if (!ignoreRegion(stopRegionName))
					MERIC_INFO << "region '" << stopRegionName << "' stopping and starting region '"  << startRegionName << "'\n";
				else
					MERIC_INFO << "starting region '"  << startRegionName << "'\n";
				SocketsRegionParameters change = regionValue(currentState, startRegionName, _nodeId);
				if (MPIbarriers)
					MPI_Barrier(_currentNode);
				setRegionEnvironment(change, !_MPInodeRank);
			}
			else if (!ignoreRegion(stopRegionName))
			{
				MERIC_INFO << "region '" << stopRegionName << "' stopping\n";
				SocketsRegionParameters change = regionValue(currentState, getRegionName(_stack.size()-1, _stack.back()-1), _nodeId);
				if (MPIbarriers)
					MPI_Barrier(_currentNode);
				setRegionEnvironment(change, !_MPInodeRank);
			}
		}
		
		if (!_stack.size())
		{
			#pragma omp single nowait
			std::cerr << "MERIC ERROR: Incorrect regions. There are more 'ends' than 'starts'!\n";
			return -1.0;
		}
		#pragma omp master
		{
			if (!ignoreRegion(stopRegionName))
				regionRuntime = Meric::stop();
			_stack.pop_back();

			if (!ignoreRegion(startRegionName))
				Meric::start(startRegionName);
			_stack.push_back(getRegionID(startRegionName));
		}
		if (OMPbarriers)
		{
			#pragma omp barrier
		}
		return regionRuntime;
	}

	void startIgnore()
	{
		Meric::startIgnore();
	}
	
	void stopIgnore()
	{
		Meric::stopIgnore();
	}	

	void save()
	{
		stop(); //applicationMainRegion
		
		if (MPIbarriers)
			MPI_Barrier(_currentNode);

		if (!_MPInodeRank)
		{
			if (std::find(MONITORS.begin(), MONITORS.end(), MODE_HDEEM_SAMPLES) != MONITORS.end() || std::find(MONITORS.begin(), MONITORS.end(), MODE_HDEEM_COUNTERS) != MONITORS.end())
			{
				HDEEM::stop(_stopTime);
			}
			else if (std::find(MONITORS.begin(), MONITORS.end(), MODE_DAVIDE) != MONITORS.end())
			{
				DAVIDE::stop(_stopTime);
			}
		}
	
		Meric::close();
		
		if (!_MPInodeRank)
		{
			Environment::restoreSystemSettings();
		}
		
		if (_stack.size() != 0)
		{
			if (!_MPInodeRank)
			{
				std::cerr << "MERIC ERROR: not all regions were stopped before the MERIC_Close()\n";
				MPI_Abort(MPI_COMM_WORLD, -1);
			}
		}

		std::string nodeNamePrefix;
		if(Meric::AGGREGATE)
			nodeNamePrefix = "";
		else
			nodeNamePrefix = _nodeName;
		
		//remove ignored regions from the list of regions
		for(const std::string reg : ignoreRegions)
		{
			size_t pos = find(_name.begin(), _name.end(), reg) - _name.begin();
			if (pos < _name.size())
				_name[pos] = "";
		}
		
		unsigned int executionID = 0;
		for(unsigned int i=0; i<_storeList.size(); i++)
		{
			if (!_MPInodeRank)
			{
				if (!_MPIglobalRank)
					_storeList[i]->initStore(executionID, Meric::OUTPUT_DIR, Meric::OUTPUT_FILENAME, Meric::outputFormat, Meric::outputFilenameInfo, nodeNamePrefix);
				else
					_storeList[i]->initStore(executionID);
			}
			if (_storeList[i]->getMPIWrite())
			{
				try
				{
					_storeList[i]->save(Meric::OUTPUT_DIR, Meric::OUTPUT_FILENAME, _name, nodeNamePrefix);
				}
				catch (int &e)
				{
					//exception for timeenergystore(HDEEM), timestore<HDEEM>, timestore<DAVIDE> store when there are no power samples
					MPI_Abort(MPI_COMM_WORLD, 1);
				}
			}
		}
		
		// re-write the measurementInfo.json
		MPI_Barrier(MPI_COMM_WORLD);
		if (!_MPIglobalRank && _storeList.size())
			_storeList[0]->storeMeasurementInfo();
	}

protected:
	int _MPIglobalRank;
	int _MPIglobalSize;
	int _MPInodeRank;
	int _MPInodeSize;
	int _nodeId;
	std::string _nodeName;
	MPI_Comm _currentNode;
	MPI_Comm _storingProcs;
	MPI_Comm _supportComm;
	MPI_Comm _worldCopy;

private:

};

}

#endif /* SRC_MERIC_DISTRIBUTED_DISTRIBUTEDMERIC_H_ */
