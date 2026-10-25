#ifndef SRC_TOOLS_WRAPPER_EVALUATE_DATAANALYSIS_H_
#define SRC_TOOLS_WRAPPER_EVALUATE_DATAANALYSIS_H_

#include <dirent.h>

#include "../parameter/parameter.h"

//power monitoring systems
#include "../../../wrapper/counters.h"
#include "../../../wrapper/davidewrapper.h"
#include "../../../wrapper/e4wrapper.h"
#include "../../../wrapper/hdeemwrapper.h"
#include "../../../wrapper/jetsonwrapper.h"
#include "../../../wrapper/nvmlwrapper.h"
#include "../../../wrapper/occwrapper.h"
#include "../../../wrapper/raplwrapper.h"
#include "../../../wrapper/thunderwrapper.h"

class DataAnalysis {

public:
	static std::vector <std::string> regions; // otazka jestli tento seznam udrzovat tady
	static std::vector <std::string> dataFormat;
	static std::string measurementDirectory;
		
	/**
	 * reads list of regions of the application and recognise the energy measurement system
	 * requires at least the default run of the application
	 **/
	static void initDataAnalysis(cfg & config)
	{
		measurementDirectory = config.meric.outputdir.value;
		baseline = config.baseline.value;
		margin = config.margin.value;
		extention = config.extention.value;
		
		//read list of application's regions
		DIR *dir;
		struct dirent *ent;
		if ((dir = opendir(measurementDirectory.c_str())) != NULL)
		{
			while ((ent = readdir(dir)) != NULL)
			{
				if (ent->d_type == DT_DIR && ent->d_name[0] != '.')
				{
					std::string regionName = std::string(ent->d_name);
					if (regionName.size() > std::string("_static").size() && regionName.substr(regionName.size() - std::string("_static").size()) == "_static")
						regions.insert(regions.begin(), regionName); //regions[0] will allways be the static region
					else
						regions.push_back(regionName);
				}
			}
			closedir (dir);
		}
		
		if (regions.size() == 0)
		{
			std::cerr << "MERICwrapper ERROR: no data from the application default run\n";
			return;
		}
		
		//read measurementInfo.json for data format
		struct stat info;
		struct json_value_s* root = NULL;
		const std::string measurementInfoFile = measurementDirectory+"/measurementInfo.json";	//TODO MERIC store also number of app runs
		if(stat(measurementInfoFile.c_str(), &info) == 0)
		{
			std::ifstream jsonFile (measurementInfoFile);
			std::string line, jsonString = "";
			if (jsonFile.is_open())
			{
				while (getline(jsonFile,line))
				{
					jsonString += " " + line;
				}
				jsonFile.close();
			}
			struct json_value_s* root = json_parse(jsonString.c_str(), jsonString.size());
			if (root ==  NULL)
			{
				std::cerr << "MERICwrapper ERROR: invalid " << measurementInfoFile << "\n";
				return;
			}
			else
			{
				struct json_object_element_s* data = ((struct json_object_s*)root->payload)->start;
				while(data != NULL)
				{
					if (std::string(data->name->string) == "DataFormat")
					{
						std::string val = std::string(((struct json_string_s*)data->value->payload)->string);
						if (val.length() == 0)
						{
							std::cerr << "MERICwrapper ERROR: Empty \"DataFormat\" in measurementInfo.json, cannot evaluate the data\n";
							return;
						}
						size_t pos = 0;
						std::string token;
						while ((pos = val.find('_')) != std::string::npos)
						{
							dataFormat.push_back(val.substr(0, pos));
							val.erase(0, pos + 1);
						}
						dataFormat.push_back(val);
						break;
					}
					data = data->next;
				}
			}
		}
		else
		{
			std::cerr << "MERICwrapper ERROR: Directory " << measurementDirectory << " is not valid MERIC output\n";
			return;
		}
		
		//get list of hostnames
		if (dataFormat[0] == "node")
		{
			DIR *dir;
			struct dirent *ent;
			if ((dir = opendir((measurementDirectory+"/"+regions[0]).c_str())) != NULL)
			{
				while ((ent = readdir(dir)) != NULL)
				{
					if (ent->d_type == DT_REG)
					{
						std::string host = std::string(ent->d_name);
						host = host.substr(0,host.find_first_of("_"));
						if (std::find(hostnames.begin(), hostnames.end(), host) == hostnames.end())
							hostnames.push_back(host);
					}
				}
				closedir (dir);
			}
//			dataFormat.erase(dataFormat.begin()); //hostnames are not considered as a parameter
		}

		// detect MPI aggregate mode
		if (hostnames.size() == 0)
		{
			std::string inputFileName = measurementDirectory+"/"+regions[0];
			DIR *dir;
			struct dirent *ent;
			if ((dir = opendir(inputFileName.c_str())) != NULL)
			{
				while ((ent = readdir(dir)) != NULL)
				{
					std::string name = ent->d_name;
					if (ent->d_type == DT_REG && name.length() > 4 && name.substr(name.length()-4) == ".csv")
					{
						inputFileName += "/"+name;
						break;
					}
				}
				closedir (dir);
			}
			
			std::ifstream inputfile(inputFileName);
			if(inputfile.is_open())
			{
				std::string line;
				while (!inputfile.eof())
				{
					getline(inputfile,line);
					if (line.find(label_time) != std::string::npos)
					{ 
						line = line.substr(0,3);
						if (line == "AVG" || line == "MAX" || line == "MIN")
						{
							label_energy = "SUM " + label_energy;
							label_time = "AVG "+label_time;
						}
						break;
					}
				}
				inputfile.close();
			}
		}

		// recognise target energy measurement system
		if (config.meric.measure.value == meric::MODE_HDEEM_SAMPLES)
		{
			label_dataSource = std::string("# SAMPLES - ")+meric::HDEEM::getSourceLabel()+" (Blade):";
		}
		else if (config.meric.measure.value == meric::MODE_HDEEM_COUNTERS)
		{
			label_dataSource = std::string("# COUNTERS - ")+meric::HDEEM::getSourceLabel()+":";
		}
		else if (config.meric.measure.value == meric::MODE_RAPL)
		{
//		#ifdef _INTEL_	
//			label_energy += MSR_RAPL_COUNTERS::RAPL_SUM_ENERGY;
//		#elif defined _AMD_	//TODO AMD
//			label_energy += MSR_RAPL_COUNTERS::counterLabel.at(RAPL_PCKG_ENERGY);
//		#endif
			label_dataSource = std::string("# COUNTERS - ")+meric::RAPL::getSourceLabel()+":";
		}
		else if (config.meric.measure.value == meric::MODE_NVML)
		{
			label_dataSource = std::string("# COUNTERS - ")+meric::NVML::getSourceLabel()+":";
		}
		else if (config.meric.measure.value == meric::MODE_OCC)
		{
//			label_energy += OCC_COUNTERS::counterLabel.at("PWRSYS");
			label_dataSource = std::string("# COUNTERS - ")+meric::OCC::getSourceLabel()+":";
		}
		else if (config.meric.measure.value == meric::MODE_JETSON)
		{
			label_dataSource = std::string("# SAMPLES - ")+meric::JETSON::getSourceLabel()+" (Blade):";
		}
		else if (config.meric.measure.value == meric::MODE_THUNDER)
		{
			label_dataSource = std::string("# SAMPLES - ")+meric::THUNDER::getSourceLabel()+" (Blade):";
		}
		else if (config.meric.measure.value == meric::MODE_DAVIDE)
		{
			label_dataSource = std::string("# SAMPLES - ")+meric::DAVIDE::getSourceLabel()+" (Blade):";
		}
		else if (config.meric.measure.value == meric::MODE_PDU)
		{
			label_dataSource = std::string("# COUNTERS - ") + meric::E4::getSourceLabel() + ":";
		}
		std::cout << "LABEL = '"<< label_energy << "' SRC = '" << label_dataSource << "'\n";
	}
	
