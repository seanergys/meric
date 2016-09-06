
#ifndef SRC_BASIS_STORE_H_
#define SRC_BASIS_STORE_H_

#include <vector>
#include <map>
#include <string>

namespace meric {

class Store
{
public:
	virtual void addStart(const std::string &region) = 0;
	virtual void addEnd() = 0;

	virtual ~Store() {};

protected:
	int getRegionID(const std::string &region);

	std::map<std::string, int> _regions;
	std::vector<int> _stack;
};


}

#endif /* SRC_BASIS_STORE_H_ */
