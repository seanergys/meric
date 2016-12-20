
#ifndef SRC_BASIS_COUNTERSSTORE_H_
#define SRC_BASIS_COUNTERSSTORE_H_

#include "store.h"

namespace meric {

struct CountersRecord {
	CountersRecord(int region, std::map <std::string, long> record, timespec time)
	: region(region), record(record), time(time){};
	
	int region;
	std::map <std::string, long> record;
	timespec time;
};

enum WRAPPERS {PERFEVENT, PAPI, RAPL};

template <class TStore>
class CountersAggregateStore;

class CountersStore: public Store {

	friend class CountersAggregateStore<CountersStore>;
public:
	void addStart(int region);
	void addEnd(int region);
	void fillRegionStats(std::vector<long> &stats, double *time, std::vector<size_t> &iStarts, size_t iEnd);

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

//	CountersStore(const std::string wrapper, bool write = true) : Store(write)
	CountersStore(const std::string wrapper)
	{
		if (wrapper == "perfevent")
			wrapperType = PERFEVENT;
		else if (wrapper == "papi")
			wrapperType = PAPI;
		else if (wrapper == "rapl")
			wrapperType = RAPL;

		MERIC_INFO << "Create CountersStore - " << wrapper << std::endl;
	}
	CountersStore(const std::string wrapper, bool write) : Store(write)
	{
		if (wrapper == "perfevent")
			wrapperType = PERFEVENT;
		else if (wrapper == "papi")
			wrapperType = PAPI;
		else if (wrapper == "rapl")
			wrapperType = RAPL;

		MERIC_INFO << "Create CountersStore - " << wrapper << std::endl;
	}
	
//	CountersStore(bool write) : Store(write) {wrapperType = RAPL;};
//	CountersStore() {wrapperType = RAPL;};
//	~CountersStore() {};

protected:
	int wrapperType;

	void saveInterval(std::ofstream &ss, std::vector<size_t> &iStarts, size_t iEnd);

	double timespec2double(timespec time) {return time.tv_sec + (double)time.tv_nsec/1e9;};

	std::vector<CountersRecord> _records;
};

}


#endif /* SRC_BASIS_COUNTERSSTORE_H_ */
