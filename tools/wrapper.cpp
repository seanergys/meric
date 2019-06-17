//g++ -std=c++11 wrapper.cpp 

#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include <string>
#include <string.h>
#include <iostream>
#include <dirent.h> //list of files in a dir

#include <unistd.h>
#include <fstream>

const std::string APPLICATION="mpirun -n 2 ./testmpi";
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
};

configuration selectedParams = {0, 0, 0, 0, 0}; //which parameteres will be tuned

struct regionConfiguration{ //configuration for a region
	std::string regName;
	struct configuration regConfiguration;
	double energy;
	double runtime;
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
	std::string content = "}";
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
		if (content.substr(content.size()-2) == ",\n")
			content = content.substr(0,content.size()-3);
//		if (content[:-2] == ",\n")
//			content = content[-2:];
		content += "},\n";
	}
		if (content.back() == ',')
			content = content.substr(0,content.size()-2);
//	if (content[-1] == ",")
//			content = content[-1:];
	content += "}\n";

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

	std::string MERIC_ITERATION="MERIC_ITERATION=0";
	std::string MERIC_MODE="MERIC_MODE=7";
	putenv(&MERIC_ITERATION[0]);
	putenv(&MERIC_MODE[0]);

	struct timespec timeNow;
	clock_gettime(CLOCK_REALTIME, &timeNow);
	MERIC_OUTPUT_DIR = "MERIC_OUTPUT_DIR=MERICwrapper-"+std::to_string(timeNow.tv_sec);
	putenv(&MERIC_OUTPUT_DIR[0]);
}

/*
void runDefault()
{
	putenv("MERIC_FREQUENCY=0");
	putenv("MERIC_UNCORE_FREQUENCY=0");
	putenv("MERIC_NUM_THREADS=0");
	putenv("MERIC_PWRCAP_POWER=0");
	putenv("MERIC_PWRCAP_TIME=0");

	putenv("MERIC_COUNTERS=papi");
	unsetenv("MERIC_REGION_OPTIONS");

	system (APPLICATION);
}
*/

void constructCallPath()
{
	//TODO - nacteni seznamu regionu
	DIR *dir;
	struct dirent *ent;
	if ((dir = opendir(MERIC_OUTPUT_DIR.c_str())) != NULL)
	{
		/* print all the files and directories within directory */
		while ((ent = readdir(dir)) != NULL) {
			printf ("%s\n", ent->d_name);
			appRegions.push_back(std::string(ent->d_name)); //navic zde bude JSON
		}
		closedir (dir);
	}
	else
		return;
	
	//TODO - vytvoreni stromu volani
	// ...
}

systemLimits constructStateSpace()
{
	//TODO code based 
	systemLimits limit = {24, 2, 2500000000, 1200000000, 3000000000, 1200000000, 0, 0, 0, 0, 0, 0}; //Salomon

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
	if (selectedParams.threads)
		v.threads = values[iter++];
	if (selectedParams.coreFreq)
		v.coreFreq = values[iter++];
	if (selectedParams.uncoreFreq)
		v.uncoreFreq = values[iter++];
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
			struct regionConfiguration c = {"", decodeConfigVector(v), 0.0, 0.0 };
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
		optimalConfiguration.push_back({ region, {0, 0, 0, 0, 0}, 0.0, 0.0 });
		regIndex = optimalConfiguration.size()-1;
	}
	return regIndex;
}