	static void readConsumption(regionConfiguration & conf, std::string fileName)
	{
//std::cout << fileName << std::endl;
		
		auto getEnergyConsumption = [&] (std::ifstream & inputfile, std::string & line)
		{
			size_t position = line.find(label_dataSource);
			if (position == std::string::npos)
				return false;

			do{
				getline(inputfile,line);
				position = line.find(label_energy);
			}while (!inputfile.eof() && position == std::string::npos);

			if (position != std::string::npos)
			{
				conf.consumption.energy.push_back(std::stod(line.substr(label_energy.size()+1+position)));
//std::cout << "\tE: " << conf.consumption.energy.back() << std::endl;
			}
			return true;
		};
		
		auto getTimeConsumption = [&] (std::ifstream & inputfile, std::string & line)
		{
			const std::string label_timeStore = "# Job info"; // infostore
			size_t position = line.find(label_timeStore);
			if (position == std::string::npos)
				return false;

			do{
				getline(inputfile,line);
				position = line.find(label_time);
			}while (!inputfile.eof() && position == std::string::npos);

			if (position != std::string::npos)
			{
				conf.consumption.runtime.push_back(std::stod(line.substr(label_time.size()+1+position)));
//std::cout << "\tT: " << conf.consumption.runtime.back() << std::endl;
			}
			return true;
		};
		
		auto getCallPath = [&] (std::ifstream & inputfile, std::string & line)
		{
			const std::string label_callpath = "# CALLTREE;";
			size_t position = line.find(label_callpath);
			if (position == std::string::npos)
				return false;
			conf.consumption.callpath.push_back(line.substr(label_callpath.size()));
//std::cout << "\tC: " << conf.consumption.callpath.back() << std::endl;
			return true;
		};
		
		
		//read the file
		std::ifstream inputfile (fileName);
		if(inputfile.is_open())
		{
			std::string line;
			while (!inputfile.eof())
			{
				getline(inputfile,line);
				if (line.front() != '#')
					continue;
				getEnergyConsumption(inputfile, line) || getTimeConsumption(inputfile, line) || getCallPath(inputfile, line);
			}
			inputfile.close();
		}
		
		// call-path currently not only part of the infostore - TODO should be removed from MERIC in a future version
		conf.consumption.callpath.resize(conf.consumption.runtime.size());
		
		//TODO vytvoreni call-path a odecteni vnorenych regionu
		
		//add static power baseline
		for (int i=0; i<conf.consumption.energy.size(); i++)
			conf.consumption.energy[i] += conf.consumption.runtime[i] * baseline;
	}
	
	
	/**
	 * read runtime and energy consumtion of the regions in the default execution
	 * 
	 **/
	static void loadDefaultRunConsumption()
	{
		if (defaultConfiguration.size())	//TODO why??
			return;
		
		std::string defaultRunFileName = "default.csv";
		std::string nodePrefix = "";
		if (hostnames.size() != 0)
			nodePrefix = hostnames[0]+"_";
		
		if (access((measurementDirectory+"/"+regions[0]+"/"+nodePrefix+defaultRunFileName).c_str(), F_OK ) != -1 )
		{
			defaultRunFileName = "0";
			for (int i=1; i < dataFormat.size(); i++)
				defaultRunFileName += "_0";
			defaultRunFileName += ".csv";
			if (access((measurementDirectory+"/"+regions[0]+"/"+nodePrefix+defaultRunFileName).c_str(), F_OK ) != -1 )
			{
				std::cerr << "MERICwrapper warning: no default execution\n";
				return;
			}
		}
		
		defaultConfiguration.resize(regions.size());
		
		for (int regionID = 0; regionID < regions.size(); regionID++)
		{
			regionConfiguration conf(regions[regionID]);
			readConsumption(conf, measurementDirectory+"/"+regions[regionID]+"/"+nodePrefix+defaultRunFileName);
			
			defaultConfiguration[regionID] = conf;
		}
		
		for (int i=1; i<hostnames.size(); i++)
		{
			nodePrefix = hostnames[i]+"_";
			for (int regionID = 0; regionID < regions.size(); regionID++)
			{
				regionConfiguration conf;
				readConsumption(conf, measurementDirectory+"/"+regions[regionID]+"/"+nodePrefix+defaultRunFileName);
				defaultConfiguration[regionID].consumption += conf.consumption;
			}
		}
	}
	
	
	
