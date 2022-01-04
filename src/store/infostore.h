
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

struct GeneralInfoRecord {
	GeneralInfoRecord(unsigned int id = 0, std::string time = "", std::string sys = "", std::string format = "", std::string note = "") :
		ExecutionID(id), Timestamp(time), System(sys), DataFormat(format), Note(note) {};
	
	unsigned int ExecutionID;
	std::string Timestamp;
	std::string System;
	std::string DataFormat;
	std::string Note;
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

	void initStore(	unsigned int & counter,
					const std::string outputDir,
					const std::string outputFilename,
					const RegionParameters &outputFormat,
					const std::string outputFilenameInfo, //node_CF_UnCF_config
					const std::string prefix = "") override
	{
		loadSchedulerEnvironment();
		outputFilenameFormat = outputFormat; // TODO

		mutils::createDir(outputDir);
		dataInfoFileName = outputDir + "/measurementInfo.json";
		
		char hostname[HOST_NAME_MAX];
		gethostname(hostname, HOST_NAME_MAX);
		time_t now_t = time(0);
		std::string now_s = std::string(ctime(&now_t));
		now_s.pop_back();

		struct stat info;
		if(stat(dataInfoFileName.c_str(), &info ) != 0)
		{
			measurementInfo = {0, now_s, std::string(hostname), (prefix.size() ? "node_" : "") + outputFilenameInfo, ""};
		}
		else
		{
			//load input file
			std::string line, jsonString = "";
			std::ifstream jsonFile (dataInfoFileName);

			if (jsonFile.is_open())
			{
				while (getline(jsonFile,line))
				{
					jsonString += " " + line;
				}
				jsonFile.close();
			}
			const char * json = jsonString.c_str();
			
			struct json_value_s* root = json_parse(json, strlen(json));
			if (root ==  NULL)
			{
				std::cerr << "MERIC WARNING: empty or invalid input "<< dataInfoFileName <<"\n";
				measurementInfo = {0, now_s, std::string(hostname), (prefix.size() ? "node_" : "") + outputFilenameInfo, ""};
			}

			struct json_object_element_s* data = ((struct json_object_s*)root->payload)->start;
			if (data == NULL)
			{
				free(root);
				measurementInfo = {0, now_s, std::string(hostname), (prefix.size() ? "node_" : "") + outputFilenameInfo, ""};
			}
			else
			{
				do
				{
					struct json_object_element_s* value;
					if(std::string(data->name->string) == "Execution ID")
					{
						measurementInfo.ExecutionID = std::stoul(std::string(((struct json_string_s*)data->value->payload)->string)) + 1;
					}
					else if(std::string(data->name->string) == "Timestamp")
					{
						measurementInfo.Timestamp = std::string(((struct json_string_s*)data->value->payload)->string);
					}
					else if(std::string(data->name->string) == "System")
					{
						measurementInfo.System = std::string(((struct json_string_s*)data->value->payload)->string);
					}
					else if(std::string(data->name->string) == "DataFormat")
					{
						measurementInfo.DataFormat = std::string(((struct json_string_s*)data->value->payload)->string);
					}
					else if(std::string(data->name->string) == "Note")
					{
						measurementInfo.Note = std::string(((struct json_string_s*)data->value->payload)->string);
					}
					else
					{
						std::cerr << "MERIC WARNING: unknown json input \'" << std::string(data->name->string) << "\' in " << dataInfoFileName << '\n';
					}
					data = data->next;
				}while(data != NULL);
			}
		}
		
		_initCounter = measurementInfo.ExecutionID;
		counter = _initCounter;
	}

	void storeMeasurementInfo()
	{
		std::ofstream oFile(dataInfoFileName);
		oFile << "{\n"
			  << "\t\"Execution ID\": \"" << measurementInfo.ExecutionID << "\",\n"
			  << "\t\"Timestamp\"   : \"" << measurementInfo.Timestamp << "\",\n"
			  << "\t\"System\"      : \"" << measurementInfo.System << "\",\n"
			  << "\t\"DataFormat\"  : \"" << measurementInfo.DataFormat << "\",\n"
			  << "\t\"Note\"        : \"" << measurementInfo.Note << "\"\n"
			  << "}\n";
		oFile.close();
	}

protected:
	void saveInterval(std::ofstream &ss, std::vector<size_t> &iStarts, size_t iEnd) override;

	std::vector<InfoRecord> _records;
	std::vector<timespec> _timeStack;
	std::string dataInfoFileName;
	GeneralInfoRecord measurementInfo;
};

}


#endif /* SRC_STORE_INFOSTORE_H_ */
