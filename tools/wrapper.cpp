//g++ -std=c++11 wrapper.cpp 

#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include <list>
#include <string>
#include <string.h>
#include <iostream>
#include <dirent.h> //list of files in a dir

#include <unistd.h>
#include <fstream>
#include <limits>
#include <time.h>

const std::string APPLICATION="/galileo/home/userexternal/ovysocky/meric/test/test";
std::string MERIC_OUTPUT_DIR;
// STRUCTS /////////////////////////////////////////////////////////////////////

enum ALGORITHM {EXHAUSTIVE=0, EVO=1, PSO=2};

struct systemLimits{
	int  coresPerSocket;
	int  socketsPerNode;
	
	long maxCoreFreq;            //Hz
	long minCoreFreq;            //Hz

	long maxUncoreFreq;          //Hz
	long minUncoreFreq;          //Hz

	long maxPowerCapPKG;         //W
	long minPowerCapPKG;         //W

	long maxPowerCapPKGtime;     //us

	long maxPowerCapRAM;         //W
	long minPowerCapRAM;         //W

	long maxPowerCapRAMtime;     //us
};

struct configuration{	// single HW configuration
	int  threads;
	long coreFreq;
	long uncoreFreq;
	long powerCapPKG;
	long powerCapPKGtime;
	
	bool operator==(const configuration& c) const
	{
		if (this->threads != c.threads)
			return false;
		if (this->coreFreq != c.coreFreq)
			return false;
		if (this->uncoreFreq != c.uncoreFreq)
			return false;
		if (this->powerCapPKG != c.powerCapPKG)
			return false;
		if (this->powerCapPKGtime != c.powerCapPKGtime)
			return false;
		return true;
	}
};

configuration selectedParams = {0, 0, 0, 0, 0}; //which parameteres will be tuned

struct regionConfiguration{ //configuration for a region
	std::string regName;
	struct configuration regConfiguration;
	double energy;
	double runtime;

	bool operator<(const regionConfiguration& c) const
	{
		return (this->energy < c.energy);
	}
};

typedef std::vector <regionConfiguration> appConfiguration; //configurations for the application's regions

appConfiguration optimalConfiguration;
std::vector <appConfiguration> testConfigurationsToTry;
std::vector <appConfiguration> testConfigurationsDone;
unsigned long newconfigurations;

std::vector <std::string> appRegions; //list of regions of the application

// FUNCTIONS ///////////////////////////////////////////////////////////////////
/*
{
	"DGEMM" : {
		"FREQUENCY" : 1500MHz,
		"UNCORE_FREQUENCY" : 1500MHz,
		"NUM_THREADS" : 12
	}
	,
	"DGEMV" : { 
		"FREQUENCY" : 2300MHz,
		"UNCORE_FREQUENCY" : 2300MHz
	}
}
*/
void generateConfigFile(appConfiguration config, const std::string fileName)
{
	std::string content = "{";
	for (regionConfiguration region : config)
	{
		content += "\"" + region.regName + "\" : {\n";
		if (region.regConfiguration.threads)
			content += "\"NUM_THREADS\" : " + std::to_string(region.regConfiguration.threads) + ",\n";
		if (region.regConfiguration.coreFreq)
			content += "\"FREQUENCY\" : " + std::to_string(region.regConfiguration.coreFreq) + ",\n";
		if (region.regConfiguration.uncoreFreq)
			content += "\"UNCORE_FREQUENCY\" : " + std::to_string(region.regConfiguration.uncoreFreq) + ",\n";
		if (region.regConfiguration.powerCapPKG)
			content += "\"PWRCAP_POWER\" : " + std::to_string(region.regConfiguration.powerCapPKG) + ",\n";
		if (region.regConfiguration.powerCapPKGtime)
			content += "\"PWRCAP_TIME\" : " + std::to_string(region.regConfiguration.powerCapPKGtime) + "\n"; // + ",\n";
		if (content.size() > 2 && content[content.size()-2] == ',')
			content = content.substr(0,content.size()-2);
		content += "},\n";
	}
	if (content.size() > 2 && content[content.size()-2] == ',')
		content = content.substr(0,content.size()-2);
	content += "\n}\n";

	//store to a file
	std::ofstream configFile (fileName.c_str());
	if(configFile.is_open())
		configFile << content;
}


