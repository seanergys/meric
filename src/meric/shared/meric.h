
#ifndef SRC_MERIC_SHARED_SHAREDMERIC_H_
#define SRC_MERIC_SHARED_SHAREDMERIC_H_

#include "../meric.h"


namespace meric {

class SharedMeric: public Meric {

public:
	
	void start(const std::string &region)
	{
		MERIC_INFO << "region '" << region << "' starting\n";
		timespec spectime1, spectime2;
		if (OMPbarriers)
		{
			#pragma omp barrier
		}
		#pragma omp master
		{
			clock_gettime(CLOCK_REALTIME, &spectime1);
			SocketsRegionParameters change = regionValue(currentState, region);
			setRegionEnvironment(change);
			clock_gettime(CLOCK_REALTIME, &spectime2);
			MERIC_INFO << "MERIC settings changed in " << mutils::timeToDouble(spectime2 - spectime1)*1000 << " ms\n";
		}
		if (Meric::IGNORE)
			return;
		#pragma omp master
		{
			if (!ignoreRegion(region))
				Meric::start(region);
			_stack.push_back(getRegionID(region));
			clock_gettime(CLOCK_REALTIME, &spectime1);
			MERIC_INFO << "MERIC status recorded in " << mutils::timeToDouble(spectime1 - spectime2)*1000 << " ms\n";
		}
		if (OMPbarriers)
		{
			#pragma omp barrier
		}
	}
	
	void stop()
	{
		MERIC_INFO << "region '" << getRegionName(_stack.size(), _stack.back()) << "' stopping\n";
		if (OMPbarriers)
		{
			#pragma omp barrier
		}
		#pragma omp master
		{
			SocketsRegionParameters change = regionValue(currentState, getRegionName(_stack.size()-1, _stack.back()-1));
			setRegionEnvironment(change);
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
		if (OMPbarriers)
		{
			#pragma omp barrier
		}
	}
	
	void stopStart(const std::string &region)
	{
		MERIC_INFO << "region '" << getRegionName(_stack.size(), _stack.back()) << "' stopping and staring region'"  << region << "'\n";
		if (OMPbarriers)
		{
			#pragma omp barrier
		}
		#pragma omp master
		{
			SocketsRegionParameters change = regionValue(currentState, region);
			setRegionEnvironment(change);
		}
		
		if (Meric::IGNORE)
			return;
		if (!_stack.size())
		{
			#pragma omp master
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
		if (OMPbarriers)
		{
			#pragma omp barrier
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

};

}

#endif /* SRC_MERIC_SHARED_SHAREDMERIC_H_ */
