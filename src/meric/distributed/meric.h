
#ifndef SRC_MERIC_DISTRIBUTED_DISTRIBUTEDMERIC_H_
#define SRC_MERIC_DISTRIBUTED_DISTRIBUTEDMERIC_H_

#include <mpi.h>

#include "../meric.h"

#include "../../store/aggregated/aggregatestore.h"
#include "../../store/aggregated/detailedaggregatestore.h"
#include "../../store/aggregated/debugaggregatestore.h"
#include "../../store/aggregated/countersaggregatestore.h"

namespace meric {

class DistributedMeric: public Meric {

public:
	DistributedMeric();
	~DistributedMeric();

	void start(const std::string &region)
	{
		MERIC_INFO << "region '" << region << "' starting\n";

		#pragma omp master
		{
			SocketsRegionParameters change = regionValue(currentState, region, _nodeId);
			if (MPIbarriers)
				MPI_Barrier(_currentNode);
			setRegionEnvironment(change, !_MPInodeRank);
		}
		if (Meric::IGNORE)
			return;
		#pragma omp master
		{
			if (!ignoreRegion(region))
				Meric::start(region);
			_stack.push_back(getRegionID(region));
		}
	}
	
	void stop()
	{
		MERIC_INFO << "region '" << getRegionName(_stack.size(), _stack.back()) << "' stopping\n";

		#pragma omp master
		{
			SocketsRegionParameters change = regionValue(currentState, getRegionName(_stack.size()-1, _stack.back()-1), _nodeId);
			if (MPIbarriers)
				MPI_Barrier(_currentNode);
			setRegionEnvironment(change, !_MPInodeRank);
		}
		if (Meric::IGNORE)
			return;
		#pragma omp master
		{
			if (!ignoreRegion(getRegionName(_stack.size(), _stack.back())))
			{
				if (!_stack.size())
				{
					std::cerr << "MERIC ERROR: Incorrect regions. There are more 'ends' than 'starts'!\n";
				}
				else
				{
					Meric::stop();
					_stack.pop_back();
				}
			}
			else
				_stack.pop_back();
		}
	}

	void stopStart(const std::string &region)
	{
		MERIC_INFO << "region '" << getRegionName(_stack.size(), _stack.back()) << "' stopping and staring region'"  << region << "'\n";

		#pragma omp master
		{
			SocketsRegionParameters change = regionValue(currentState, region, _nodeId);
			if (MPIbarriers)
				MPI_Barrier(_currentNode);
			setRegionEnvironment(change, !_MPInodeRank);
		}
		
		if (Meric::IGNORE)
			return;
		if (!_stack.size())
		{
			#pragma omp single
			std::cerr << "MERIC ERROR: Incorrect regions. There are more 'ends' than 'starts'!\n";
			return;
		}
		#pragma omp master
		{
			if (!ignoreRegion(getRegionName(_stack.size(), _stack.back())))
				Meric::stop();
			_stack.pop_back();

			if (!ignoreRegion(region))
				Meric::start(region);
			_stack.push_back(getRegionID(region));
		}
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
		if (MPIbarriers)
			MPI_Barrier(_currentNode);

		if ((Meric::MODE == MODE_HDEEM || Meric::MODE == MODE_BOTH) && (!_MPInodeRank))
		{
			HDEEM::stop(_stopTime);
		}

		Meric::close();
		
		if (!_MPInodeRank)
		{
			Environment::setDefaultFrequencies();
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
		if (MPIbarriers)
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
	MPI_Comm _supportComm;
	MPI_Comm _worldCopy;

private:

};

}

#endif /* SRC_MERIC_DISTRIBUTED_DISTRIBUTEDMERIC_H_ */
