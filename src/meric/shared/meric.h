
#ifndef SRC_MERIC_SHARED_SHAREDMERIC_H_
#define SRC_MERIC_SHARED_SHAREDMERIC_H_

#include "../meric.h"


namespace meric {

class SharedMeric: public Meric {

public:
	
	void start(const std::string &region)
	{
		timespec spectime1, spectime2;
		if (OMPbarriers)
		{
			#pragma omp barrier
		}
		
		#pragma omp single nowait
		{
			MERIC_INFO << "region '" << region << "' starting\n";
			clock_gettime(CLOCK_REALTIME, &spectime1);
			SocketsRegionParameters change = regionValue(currentState, region);
			setRegionEnvironment(change);
			clock_gettime(CLOCK_REALTIME, &spectime2);
			MERIC_INFO << "MERIC settings changed in " << mutils::timeToDouble(spectime2 - spectime1)*1000 << " ms\n";
		}
		if (Meric::IGNORE)
			return;
		#pragma omp single nowait
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
	
	double stop()
	{
		double regionRuntime = 0.0;
		if (OMPbarriers)
		{
			#pragma omp barrier
		}

		#pragma omp single nowait
		{
			MERIC_INFO << "region '" << getRegionName(_stack.size(), _stack.back()) << "' stopping\n";
			SocketsRegionParameters change = regionValue(currentState, getRegionName(_stack.size()-1, _stack.back()-1));
			setRegionEnvironment(change);
		}
		if (Meric::IGNORE)
			return 0.0;
		#pragma omp single nowait
		{
			if (!ignoreRegion(getRegionName(_stack.size(), _stack.back())))
			{
				if (!_stack.size())
				{
					std::cerr << "MERIC ERROR: Incorrect regions. There are more 'ends' than 'starts'!\n";
					regionRuntime = -1.0;
				}
				else
				{
					regionRuntime = Meric::stop();
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
		return regionRuntime;
	}
	
	double stopStart(const std::string &region)
	{
		double regionRuntime = 0.0;
		if (OMPbarriers)
		{
			#pragma omp barrier
		}
		
		#pragma omp single nowait
		{
			MERIC_INFO << "region '" << getRegionName(_stack.size(), _stack.back()) << "' stopping and staring region'"  << region << "'\n";
			SocketsRegionParameters change = regionValue(currentState, region);
			setRegionEnvironment(change);
		}
		
		if (Meric::IGNORE)
			return 0.0;
		if (!_stack.size())
		{
			#pragma omp single nowait
			std::cerr << "MERIC ERROR: Incorrect regions. There are more 'ends' than 'starts'!\n";
			return -1.0;
		}
		#pragma omp single nowait
		{
			if (!ignoreRegion(getRegionName(_stack.size(), _stack.back())))
				regionRuntime = Meric::stop();
			_stack.pop_back();

			if (!ignoreRegion(region))
				Meric::start(region);
			_stack.push_back(getRegionID(region));
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
		
		if (std::find(MONITORS.begin(), MONITORS.end(), MODE_HDEEM_SAMPLES) != MONITORS.end() || std::find(MONITORS.begin(), MONITORS.end(), MODE_HDEEM_COUNTERS) != MONITORS.end())
		{
			HDEEM::stop(_stopTime);
		}
		else if (std::find(MONITORS.begin(), MONITORS.end(), MODE_DAVIDE) != MONITORS.end())
		{
			DAVIDE::stop(_stopTime);
		}

		Meric::close();
		Environment::restoreSystemSettings();

		if (_stack.size() != 0)
		{
			std::cerr << "MERIC ERROR: not all regions were stopped before the MERIC_Close()\n";
			exit(-1);
		}
		
		for(unsigned int i=0; i<_storeList.size(); i++)
		{
			_storeList[i]->initStore(Meric::ITERATION, Meric::OUTPUT_DIR, Meric::OUTPUT_FILENAME, Meric::outputFormat, Meric::outputFilenameInfo);
			_storeList[i]->save(Meric::OUTPUT_DIR, Meric::OUTPUT_FILENAME, _name);
		}
	}

	SharedMeric();
	~SharedMeric();
private:

};

}

#endif /* SRC_MERIC_SHARED_SHAREDMERIC_H_ */