/*
void processFile(std::string fileName, std::vector<std::string> * elements)
{
	if(fileName != "")
	{
		std::ifstream inputfile (fileName.c_str());
		if(inputfile.is_open())
		{
			std::string element;
			while (inputfile >> element)
				(*elements).push_back(element);
			inputfile.close();
		}
	}
}
*/

void readParameters()
{
/*
# TODO - nacteni parametru
# - command spoustejici aplikaci
# - pocet iteraci se stejnou konfiguraci
# - energy measurement system
# - algoritmus prohledavani SP
# - systemove parametry ktere se maji ladit, krok s jakym prohledavat SP daneho parametru
# - maximalni vliv konfigurace na cas

export MERIC_ITERATION=0
export MERIC_MODE=7	#enum {hdeem=0, rapl=1, both=2, run=3, jetson=4, thunder=5, davide=6, time=7}
*/

	struct timespec timeNow;
	clock_gettime(CLOCK_REALTIME, &timeNow);
	MERIC_OUTPUT_DIR = "MERICwrapper-"+std::to_string(timeNow.tv_sec);
	setenv("MERIC_OUTPUT_DIR", MERIC_OUTPUT_DIR.c_str(), 1);

	setenv("MERIC_COUNTERS", "papi", 1);
	setenv("MERIC_MODE", "1", 1);
	setenv("MERIC_ITERATION", "0", 1);
}

void constructCallPath()
{
	//read list of application's regions
	DIR *dir;
	struct dirent *ent;
	if ((dir = opendir(MERIC_OUTPUT_DIR.c_str())) != NULL)
	{
		while ((ent = readdir(dir)) != NULL)
		{
			if (ent->d_type == DT_DIR)
			{
				if (ent->d_name[0] != '.')
				{
//					std::cout << ent->d_name << std::endl;
					appRegions.push_back(std::string(ent->d_name));
				}
			}
		}
		closedir (dir);
	}
	else
		return;
	
	//TODO - construct callpath graph
	// ...
}

systemLimits constructStateSpace()
{
	//TODO code based 
//systemLimits limit = {12, 2, 2500000000, 2300000000, 3000000000, 2800000000, 0, 0, 0, 0, 0, 0};

//	systemLimits limit = {12, 2, 2500000000, 1200000000, 3000000000, 1200000000, 0, 0, 0, 0, 0, 0}; //Salomon
	systemLimits limit = {18, 2, 2300000000, 1200000000, 2800000000, 1200000000, 0, 0, 0, 0, 0, 0}; //Galileo

	selectedParams.threads=1;
	selectedParams.coreFreq = 1;
	selectedParams.uncoreFreq = 1;
	selectedParams.powerCapPKG = 0;
	selectedParams.powerCapPKGtime = 0;
	
	//threads
	//CF
	//UnCF
	//RAPLpower
	//RAPLwindow

	newconfigurations = 0;
	return limit;
}

configuration decodeConfigVector (std::vector<long> values)
{
	////////////////////////////
	std::cout << "@: ";
	for (auto e : values)
		std::cout << e << " ";
	std::cout << std::endl;
	////////////////////////////
	
	configuration v;
	int iter = 0;
	if (selectedParams.uncoreFreq)
		v.uncoreFreq = values[iter++];
	if (selectedParams.coreFreq)
		v.coreFreq = values[iter++];
	if (selectedParams.threads)
		v.threads = values[iter++];
	if (selectedParams.powerCapPKG)
		v.powerCapPKG = values[iter++];
	if (selectedParams.powerCapPKGtime)
		v.powerCapPKGtime = values[iter++];
	return v;
}