	/**
	 * Read resources consumption of the configurations from the vector finished
	 * and identify the optimal configuration for each region
	 * return number of improvements of the optimal configuration, 
	 **/
	static int analyse(std::vector<appConfiguration> & finished)
	{
	
	
	
std::cout << "ANALYSE (" << finished.size() <<")\n";

		loadDefaultRunConsumption();
		int ImprovedSolution = 0;
		
		for (int x = 0; x<finished.size(); x++) //Application execution
		{
			for(int y = 0; y<finished[x].size(); y++) //Application region
			{
				std::string fileName = measurementDirectory + "/";
//				if (hostnames.size())	//TODO MPI non-aggregate applikace
//				{
//					for (std::string host : hostnames)
//					{
//						readConsumption(finished[x][y], fileName + finished[x][y].constructConfigFileName(host));
//					}
//				}
//				else
				{
					readConsumption(finished[x][y], fileName + finished[x][y].constructConfigFileName());
				}

				//TODO merge konfigurace pres jednotlive uzly
			
				if (finished[x][y].consumption.getAvgEnergy() <= 0.0)
				{
					std::cerr << "WARNING no energy consumption: ";
					finished[x][y].print();
					continue; //the configuration would be identified as optimal, so we must skip it
				}
				//compare the configuration with the so far the best configuration and update the optimalConfiguration
				int i=0;
				for (;i<optimalConfiguration.size(); i++)
				{
					if (optimalConfiguration[i].regionName == finished[x][y].regionName)
					{ 
						if (optimalConfiguration[i].consumption.getAvgEnergy() > finished[x][y].consumption.getAvgEnergy() //TODO default run
						    // && defaultConfiguration[i].consumption.getAvgTime()*extention  > finished[x][y].consumption.getAvgTime()
						    //TODO extention 
						)
						{
//							std::cout << "Improved solution, FROM reg '"; optimalConfiguration[i].print();
//							std::cout << "Improved solution,   TO reg '"; finished[x][y].print();
							
							optimalConfiguration[i] = finished[x][y];
							//optimalConfiguration.erase(optimalConfiguration.begin()+i);
							//optimalConfiguration.push_back(finished[x][y]);
							ImprovedSolution++;
						}
						break;
					}
				}
				if (i == optimalConfiguration.size()) //region has not been found
					optimalConfiguration.push_back(finished[x][y]);

//				break;	//TODO remove 1st configuration only
			}
//			break;	//TODO remove 1st region only
		}
		
		//TODO popis toho co se deje na nasledujicich radcich
		if (!selectedConfiguration.size())
			selectedConfiguration = optimalConfiguration;
		else if (selectedConfiguration.size() != optimalConfiguration.size())
		{
			for (int i=selectedConfiguration.size(); i<optimalConfiguration.size(); i++)
				selectedConfiguration[i] = optimalConfiguration[i];
		}
		
		for(auto appConf : finished)
		{
			for (regionConfiguration c : appConf)
			{
				int i=0;
				for (;i<optimalConfiguration.size(); i++)
				{
					if (optimalConfiguration[i].regionName == c.regionName)
					{
						if (optimalConfiguration[i].consumption.getAvgEnergy()*margin > c.consumption.getAvgEnergy() &&
						    selectedConfiguration[i].consumption.getAvgTime() > c.consumption.getAvgTime())
						{
							selectedConfiguration[i] = c;
						}
						break;
					}
				}
			}
		}
/*
std::cout << " * OPTIMAL CONFIGURATION * \n";
for (regionConfiguration optC : optimalConfiguration)
{
	optC.print();
}

std::cout << " * SELECTED CONFIGURATION * \n";
for (regionConfiguration optC : selectedConfiguration)
{
	optC.print();
}
*/

		storeMERICconfig(optimalConfiguration, "optimalConfig.opts");
		return ImprovedSolution;
	}

private:
	static std::string label_energy;
	static std::string label_dataSource;
	static std::string label_time;
	static std::vector <std::string> hostnames;
	static double baseline;
	static double margin;
	static double extention;
	static appConfiguration optimalConfiguration;
	static appConfiguration selectedConfiguration;
	static appConfiguration defaultConfiguration;
};

std::string DataAnalysis::label_energy = "Energy consumption [J]";
std::string DataAnalysis::label_dataSource = "";
std::string DataAnalysis::label_time = "Runtime of function [s]";
std::vector <std::string> DataAnalysis::hostnames;
std::vector <std::string> DataAnalysis::regions;
std::vector <std::string> DataAnalysis::dataFormat;
std::string DataAnalysis::measurementDirectory;
double DataAnalysis::baseline = 0.0;
double DataAnalysis::margin = 1.0;
double DataAnalysis::extention = 1.0;
appConfiguration DataAnalysis::optimalConfiguration;
appConfiguration DataAnalysis::selectedConfiguration;
appConfiguration DataAnalysis::defaultConfiguration;

#endif // SRC_TOOLS_WRAPPER_EVALUATE_DATAANALYSIS_H_
