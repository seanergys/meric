
#ifndef SRC_MERIC_DISTRIBUTED_DISTRIBUTEDMERIC_H_
#define SRC_MERIC_DISTRIBUTED_DISTRIBUTEDMERIC_H_

#include "mpi.h"

#include "../meric.h"

#include "../../store/aggregated/aggregatestore.h"
#include "../../store/aggregated/detailedaggregatestore.h"
#include "../../store/aggregated/debugaggregatestore.h"
#include "../../store/aggregated/countersaggregatestore.h"

#include "../../wrapper/environmentwrapper.h"

namespace meric {

class DistributedMeric: public Meric {

public:
	DistributedMeric();
	~DistributedMeric();

	void start(const std::string &region)
	{
		if (omp_in_parallel())
		{
			#pragma omp barrier
			#pragma omp master
			{
				startRegion(region);
			}
		}
		else
		{
			startRegion(region);
		}
	}
	
	void stop()
	{
		if (omp_in_parallel())
		{
			#pragma omp barrier
			#pragma omp master
			{
				stopRegion();
			}
		}
		else
		{
			stopRegion();
		}
	}
	
	void save()
	{
		MPI_Barrier(_currentNode);
/*
		if (!_MPInodeRank) {
			if (Meric::AGGREGATE) {
				for(int i=0; i<_storeList.size(); i++)
					_storeList[i]->save(Meric::OUTPUT_DIR, Meric::OUTPUT_FILENAME, _name, _nodeName);
			} else {
				for(int i=0; i<_storeList.size(); i++)
					_storeList[i]->save(Meric::OUTPUT_DIR, Meric::OUTPUT_FILENAME, _name, _nodeName);
			}
		}
*/
		int initCounter = -1;
		for(int i=0; i<_storeList.size(); i++)
		{
			if (_storeList[i]->getMPIWrite())
			{
				initCounter = _storeList[i]->initStore(initCounter, Meric::OUTPUT_DIR, Meric::OUTPUT_FILENAME, _nodeName);

				_storeList[i]->save(Meric::OUTPUT_DIR, Meric::OUTPUT_FILENAME, _name, _nodeName);
			}
		}
		sleep(1);
		MPI_Barrier(_currentNode);
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
	MPI_Comm _worldCopy;

private:
	void startRegion (const std::string &region)
	{
		MPI_Barrier(_currentNode);
		if (!_MPInodeRank)
		{
			Environment::frequency(regionValue(region, _nodeId).frequency);
			Environment::uncoreFrequency(regionValue(region, _nodeId).uncoreFrequency);
		}
		Environment::threadNumber(regionValue(region, _nodeId).nthreads);
		Meric::start(region);
		_stack.push_back(getRegionID(region));
		return;
	}

	void stopRegion()
	{
		MPI_Barrier(_currentNode);
		if (_stack.size())
		{
			Meric::stop();
			_stack.pop_back();
		}
		else
		{
			std::cerr << "MERIC ERROR: Incorrect regions. There are more 'ends' than 'starts'!\n";
		}	
		if (!_MPInodeRank) 
		{
			Environment::frequency(regionValue(getRegionName(_stack.size(), _stack.back()), _nodeId).frequency);
			Environment::uncoreFrequency(regionValue(getRegionName(_stack.size(), _stack.back()), _nodeId).uncoreFrequency);
		}
		Environment::threadNumber(regionValue(getRegionName(_stack.size(), _stack.back()), _nodeId).nthreads);
		return;
	}

};

}

#endif /* SRC_MERIC_DISTRIBUTED_DISTRIBUTEDMERIC_H_ */