void EXHAUSTIVE_generateConfigurationInRecursive(size_t size, std::vector <std::vector<long>> vecConfig, std::vector <long> v)
{
	size--;
	for (long elem = 0; vecConfig[size].size() > elem; elem++)
	{
		v.push_back(vecConfig[size][elem]);
		if (size == 0)
		{
			struct regionConfiguration c = {"", decodeConfigVector(v), std::numeric_limits<double>::max(), 0.0 };
			appConfiguration a = {c};
			testConfigurationsToTry.push_back(a);
			v.pop_back();
		}
		else
		{
			EXHAUSTIVE_generateConfigurationInRecursive(size, vecConfig, v);
			v.pop_back();
		}
	}
}

size_t getRegionIndex(std::string region)
{
	size_t regIndex = -1;
	for (int r=0; r < optimalConfiguration.size(); r++)
	{
		if (optimalConfiguration[r].regName == region)
		{
			regIndex = r;
			break;
		}
	}
	if (regIndex == -1)
	{
		optimalConfiguration.push_back({ region, {0, 0, 0, 0, 0}, std::numeric_limits<double>::max(), 0.0 });
		regIndex = optimalConfiguration.size()-1;
	}
	return regIndex;
}

bool testIfConfigExists(std::string regName, configuration c)
{	//TODO in the finished configurations
	bool find = false;
	size_t index = 0;
	while (index < testConfigurationsToTry.size() && find == false)
	{
		for (regionConfiguration reg : testConfigurationsToTry[index])
		{
			if (reg.regName == regName)
			{
				find = (reg.regConfiguration == c);
				break;
			}
		}
		index++;
	}
	return find;
}

configuration getRandomConfig(const systemLimits limit)
{
	configuration c = {0, 0, 0, 0, 0};
	if (selectedParams.uncoreFreq)
	{
		int max = (limit.maxUncoreFreq - limit.minUncoreFreq)/100000000;
		long val = rand() % max;
		c.uncoreFreq = limit.minUncoreFreq + val*100000000;
	}
	if (selectedParams.coreFreq)
	{
		int max = (limit.maxCoreFreq - limit.minCoreFreq)/100000000;
		long val = rand() % max;
		c.coreFreq = limit.minCoreFreq + val * 100000000;
	}
	if (selectedParams.threads)
	{
		c.threads = rand() % (limit.coresPerSocket * limit.socketsPerNode)+1;
	}
/*	//TODO
	if (selectedParams.powerCapPKG)
		c.powerCapPKG = limit.maxPowerCapPKG;
	if (selectedParams.powerCapPKGtime)
		c.powerCapPKGtime = limit.maxPowerCapPKGtime;
*/

std::cout << "GENcfg: " << c.uncoreFreq << " " << c.coreFreq << " " << c.threads << std::endl;
	return c;
}

/*
		double compareWithTheBest (std::string regName, configuration config)
		{
			size_t regIndex = getRegionIndex(regName);
			//open file that has measurement results for the region in the specified configuration

			std::string cfgStr = "";
			if (config.coreFreq)
				cfgStr += std::to_string(config.coreFreq)+"_";
			if (config.uncoreFreq)
				cfgStr += std::to_string(config.uncoreFreq)+"_";
			if (config.threads)
				cfgStr += std::to_string(config.threads)+"_";
			if (config.powerCapPKG)
				cfgStr += std::to_string(config.powerCapPKG)+"_";
			if (config.powerCapPKGtime)
				cfgStr += std::to_string(config.powerCapPKGtime);
			if (cfgStr.back() == '_')
				cfgStr.pop_back();

			double energyConsumption = 0.0;
			const std::string fileName = MERIC_OUTPUT_DIR+"/"+regName+"/"+cfgStr+".csv";
			std::ifstream inputfile (fileName);
			if(inputfile.is_open())
			{
				std::string element;
				//TODO runtime
				const std::string label = "Energy summary [J]";
				while (!inputfile.eof())
				{
					getline(inputfile,element);
					if (element.find(label) != std::string::npos)
					{
						energyConsumption = std::stod(element.substr(label.size()+1));
//std::cout << regName << " - " << cfgStr << " : " << val << std::endl;
						if (energyConsumption < optimalConfiguration[regIndex].energy)
						{
							optimalConfiguration[regIndex].energy = energyConsumption;
							optimalConfiguration[regIndex].regConfiguration = config;
						}

						break; //TODO just the first one
					}
				}
				inputfile.close();
			}
			return energyConsumption;
		}
*/

