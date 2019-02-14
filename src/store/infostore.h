
#ifndef SRC_STORE_INFOSTORE_H_
#define SRC_STORE_INFOSTORE_H_

#include "store.h"

namespace meric {

struct InfoRecord {
	InfoRecord(int region, timespec &time, RegionParameters configuration)
	: time(time), region(region), configuration(configuration) {};

	timespec time;
	int region;
	RegionParameters configuration;
};


template <class TStore>
class InfoAggregateStore;

/*
template <class TStore>
class DetailedAggregateStore;
*/

class InfoStore: public Store {

//	friend class AggregateStore<InfoStore>;
	friend class InfoAggregateStore<InfoStore>;

public:
	void addStart(int region, RegionParameters configuration);
	void addEnd(int region);
	double getRegionDuration();
	void fillRegionStats(RegionStats &stats, size_t iStarts, size_t iEnd);

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
/*
	InfoStore(bool samples, bool write) : Store(samples, write)
	{
		MERIC_INFO << "Create InfoStore\n";
	}

	InfoStore(bool samples)
	{
		MERIC_INFO << "Create InfoStore\n";
	}
*/
/*
	InfoStore()
	{
		MERIC_INFO << "Create InfoStore\n";
	}
*/
	InfoStore(bool write = true) : Store(false, write)
	{
		MERIC_INFO << "Create InfoStore\n";
	}

protected:
	void saveInterval(std::ofstream &ss, std::vector<size_t> &iStarts, size_t iEnd);

	std::vector<InfoRecord> _records;
	std::vector<timespec> _timeStack;
};

}


#endif /* SRC_STORE_INFOSTORE_H_ */
