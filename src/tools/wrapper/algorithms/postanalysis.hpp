
#ifndef SRC_TOOLS_WRAPPER_ALGORITHMS_POSTANALYSIS_HPP_
#define SRC_TOOLS_WRAPPER_ALGORITHMS_POSTANALYSIS_HPP_

#include "../parameter/statespace.h"

class postanalysis : public configurationStateSpace {

public:
	int generateConfigurations(std::vector<appConfiguration> & space, std::vector<appConfiguration> & finished)
	{
	
		std::vector <dummyparameter> parameters;
		for(std::string parameter : DataAnalysis::dataFormat)
		{
			dummyparameter p(parameter); //nemam vector valid values
			parameters.push_back(p);		
/*			else if (parameter == "CF")
				selectedParams.coreFreq = true;
			else if (parameter == "UnCF")
				selectedParams.uncoreFreq = true;
			else if (parameter == "thrds")
				selectedParams.threads = true;
			else if (parameter == "PWR")
				selectedParams.powerCapPKG = true;
			else if (parameter == "Twindow")
				selectedParams.powerCapPKGtime = true;
			// TODO - APPx parameters
			else if (parameter.substr(0,3) == "APP")
			{
				std::cerr << "ERROR: application parameters currently not supported\n";
			}
*/
		}

//		long confCnt = 0;
/*		if ((dir = opendir((DataAnalysis::measurementDirectory+"/"+DataAnalysis::regions[0]+"/").c_str())) != NULL)
		{
			while ((ent = readdir(dir)) != NULL)
			{
				std::string val = std::string(ent->d_name);
				if (ent->d_type == DT_REG && val.length() > 4 && val.substr(val.length()-4) == ".csv")
					confCnt++;
			}
		}
*/
	
		std::vector<std::vector<regionConfiguration>> allRegionConfigurations(DataAnalysis::regions.size());	//TODO use struct appConfiguration
/*		regionConfiguration rc[5][10];
		regionConfiguration** rc = new regionConfiguration * [DataAnalysis::regions.size()];		
		for (int i=0; i<DataAnalysis::regions.size(); i++)
			rc[i] = new regionConfiguration[confCnt];
*/	
	
		auto addCofigurationValue = [&] (regionConfiguration & conf, int paramID, std::string token)
		{
			int valueID = parameters[paramID].isValidValue(token, true);
			if (valueID >= 0)
			{
				parameterValueNew pv(paramID, valueID);
				conf.addParameterValue(pv, std::bind(&dummyparameter::setParameterValue, parameters[paramID], valueID), std::bind(&dummyparameter::getParameterValue, parameters[paramID], valueID));
				return true;
			}
			return false;
		};
		
		/*
		TODO neni vyreseno nacitani dat kdyz prvni parametr je hostname
		
		TODO pozor - funkcni jen v pripade ze analyza byla procevdena pomoci nastaveni staticke konfigurace - jinak se budou spatne odcitat od sebe jednotliva mereni
		*/

		//if the dataFormat was specified MERIC stores the output in the format 0_0_0_ ... .csv
		std::string defaultFileName = "0";
		for(int d=1; d< DataAnalysis::dataFormat.size(); d++)
			defaultFileName += "_0";
		defaultFileName += ".csv";
								
		//load the data from the files
		for (int regionID = 0; regionID < DataAnalysis::regions.size(); regionID++)
		{
			// list of configurations of the region
			DIR *dir;
			if ((dir = opendir((DataAnalysis::measurementDirectory+"/"+DataAnalysis::regions[regionID]+"/").c_str())) != NULL)
			{
				struct dirent *ent;
				while ((ent = readdir(dir)) != NULL)
				{
					std::string fileName = std::string(ent->d_name);
					if (fileName == "default.csv" || fileName == defaultFileName)
					{	
						continue; // the default run is loaded during DataAnalysis::analyse();
					}
					else if (ent->d_type == DT_REG && fileName.length() > 4 && fileName.substr(fileName.length()-4) == ".csv")
					{
//std::cout << "Region " << DataAnalysis::regions[regionID] << " fileName = " << fileName << std::endl;
						size_t pos = 0;
						int paramID = 0;
						regionConfiguration conf(DataAnalysis::regions[regionID]);
						bool confOK = true;
						while ((pos = fileName.find('_')) != std::string::npos)
						{
							std::string parameterValue = fileName.substr(0, pos);
							//if (parameterValue != "0") //TODO this line and line+10 forces to skip default executions, temporary turned off condition until the moment when the radar will be able to load just the default run without meric config file
								confOK = addCofigurationValue(conf, paramID, parameterValue);
							if (confOK == false)
								break;
							fileName.erase(0, pos + 1);
							paramID++;
						}
						if (confOK == true)
						{
							std::string parameterValue = fileName.substr(0,fileName.length()-4);
						 	//if (parameterValue != "0" && addCofigurationValue(conf, paramID, parameterValue) == true)
						 	if (addCofigurationValue(conf, paramID, parameterValue) == true)
								allRegionConfigurations[regionID].push_back(conf);
						}
					}
				}
				closedir (dir);
			}
		}

for (auto p : parameters)
	p.printList();
	
		//TODO the analysis works only if every execution is a new configuration

		//fill vector finished, that holds application configurations in which the application was executed 
		//static region has regionID == 0, this region is called in evergy application execution
		for (int i=0; i<allRegionConfigurations[0].size(); i++)
		{
			appConfiguration appc;
			for (int regionID = 0; regionID < DataAnalysis::regions.size(); regionID++)
				appc.push_back(allRegionConfigurations[regionID][i]);
				//appc.push_back(rc[regionID][i]);
			finished.push_back(appc);
		}
		
		DataAnalysis::analyse(finished);
		
		
/*		
		for(int i = 0; i < DataAnalysis::regions.size(); ++i)
			delete [] rc[i];
		delete [] rc;
*/	
		return 0;
	}

};


#endif // SRC_TOOLS_WRAPPER_ALGORITHMS_POSTANALYSIS_HPP_