//EVO - EXHAUSTIVE - PSO
size_t generateConfigurationsToTest(const systemLimits limit, ALGORITHM alg)
{
	//identify so-far the best configurations - exhaustive
	//currently default behavior is not evaluated
	if (newconfigurations != 0)
	{
/*
- nactu posledni konfigurace z testConfigurationsDone, dle velikosti newconfigurations
- pro kazdy region urcim jestli v dane konfiguraci je spotreba energie mensi nez doposud nejnizsi hodnota
- ulozim konfiguraci, spotrebu regionu, runtime, rozdil runtime oproti default
	+ v pripade nastaveni maximalni performance penalty je treba zaznamenat kombinace konfiguraci, ktere mohou vest k uspore energie ale prodluzuji runtime - kombinace, ktera prodlouzi runtime o 1-x %
	
	+ musim si spocitat o kolik sekund muzu protahnout runtime + kolik procent (skupin) to je
	+ seradim si vyrazene konfigurace jednotlivych regionu podle toho jaky maji dopad na runtime - z tohoto sezanamu vyberu vsechny konfigurace s dopadem mensim, nez uvedeny limit a nasledne beru v potaz dalsi konfigurace ze seznamu, v poradi dle dopadu na runtime. ty konfigurace musim srovnat se senamem konfiguraci ktere mely vetsi dopad a jejihz soucet dopadu na cas je vetsi nebo roven dopadu porovnavane konfigurace. takto muze dojit k vyrazeni konfiguraece, ktera runtime ovlivni jen minimalne, ale jeste by se do limitu vlezla (dopad dalsiho nahrazovani konfigurace), takze proces je treba jeste zopakovat - dokud se budou provadet zmeny.
	+ presto muze jit o suboptimalni reseni!!!
	
	+ obdoba algoritmu, pro nakladani auta
*/

		///*
		auto compareWithTheBest = [&] (std::string regName, configuration config)
		{
			size_t regIndex = getRegionIndex(regName);
			//open file that has measurement results for the region in the specified configuration

			std::string cfgStr = "";
			if (config.coreFreq)
				cfgStr += std::to_string(config.coreFreq)+"_";
			if (config.uncoreFreq)
				cfgStr += std::to_string(config.uncoreFreq)+"_";
			if (config.threads)
				cfgStr += std::to_string(config.threads)+"_";
			if (config.powerCapPKG)
				cfgStr += std::to_string(config.powerCapPKG)+"_";
			if (config.powerCapPKGtime)
				cfgStr += std::to_string(config.powerCapPKGtime);
			if (cfgStr.back() == '_')
				cfgStr.pop_back();

			double energyConsumption = 0.0;
			const std::string fileName = MERIC_OUTPUT_DIR+"/"+regName+"/"+cfgStr+".csv";
			std::ifstream inputfile (fileName);
			if(inputfile.is_open())
			{
				/*
				 * e.g.
					# Blade summary
					...
					Energy consumption [J],1549.86
				 * or
					# COUNTERS - RAPL: 
					...
					Energy summary [J],410
				*/
				std::string element;
				//TODO runtime
				const std::string label = "Energy summary [J]";
				while (!inputfile.eof())
				{
					getline(inputfile,element);
					if (element.find(label) != std::string::npos)
					{
						energyConsumption = std::stod(element.substr(label.size()+1));
		//std::cout << regName << " - " << cfgStr << " : " << val << std::endl;
						if (energyConsumption < optimalConfiguration[regIndex].energy)
						{
							optimalConfiguration[regIndex].energy = energyConsumption;
							optimalConfiguration[regIndex].regConfiguration = config;
						}

						break; //TODO just the first one
					}
				}
				inputfile.close();
			}
			return energyConsumption;
		};
		//*/



std::cout << "searching for the best config so-far\n";
	
		size_t i = testConfigurationsDone.size() - newconfigurations;

		for (size_t newbies = newconfigurations; newbies > 0; newbies--)
		{
			for (size_t zz = 0; zz < testConfigurationsDone[i].size(); zz++)
			{
				if (testConfigurationsDone[i][zz].regName == "")
				{
					for (std::string regName : appRegions)
						testConfigurationsDone[i].push_back({regName, testConfigurationsDone[i][zz].regConfiguration, compareWithTheBest(regName, testConfigurationsDone[i][zz].regConfiguration), 0.0});
					break;
				}
				else
				{
					testConfigurationsDone[i][zz].energy = compareWithTheBest(testConfigurationsDone[i][zz].regName, testConfigurationsDone[i][zz].regConfiguration);
				}
			}
/*			
			for (regionConfiguration xx : testConfigurationsDone[i])
			{
				if (xx.regName == "")
				{
					for (std::string regName : appRegions)
						compareWithTheBest(regName, xx.regConfiguration);
				}
				else
				{
					compareWithTheBest(xx.regName, xx.regConfiguration);
				}
			}
*/
			i++;
		}
	}

/// EVO ////////////////////////////////////////////////////////////////////////
	if (alg == EVO)
	{
		//INITIAL GENERATION
		if (!testConfigurationsDone.size())
		{
			const int EVO_POPULATION_SIZE = 10;
			configuration v = {0, 0, 0, 0, 0};
			if (selectedParams.uncoreFreq)
				v.uncoreFreq = limit.maxUncoreFreq;
			if (selectedParams.coreFreq)
				v.coreFreq = limit.maxCoreFreq;
			if (selectedParams.threads)
				v.threads = limit.coresPerSocket * limit.socketsPerNode;
			if (selectedParams.powerCapPKG)
				v.powerCapPKG = limit.maxPowerCapPKG;
			if (selectedParams.powerCapPKGtime)
				v.powerCapPKGtime = limit.maxPowerCapPKGtime;
			testConfigurationsToTry.push_back({{"", v, std::numeric_limits<double>::max(), 0.0 }});//MAX
			
			while (testConfigurationsToTry.size() < EVO_POPULATION_SIZE)
			{
std::cout << "EVO size - " << testConfigurationsToTry.size() << std::endl;
				appConfiguration ac;
				//for (std::string regName : appRegs)
				std::string regName = "";
				{
					configuration c = getRandomConfig(limit);
					while (testIfConfigExists(regName, c))
						c = getRandomConfig(limit);
					ac.push_back({regName, c, std::numeric_limits<double>::max(), 0.0});
				}
				testConfigurationsToTry.push_back(ac);
			}
		}
		else
		{
/*
struct regionConfiguration{ //configuration for a region
	std::string regName;
	struct configuration regConfiguration;
	double energy;
	double runtime;
};
*/

			//TODO move the init somewhere
			std::vector <std::list<regionConfiguration>> population;
			population.resize(appRegions.size());

			/*
			size_t i = testConfigurationsDone.size() - newconfigurations;

			for (size_t newbies = newconfigurations; newbies > 0; newbies--)
			{
				for (size_t zz = 0; zz < testConfigurationsDone[i].size(); zz++)
				{
					if (testConfigurationsDone[i][zz].regName == "")
					{
						for (std::string regName : appRegions)
							testConfigurationsDone[i].push_back({regName, testConfigurationsDone[i][zz].regConfiguration, compareWithTheBest(regName, testConfigurationsDone[i][zz].regConfiguration), 0.0});
						break;
					}
				}
			}
			*/
			
		}
	}
/// EXHAUSTIVE /////////////////////////////////////////////////////////////////
	if (alg == EXHAUSTIVE && testConfigurationsDone.size() == 0)
	{
		std::vector <std::string> parameterName;
		std::vector <std::vector<long>> exhConf;
		std::vector <long> cfg;
		//threads
//		for (long t = limit.coresPerSocket * limit.socketsPerNode; t >= 1; t--) //TODO stepSize
		for (long t = limit.coresPerSocket * limit.socketsPerNode; t >= 1; t-= limit.coresPerSocket) //TODO stepSize
		{
			cfg.push_back(t);
			std::cout << "T: "<<t<<std::endl;
		}
		if (cfg.size() != 0)
			exhConf.push_back(cfg);
		cfg.clear();

		//CF
		for (long c = limit.maxCoreFreq; c >= limit.minCoreFreq; c-= 100000000) //TODO
		{
			cfg.push_back(c);
			std::cout << "C: "<<c<<std::endl;
		}
		if (cfg.size() != 0)
			exhConf.push_back(cfg);
		cfg.clear();

		//UCF
		for (long u = limit.maxUncoreFreq; u >= limit.minUncoreFreq; u-=100000000) //TODO
		{
			cfg.push_back(u);
			std::cout << "U: "<<u<<std::endl;
		}
		if (cfg.size() != 0)
			exhConf.push_back(cfg);
		cfg.clear();
/*
		//TODO RAPL
*/

std::cout << std::endl << std::endl;
		
		std::vector <long> v;
		EXHAUSTIVE_generateConfigurationInRecursive(exhConf.size(), exhConf, v);
	}

	newconfigurations = 0;
	return testConfigurationsToTry.size();	//this should be the only return statement in this function
}


