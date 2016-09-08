
#ifndef SRC_BASIS_STORE_H_
#define SRC_BASIS_STORE_H_

#include <vector>
#include <map>
#include <string>

#include <iostream>
#include <sstream>
#include <fstream>
#include <ctime>
#include <iomanip>

#include "../wrapper/hdeemwrapper.h"
#include "utils.h"

namespace meric {

class Store
{
public:
	virtual void addStart(const std::string &region) = 0;
	virtual void addEnd() = 0;

	virtual ~Store() {};
protected:
	int getRegionID(const std::string &region)
	{
		if (_regions.find(region) == _regions.end()) {
			size_t size = _regions.size();
			_regions[region] = size;
		}
		return _regions[region];
	}

	std::map<std::string, int> _regions;
	std::vector<int> _stack;
};

}

#endif /* SRC_BASIS_STORE_H_ */
