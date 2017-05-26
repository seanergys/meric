
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
		MERIC_INFO << "region '" << region << "' starting\n";
#ifdef HAVE_OMP_H
		if (omp_in_parallel())
		{
			if (ignoreRegion(region))
			{
				#pragma omp master
				if (!_MPInodeRank)
					_stack.push_back(getRegionID(region));
			}
			else
			{
				#pragma omp barrier
				#pragma omp master
				{
					startRegion(region);
				}
			}
		}
		else
#endif
		{
			if (ignoreRegion(region))
			{
				if (!_MPInodeRank)
					_stack.push_back(getRegionID(region));
			}
			else
				startRegion(region);
		}
	}
	
	void stop()
	{
		MERIC_INFO << "region '" << getRegionName(_stack.size(), _stack.back()) << "' stopping\n";
#ifdef HAVE_OMP_H
		if (omp_in_parallel())
		{
			if (ignoreRegion(getRegionName(_stack.size(), _stack.back())))
			{
				#pragma omp master
				if (!_MPInodeRank)
					_stack.pop_back();
			}
			else
			{		
				#pragma omp barrier
				#pragma omp master
				{
					stopRegion();
				}
			}
		}
		else
#endif
		{
			if (ignoreRegion(getRegionName(_stack.size(), _stack.back())))
			{
				if (!_MPInodeRank)
					_stack.pop_back();
			}
			else
				stopRegion();
		}
	}
	
	void save()
	{
		MPI_Barrier(_currentNode);

		if ((Meric::MODE == MODE_HDEEM || Meric::MODE == MODE_BOTH) && (!_MPInodeRank))
		{
			HDEEM::stop(_stopTime);
		}

		Meric::close();
		
		if (!_MPInodeRank)
		{
			Environment::uncoreFrequency(meric::MAX_UNCORE_FREQ);
			Environment::frequency(meric::MAX_FREQ);
		}
		
		if (_stack.size() != 0)
		{
			if (!_MPInodeRank)
			{
				std::cerr << "MERIC ERROR: not all regions were stopped before the MERIC_Close()\n";
				MPI_Abort(MPI_COMM_WORLD, -1);
			}
		}
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
			if (!_MPInodeRank)
			{
				initCounter = _storeList[i]->initStore(initCounter, Meric::OUTPUT_DIR, Meric::OUTPUT_FILENAME, _nodeName);
			}
			if (_storeList[i]->getMPIWrite())
			{
				try
				{
					_storeList[i]->save(Meric::OUTPUT_DIR, Meric::OUTPUT_FILENAME, _name, _nodeName);
				}
				catch (int &e)
				{
					//exception for hdeem store only, that is always first store in the list
					_storeList[i]->restoreCounter(initCounter, Meric::OUTPUT_DIR, Meric::OUTPUT_FILENAME);
					MPI_Abort(MPI_COMM_WORLD, 1);
				}
			}
		}
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
		SocketsRegionParameters change = regionValue(currentState, region, _nodeId);
		if (!_MPInodeRank)
		{
			for (int socket=0; socket < NUMOFSOCKETS; socket++)
			{
				if (change.socket[socket].frequency)
					Environment::frequency(currentState.socket[socket].frequency, socket);
				if (change.socket[socket].uncoreFrequency)
					Environment::uncoreFrequency(currentState.socket[socket].uncoreFrequency, socket);
			}
		}
		if (change.socket[0].nthreads)
			Environment::threadNumber(currentState.socket[0].nthreads);

		Meric::start(region);
		_stack.push_back(getRegionID(region));
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
			return;
		}
		
		SocketsRegionParameters change = regionValue(currentState, getRegionName(_stack.size(), _stack.back()), _nodeId);
		if (!_MPInodeRank) 
		{
			for (int socket=0; socket < NUMOFSOCKETS; socket++)
			{
				if (change.socket[socket].frequency)
					Environment::frequency(currentState.socket[socket].frequency, socket);
				if (change.socket[socket].uncoreFrequency)
					Environment::uncoreFrequency(currentState.socket[socket].uncoreFrequency, socket);
			}
		}
		if (change.socket[0].nthreads)
			Environment::threadNumber(currentState.socket[0].nthreads);
	}

};

}

#endif /* SRC_MERIC_DISTRIBUTED_DISTRIBUTEDMERIC_H_ */
