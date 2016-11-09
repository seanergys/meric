
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

enum WRAPPERS {PERFEVENT, PAPI};


class CountersStore: public Store {


public:
	void addStart(const std::string &region);
	void addEnd();

	void save(const std::string outputDir, const std::string outputFilename, const std::string prefix = "") { 
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

	CountersStore(const std::string wrapper)
	{
		if (wrapper == "perfevent")
			wrapperType = PERFEVENT;
		else if (wrapper == "papi")
			wrapperType = PAPI;

		MERIC_INFO << "Create CountersStore - " << wrapper << std::endl;
	}

protected:
	int wrapperType;

	void saveInterval(std::ofstream &ss, std::vector<size_t> &iStarts, size_t iEnd);

	std::vector<CountersRecord> _records;
};

}


#endif /* SRC_BASIS_COUNTERSSTORE_H_ */
