
#ifndef SRC_STORE_COUNTERSSTORE_H_
#define SRC_STORE_COUNTERSSTORE_H_

#include <iomanip>

#include "store.h"

namespace meric {

template <typename TType>
struct CountersRecord {
	CountersRecord(int region, std::map <std::string, TType> record, timespec time)
	: region(region), record(record), time(time){};
	CountersRecord() : region(-1){};
	
	int region;
	std::map <std::string, TType> record;
	timespec time;
};

template <class TStore>
class CountersAggregateStore;

template <class TCounter, typename TType> class CountersStore: public Store {

	friend class CountersAggregateStore<CountersStore>;
public:
	typedef TType dataType;
	typedef TCounter counterType;
	void addStart(int region);
	void addEnd(int region);
	void fillRegionStats(std::vector<TType> &stats, double *time, size_t iStarts, size_t iEnd);

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
	std::vector<CountersRecord<TType>> _records;
};

}


#endif /* SRC_STORE_COUNTERSSTORE_H_ */
