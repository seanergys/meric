
#ifndef SRC_MERIC_MERIC_H_
#define SRC_MERIC_MERIC_H_

#include <iostream>
#include <map>
#include <vector>
#include <algorithm>

#include "../basis/tokenizer.h"
#include "../basis/utils.h"
#include "../store/detailedenergystore.h"
#include "../store/energystore.h"
#include "../store/timestore.h"
#include "../store/detailedtimestore.h"
#include "../store/timeenergystore.h"
#include "../store/countersstore.h"

namespace meric {

enum mode {MODE_HDEEM, MODE_RAPL, MODE_BOTH};

struct RegionParameters {
	double frequency;
	uint64_t uncoreFrequency;
	int nthreads;
};

class Meric {

public:
	static void readRegionParameters();

	static bool CONTINUAL;
	static bool DETAILED;
	static bool AGGREGATE;
	static bool DEBUG;
	static int MODE;
	static std::string COUNTERS;
	static std::string OUTPUT_DIR;
	static std::string OUTPUT_FILENAME;

	virtual void start(const std::string &region)
	{
		MERIC_INFO << "region '" << region << "' starting\n";
		for (int i=0; i<_storeList.size(); i++)
			_storeList[i]->addStart(getRegionID(region));
		MERIC_INFO << "region '" << region << "' started\n";
	}

	virtual void stop()
	{
		MERIC_INFO << "region '" << getRegionName(_stack.size(), _stack.back()) << "' stopping\n";
		//stop should be in opposit direction
		//if MERIC_CONTINUAL is used, we want to start it first and stop it last
		for(int i=_storeList.size()-1;i>=0; i--)
			_storeList[i]->addEnd(_stack.back());
		MERIC_INFO << "region '" << getRegionName(_stack.size(), _stack.back()) << "' stopped\n";
	}

	virtual void save()
	{
		MERIC_INFO << "save stored data\n";
		for(int i=0; i<_storeList.size(); i++)
			_storeList[i]->save(Meric::OUTPUT_DIR, Meric::OUTPUT_FILENAME, _name);
		MERIC_INFO << "store data saved\n";
	}

	virtual ~Meric() {};

protected:
	const RegionParameters& regionValue(const std::string &region)
	{
		if (parameter.find(region) != parameter.end()) {
			return parameter[region];
		} else {
			return defaultParameter;
		}
	}
	
	const std::string& getRegionName(int size, int last)
	{
		return size ? _name[last] : _mainRegion;
	}
	
	int getRegionID(const std::string &region)
	{
		size_t pos = find(_name.begin(), _name.end(), region) - _name.begin();
		if (pos >= _name.size())
		{
			pos = _name.size();			
			_name.push_back(region);
		}
		return pos;
	}

	static std::map<std::string, RegionParameters> parameter;
	static RegionParameters defaultParameter;

	static std::vector<int> _stack;
	static std::vector<std::string> _name;
	std::vector<StoreBase *> _storeList;
	static std::string _mainRegion;
};

}

#endif /* SRC_MERIC_MERIC_H_ */
