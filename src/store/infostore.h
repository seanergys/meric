
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

class InfoStore: public Store {

	friend class InfoAggregateStore<InfoStore>;

public:
	void addStart(int region, RegionParameters configuration) override;
	void addEnd(int region) override;
	double getRegionDuration() override;
	void fillRegionStats(InfoStats &stats, size_t iStarts, size_t iEnd);

	void save(const std::string outputDir, const std::string outputFilename, std::vector<std::string> regionNames, const std::string prefix = "") override
	{
		_name = regionNames;
		iterate(_records, prefix, outputDir, outputFilename);
	}
    
	void save(const std::string prefix, const std::string outputDir, const std::string outputFilename,
              std::function<void(std::ofstream &ss, std::vector<size_t> &iStarts, size_t iEnd)> processInterval) override
	{
		iterate(_records, prefix, outputDir, outputFilename, processInterval);
	}

	void save(std::function<void(std::vector<size_t> &iStarts, size_t iEnd)> processInterval) override
	{
		iterate(_records, processInterval);
	}

	InfoStore(bool samples, bool write) : Store(false, write) // required for AggregateStoreBase constructor
	{
		MERIC_INFO << "Create InfoStore\n";
	}
	
	InfoStore(bool write = true) : Store(false, write)
	{
		MERIC_INFO << "Create InfoStore\n";
	}

	void initStore(	int counter) override
	{
		loadSchedulerEnvironment();
		_initCounter = counter;
	};
	
	void initStore(	int counter,
					const std::string outputDir,
					const std::string outputFilename,
					const std::string outputFilenameInfo, //node_CF_UnCF_config
					const std::string prefix = "") override
	{
		loadSchedulerEnvironment();
		_initCounter = counter;
		if(counter == 0)
		{
			mutils::createDir(outputDir);
			std::string dataInfoFileName = outputDir + "/measurementInfo.json";
			struct stat info;
			if(stat(dataInfoFileName.c_str(), &info ) != 0)
			{
				std::ofstream oFile(dataInfoFileName);

				char hostname[HOST_NAME_MAX];
				gethostname(hostname, HOST_NAME_MAX);
				time_t now_t = time(0);
	   			std::string now_s = std::string(ctime(&now_t));
	   			now_s.pop_back();
	   			
				oFile << "{\n" 
					  << "\t\"Timestamp\" : \"" << now_s << "\",\n"
					  << "\t\"System\"    : \"" << std::string(hostname) << "\",\n"
					  << "\t\"DataFormat\": \"" << (prefix.size() ? "node_" : "") << outputFilenameInfo << "\",\n"
					  << "\t\"Note\"      : \"\"\n"
					  << "}";
				oFile.close();
			}
		}
		//CALLTREE init counter
		return;
	};


protected:
	void saveInterval(std::ofstream &ss, std::vector<size_t> &iStarts, size_t iEnd) override;

	std::vector<InfoRecord> _records;
	std::vector<timespec> _timeStack;
};

}


#endif /* SRC_STORE_INFOSTORE_H_ */