size_t generateConfigurationsToTest(const systemLimits limit, ALGORITHM alg)
{
	//identify so-far the best configurations - exhaustive
	if (newconfigurations != 0)
	{
		size_t i = testConfigurationsDone.size() - newconfigurations;
		for (; newconfigurations > 0; newconfigurations--)
		{
			for (auto xx : testConfigurationsDone[i])
			{
				size_t regIndex = getRegionIndex(xx.regName);
				//open file that has measurement results for the region in the specified configuration

				std::string cfgStr = "";
				if (xx.regConfiguration.threads)
					cfgStr += xx.regConfiguration.threads+"_";
				if (xx.regConfiguration.coreFreq)
					cfgStr += xx.regConfiguration.coreFreq+"_";
				if (xx.regConfiguration.uncoreFreq)
					cfgStr += xx.regConfiguration.uncoreFreq+"_";
				if (xx.regConfiguration.powerCapPKG)
					cfgStr += xx.regConfiguration.powerCapPKG+"_";
				if (xx.regConfiguration.powerCapPKGtime)
					cfgStr += xx.regConfiguration.powerCapPKGtime;
				if (cfgStr.back() == '_')
					cfgStr.pop_back();
		
				const std::string fileName = MERIC_OUTPUT_DIR+"/"+xx.regName+"/"+cfgStr+".csv";
				std::ifstream inputfile (fileName);
				if(inputfile.is_open())
				{
					/*
					 * e.g.
						# Blade summary
						...
						Energy consumption [J],1549.86
					*/
					std::string element;
					//TODO runtime
					const std::string label = "Energy consumption [J]";
					while (inputfile >> element)
						if (element.find(label) != std::string::npos)
						{
							double val = std::stod(element.substr(label.size()));
							if (val < optimalConfiguration[regIndex].energy)
							{
								optimalConfiguration[regIndex].energy = val;
								optimalConfiguration[regIndex].regConfiguration = xx.regConfiguration;
							}

							break; //TODO just the first one
						}
					inputfile.close();
				}
			}
			i--;
		}
		newconfigurations = 0;
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
	}
	
	//EVO - EXHAUSTIVE - PSO
	if (alg == EXHAUSTIVE && testConfigurationsDone.size() == 0)
	{
	
		configuration selectedParams = {0, 0, 0, 0, 0};

		std::vector <std::string> parameterName;
		std::vector <std::vector<long>> exhConf;
		std::vector <long> cfg;
		//threads
		for (long t = limit.coresPerSocket * limit.socketsPerNode; t >= 1; t--) //TODO stepSize
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

	return testConfigurationsToTry.size();
}


void runApplication(appConfiguration testRun)
{
	//testRun.size() == 0 ??
	
	if (testRun.size() == 1 && testRun[0].regName == "")
	{
		putenv(&(("MERIC_FREQUENCY="+std::to_string(testRun[0].regConfiguration.coreFreq))[0]));
		putenv(&(("MERIC_UNCORE_FREQUENCY="+std::to_string(testRun[0].regConfiguration.uncoreFreq))[0]));
		putenv(&(("MERIC_NUM_THREADS="+std::to_string(testRun[0].regConfiguration.threads))[0]));
		putenv(&(("MERIC_PWRCAP_POWER="+std::to_string(testRun[0].regConfiguration.powerCapPKG))[0]));
		putenv(&(("MERIC_PWRCAP_TIME="+std::to_string(testRun[0].regConfiguration.powerCapPKGtime))[0]));
		putenv(&(std::string("MERIC_COUNTERS=papi"))[0]);
		unsetenv("MERIC_REGION_OPTIONS");
	}
	else
	{
		const std::string MERIC_OUTPUT_FILENAME = MERIC_OUTPUT_DIR+"/MERICwrapperTestconfiguration.json";
		generateConfigFile(testRun, MERIC_OUTPUT_FILENAME);
		putenv(&(std::string("MERIC_FREQUENCY=0"))[0]);
		putenv(&(std::string("MERIC_UNCORE_FREQUENCY=0"))[0]);
		putenv(&(std::string("MERIC_NUM_THREADS=0"))[0]);
		putenv(&(std::string("MERIC_PWRCAP_POWER=0"))[0]);
		putenv(&(std::string("MERIC_PWRCAP_TIME=0"))[0]);
		putenv(&(std::string("MERIC_COUNTERS=papi"))[0]);
		putenv(&("MERIC_REGION_OPTIONS="+MERIC_OUTPUT_FILENAME)[0]);
	}

	system (APPLICATION.c_str());
	//TODO remove the test configuration file
}


int main()
{
	readParameters();
//	runDefault();
	appConfiguration defaultRun = {{ "", {0, 0, 0, 0, 0}, 0.0, 0.0 }};
	runApplication(defaultRun);

//state space
	constructCallPath();
	const systemLimits limit = constructStateSpace();

//analyzis	
	while (generateConfigurationsToTest(limit, EXHAUSTIVE))
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
	return 0;
}




















