
#ifndef SRC_BASIS_STORE_H_
#define SRC_BASIS_STORE_H_

#include <vector>
#include <map>
#include <string>

namespace meric {

struct EnergyRecord {
	EnergyRecord(int region, size_t time, size_t energy)
	: time(time), energy(energy), region(region) {};

	size_t time;
	size_t energy;
	int region;
};

struct DetailedEnergyRecord {
	DetailedEnergyRecord(int region, size_t time)
	: time(time), region(region) {};

	size_t time;
	size_t energy[7];
	int region;
};

struct TimeRecord {
	TimeRecord(int region, size_t time, size_t energy)
	: time(time), region(region) {};

	size_t time;
	int region;
};


class Store
{
public:
	void addStart(const std::string &region);
	void addEnd();

private:
	std::map<std::string, int> _regions;
	std::vector<EnergyRecord> _records;
	std::vector<int> _stack;
};


}

#endif /* SRC_BASIS_STORE_H_ */
