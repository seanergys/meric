
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

namespace meric {

class Store
{
public:
	virtual void addStart(const std::string &region) = 0;
	virtual void addEnd() = 0;

	virtual ~Store() {};

protected:
	int getRegionID(const std::string &region);
	timespec subTimes(const timespec &a, const timespec &b);

	std::map<std::string, int> _regions;
	std::vector<int> _stack;
};

inline std::ostream operator<<(std::ostream& os, const timespec &time)
{
	os << time.tv_sec << "." << std::setfill('0') << std::setw(9) << time.tv_nsec;
}

}

#endif /* SRC_BASIS_STORE_H_ */
