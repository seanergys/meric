
#ifndef SRC_MERIC_SHARED_SHAREDMERIC_H_
#define SRC_MERIC_SHARED_SHAREDMERIC_H_

#include "../meric.h"

#include "../../wrapper/environmentwrapper.h"

namespace meric {

class SharedMeric: public Meric {

public:
	
	void start(const std::string &region)
	{

		MERIC_INFO << "region '" << region << "' starting\n";
#ifdef HAVE_OMP_H
		if (omp_in_parallel())
		{
			#pragma omp barrier
			if (Meric::IGNORE)
				return;

			if (ignoreRegion(region))
			{
				#pragma omp master
				_stack.push_back(getRegionID(region));
			}
			else
			{
//				#pragma omp barrier
				#pragma omp master
				{
					startRegion(region);
				}
			}
		}
		else
#endif
		{
			if (Meric::IGNORE)
				return;
			if (ignoreRegion(region))
					_stack.push_back(getRegionID(region));
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
			#pragma omp barrier
			if (Meric::IGNORE)
				return;

			if (ignoreRegion(getRegionName(_stack.size(), _stack.back())))
			{
				#pragma omp master
				_stack.pop_back();
			}
			else
			{
//				#pragma omp barrier
				#pragma omp master
				{
					stopRegion();
				}
			}
		}
		else
#endif
		{
			if (Meric::IGNORE)
				return;
			if (ignoreRegion(getRegionName(_stack.size(), _stack.back())))
					_stack.pop_back();
			else
				stopRegion();
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
		if (MODE == MODE_HDEEM || MODE == MODE_BOTH)
		{
			HDEEM::stop(_stopTime);
		}
		Meric::close();
//		Environment::uncoreFrequency(meric::MAX_UNCORE_FREQ);
//		Environment::frequency(meric::MAX_FREQ);
		Environment::setDefaultFrequencies();

		if (_stack.size() != 0)
		{
			std::cerr << "MERIC ERROR: not all regions were stopped before the MERIC_Close()\n";
			exit(-1);
		}
		
		int initCounter = -1;
		for(int i=0; i<_storeList.size(); i++)
		{
			initCounter = _storeList[i]->initStore(initCounter, Meric::OUTPUT_DIR, Meric::OUTPUT_FILENAME);
			try
			{
				_storeList[i]->save(Meric::OUTPUT_DIR, Meric::OUTPUT_FILENAME, _name);
			}
			catch (int &e)
			{
				//exception for hdeem store only, that is always first store in the list
				_storeList[i]->restoreCounter(initCounter, Meric::OUTPUT_DIR, Meric::OUTPUT_FILENAME);
				break;
			}
		}
	}

	SharedMeric();
	~SharedMeric();
private:
	void startRegion(const std::string &region)
	{
		SocketsRegionParameters change = regionValue(currentState, region);
		for (int socket=0; socket < NUMOFSOCKETS; socket++)
		{
			if(change.socket[socket].uncoreFrequency)
				Environment::uncoreFrequency(currentState.socket[socket].uncoreFrequency, socket);
			if(change.socket[socket].frequency)
				Environment::frequency(currentState.socket[socket].frequency, socket);
		}
		if(change.socket[0].nthreads)
			Environment::threadNumber(currentState.socket[0].nthreads);

		Meric::start(region);
		_stack.push_back(getRegionID(region));
	}
	
	void stopRegion()
	{
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
		
		SocketsRegionParameters change = regionValue(currentState, getRegionName(_stack.size(), _stack.back()));
		for (int socket=0; socket < NUMOFSOCKETS; socket++)
		{
			if(change.socket[socket].uncoreFrequency)
				Environment::uncoreFrequency(currentState.socket[socket].uncoreFrequency, socket);
			if(change.socket[socket].frequency)
				Environment::frequency(currentState.socket[socket].frequency, socket);
		}
		if(change.socket[0].nthreads)
			Environment::threadNumber(currentState.socket[0].nthreads);
	}
};

}

#endif /* SRC_MERIC_SHARED_SHAREDMERIC_H_ */