void runApplication(appConfiguration testRun)
{

	//testRun.size() == 0 ??
	
	if (testRun.size() == 1 && testRun[0].regName == "")
	{
		std::cout << "Running: C" <<  testRun[0].regConfiguration.coreFreq << " U" << testRun[0].regConfiguration.uncoreFreq << " T" << testRun[0].regConfiguration.threads << std::endl;

		setenv("MERIC_FREQUENCY", std::to_string(testRun[0].regConfiguration.coreFreq).c_str(), 1);
		setenv("MERIC_UNCORE_FREQUENCY", std::to_string(testRun[0].regConfiguration.uncoreFreq).c_str(), 1);
		setenv("MERIC_NUM_THREADS", std::to_string(testRun[0].regConfiguration.threads).c_str(), 1);
		setenv("MERIC_PWRCAP_POWER", std::to_string(testRun[0].regConfiguration.powerCapPKG).c_str(), 1);
		setenv("MERIC_PWRCAP_TIME", std::to_string(testRun[0].regConfiguration.powerCapPKGtime).c_str(), 1);
		unsetenv("MERIC_REGION_OPTIONS");
	}
	else
	{
		const std::string MERIC_OUTPUT_FILENAME = MERIC_OUTPUT_DIR+"/MERICwrapperTestconfiguration.json";
		generateConfigFile(testRun, MERIC_OUTPUT_FILENAME);
		setenv("MERIC_FREQUENCY", "0", 1);
		setenv("MERIC_UNCORE_FREQUENCY", "0", 1);
		setenv("MERIC_NUM_THREADS", "0", 1);
		setenv("MERIC_PWRCAP_POWER", "0", 1);
		setenv("MERIC_PWRCAP_TIME", "0", 1);
		setenv("MERIC_REGION_OPTIONS", MERIC_OUTPUT_FILENAME.c_str(), 1);
	}

//	system ("printenv");
	system (APPLICATION.c_str());
	//TODO remove the test configuration file
}


int main()
{
//   TODO   //
// // // // //
ALGORITHM alg = EVO;
//ALGORITHM alg = EXHAUSTIVE;
// // // // //

	readParameters();
	appConfiguration defaultRun = {{ "", {0, 0, 0, 0, 0}, std::numeric_limits<double>::max(), 0.0 }};
	runApplication(defaultRun);

//state space
	constructCallPath();
	const systemLimits limit = constructStateSpace();

std::cout << "RUN ANALYSIS\n";
//analyzis
	while (generateConfigurationsToTest(limit, alg))
	{
		while(testConfigurationsToTry.size())
		{
			runApplication(testConfigurationsToTry[0]);
			testConfigurationsDone.push_back(testConfigurationsToTry[0]);
			testConfigurationsToTry.erase(testConfigurationsToTry.begin());
			newconfigurations++;
		}
	}
//store optimal configuration
	generateConfigFile(optimalConfiguration, MERIC_OUTPUT_DIR+"/MERICoptimalConfiguration.json");
std::cout << MERIC_OUTPUT_DIR+"/MERICoptimalConfiguration.json" << std::endl;
	return 0;
}




















