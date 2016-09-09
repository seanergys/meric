
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
#include "../basis/utils.h"

namespace meric {

class Store
{
public:
	virtual void addStart(const std::string &region) = 0;
	virtual void addEnd() = 0;

	virtual void save() = 0;

	virtual ~Store() {};
protected:
	virtual void saveInterval(std::ofstream &ss, std::vector<size_t> &iStarts, size_t end) = 0;

	template <class TRecord> void iterate(std::vector<TRecord> &records)
	{
		std::vector<std::ofstream> ss(_regions.size());
		std::vector<std::string> name(_regions.size());
		for (auto it = _regions.begin(); it != _regions.end(); ++it) {
			ss[it->second].open(it->first.c_str());
			name[it->second] = it->first;
		}

		std::vector<size_t> starts;
		for (size_t i = 0; i < records.size(); i++) {
			if (!starts.size() || records[starts.back()].region != records[i].region) {
				starts.push_back(i);
				continue;
			}

			saveInterval(ss[records[i].region], starts, i);

			starts.pop_back();
		}
	}

	int getRegionID(const std::string &region)
	{
		if (_regions.find(region) == _regions.end()) {
			size_t size = _regions.size();
			_regions[region] = size;
			_name.push_back(region);
		}
		return _regions[region];
	}

	std::map<std::string, int> _regions;
	std::vector<std::string> _name;
	std::vector<int> _stack;
};

}

#endif /* SRC_BASIS_STORE_H_ */
