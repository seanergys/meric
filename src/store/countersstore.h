
#ifndef SRC_BASIS_COUNTERSSTORE_H_
#define SRC_BASIS_COUNTERSSTORE_H_

#include <iomanip>

#include "store.h"

namespace meric {

struct CountersRecord {
	CountersRecord(int region, std::map <std::string, long long> record, timespec time)
	: region(region), record(record), time(time){};
	CountersRecord() : region(-1){};
	
	int region;
	std::map <std::string, long long> record;
	timespec time;
};

template <class TStore>
class CountersAggregateStore;

template <class TCounter> class CountersStore: public Store {

	friend class CountersAggregateStore<CountersStore>;
public:
	void addStart(int region);
	void addEnd(int region);
	void fillRegionStats(std::vector<unsigned long long> &stats, double *time, std::vector<size_t> &iStarts, size_t iEnd);
	std::string getCountersLabel();
	void getRatios(CountersRecord start, CountersRecord stop, double runtime, std::map <std::string, double> & ratios);

	void save(const std::string outputDir, const std::string outputFilename, std::vector<std::string> regionNames, const std::string prefix = "")
	{
		_name = regionNames; 
        iterate(_records, prefix, outputDir, outputFilename); 
    }
    
	void save(const std::string prefix, const std::string outputDir, const std::string outputFilename,
              std::function<void(std::ofstream &ss, std::vector<size_t> &iStarts, size_t iEnd)> processInterval)
	{
		iterate(_records, prefix, outputDir, outputFilename, processInterval);
	}
	
	void save(std::function<void(std::vector<size_t> &iStarts, size_t iEnd)> processInterval)
	{
		iterate(_records, processInterval);
	}

//	CountersStore(bool write = true) : Store(write)
	CountersStore(bool samples) : Store(samples)
	{
		MERIC_INFO << "Create CountersStore - " << TCounter::getCountersLabel() << std::endl;
	}
	CountersStore(bool samples, bool write) : Store(samples, write)
	{
		MERIC_INFO << "Create CountersStore - " << TCounter::getCountersLabel() << std::endl;
	}
	
//	~CountersStore() {};

protected:
	void saveInterval(std::ofstream &ss, std::vector<size_t> &iStarts, size_t iEnd);
	double timespec2double(timespec time) {return time.tv_sec + (double)time.tv_nsec/1e9;};
	std::vector<CountersRecord> _records;
};

}


#endif /* SRC_BASIS_COUNTERSSTORE_H_ */
