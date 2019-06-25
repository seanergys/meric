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
#include <math.h>
#include <sys/stat.h>

//const std::string APPLICATION = "mpirun -n 2 /home/vys0053/readex/meric/test/test_mpi";
const bool MPI_APP = true;
//const std::string APPLICATION="/galileo/home/userexternal/ovysocky/meric/test/test";
//const std::string APPLICATION="srun --cpu_bind=verbose,sockets --nodes 1 --ntasks-per-node 2 --cpus-per-task 12 /galileo/home/userexternal/ovysocky/pool/blasbench/blasbench/blasbench_meric --dgemv 300,8192 --dgemm 150,2048 --tan 60000 --phase 2";
const std::string APPLICATION="mpirun $BIND_TO_SOCKETS /scratch/work/user/vys0053/SOFTWARE/readex-apps/readex-repository/benchmark_apps/blasbench/blasbench_meric --dgemv 300,8192 --dgemm 150,2048 --tan 60000 --phase 2";
std::string MERIC_OUTPUT_DIR;

// STRUCTS /////////////////////////////////////////////////////////////////////

enum ALGORITHM {EXHAUSTIVE=0, EVO=1, PSO=2, NONE};

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
systemLimits limit;

struct configuration{	// single HW configuration
	int  threads;
	long coreFreq;
	long uncoreFreq;
	long powerCapPKG;
	long powerCapPKGtime;
	
	configuration() : 
		threads(0), coreFreq(0), uncoreFreq(0), powerCapPKG(0), powerCapPKGtime(0) {}
	configuration(int t, long c, long u, long p, long pt) : 
		threads(t), coreFreq(c), uncoreFreq(u), powerCapPKG(p), powerCapPKGtime(pt) {}
	
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
	
	configuration operator-(const configuration& c) const
	{
		configuration dis = {
			c.threads - this->threads,
			c.coreFreq - this->coreFreq,
			c.uncoreFreq - this->uncoreFreq,
			c.powerCapPKG - this->powerCapPKG,
			c.powerCapPKGtime - this->powerCapPKGtime};
		return dis;
	}

	configuration operator+(const configuration& c) const
	{
		configuration dis = {
			c.threads + this->threads,
			c.coreFreq + this->coreFreq,
			c.uncoreFreq + this->uncoreFreq,
			c.powerCapPKG + this->powerCapPKG,
			c.powerCapPKGtime + this->powerCapPKGtime};
		return dis;
	}
};

configuration selectedParams; //which parameteres will be tuned

struct regionConfiguration{ //configuration for a region
	std::string regName;
	struct configuration regConfiguration;
	double energy;
	double runtime;

	bool operator<(const regionConfiguration& c) const
	{
		return (c.energy < this->energy);
	}
};

typedef std::vector <regionConfiguration> appConfiguration; //configurations for the application's regions

appConfiguration optimalConfiguration;
std::vector <appConfiguration> testConfigurationsToTry;
std::vector <appConfiguration> testConfigurationsDone;
unsigned long newconfigurations;

std::vector <std::string> appRegions; //list of regions of the application

//EXHAUSTIVE
//EVO + PSO
	int improvedSolutionCounter = 0;
//PSO
	const double Cp = 2.05;
	const double Cg = 2.05;
	const double omg = 0.9;
	struct PSOparticleHistory{
		struct configuration bestPosition;
		double energy;
		double runtime;
		struct configuration velocity;
				
		PSOparticleHistory() :
			bestPosition(), energy(std::numeric_limits<double>::max()), runtime(0.0), velocity() {}
		
		bool operator>(const regionConfiguration& c) const
		{
			return (c.energy > this->energy);
		}
		
		bool operator<(const regionConfiguration& c) const
		{
			return (c.energy < this->energy);
		}
	};

	std::vector<std::vector<PSOparticleHistory>> PSOhistory;

// FUNCTIONS ///////////////////////////////////////////////////////////////////

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

void printHelp()
{
	std::cerr << "MERICwrapper" <<std::endl
	          << "select from an algorithm to search for the optimal configuration" << std::endl
	          << "\t-x = exhaustive" << std::endl
	          << "\t-e = evolution" << std::endl
	          << "\t-p = particle swarm optimization" << std::endl;
}

ALGORITHM readParameters(int argc, char **argv)
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
	ALGORITHM alg = NONE;
	if (argc == 1)
		printHelp();
	for (int i = 1; i < argc; ++i)
	{
		std::string arg = argv[i];
		if (arg == "-x")
			alg = EXHAUSTIVE;
		else if (arg == "-e")
			alg = EVO;
		else if (arg == "-p")
			alg = PSO;
		else
		{
			printHelp();
		}
	}

	struct timespec timeNow;
	clock_gettime(CLOCK_REALTIME, &timeNow);
	MERIC_OUTPUT_DIR = "MERICwrapper-"+std::to_string(timeNow.tv_sec);
	setenv("MERIC_OUTPUT_DIR", MERIC_OUTPUT_DIR.c_str(), 1);

	setenv("MERIC_COUNTERS", "papi", 1);
	setenv("MERIC_MODE", "1", 1);
	setenv("MERIC_ITERATION", "0", 1);
	
	return alg;
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

void constructStateSpace()
{
	//TODO code based 
// limit = {12, 2, 2500000000, 2300000000, 3000000000, 2800000000, 0, 0, 0, 0, 0, 0};

limit = {12, 2, 2500000000, 1200000000, 3000000000, 1200000000, 0, 0, 0, 0, 0, 0}; //Salomon
//	limit = {18, 2, 2300000000, 1200000000, 2800000000, 1200000000, 0, 0, 0, 0, 0, 0}; //Galileo

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
}

configuration decodeConfigVector (std::vector<long> values)	//EXHAUSTIVE only
{
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

std::string constructConfigFileName(std::string regName, configuration config)
{
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

	if (MPI_APP)
	{
		char hostname[HOST_NAME_MAX];
		gethostname(hostname, HOST_NAME_MAX);
		cfgStr = std::string(hostname) +"_"+ cfgStr;
	}
	
	return MERIC_OUTPUT_DIR+"/"+regName+"/"+cfgStr+".csv";
}


bool testIfConfigExists(std::string regName, configuration c, bool searchInFinished = false) //EVO only
{
	bool find = false;
	if (searchInFinished)
	{
		struct stat buffer;
		find = (stat(constructConfigFileName(regName, c).c_str(), &buffer) == 0); 
	}

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

configuration getRandomConfig()
{
	configuration c;// = {0, 0, 0, 0, 0};
	if (selectedParams.uncoreFreq)
	{
		int max = (limit.maxUncoreFreq - limit.minUncoreFreq)/100000000; //TODO step size
		long val = rand() % max;
		c.uncoreFreq = limit.minUncoreFreq + val*100000000;
	}
	if (selectedParams.coreFreq)
	{
		int max = (limit.maxCoreFreq - limit.minCoreFreq)/100000000; //TODO
		long val = rand() % max;
		c.coreFreq = limit.minCoreFreq + val * 100000000;
	}
	if (selectedParams.threads)
	{
		c.threads = rand() % (limit.coresPerSocket * limit.socketsPerNode)+1; //TODO
	}
/*	//TODO
	if (selectedParams.powerCapPKG)
		...
	if (selectedParams.powerCapPKGtime)
		...
*/

//	std::cout << "GENcfg: " << c.coreFreq << " " << c.uncoreFreq << " " << c.threads << std::endl;
	return c;
}


//UNIFORM crossover
configuration EVO_crossover(configuration c1, configuration c2)
{
	configuration conf;
	if (rand()%10 < 5)
		conf.uncoreFreq = c1.uncoreFreq;
	else
		conf.uncoreFreq = c2.uncoreFreq;

	if (rand()%10 < 5)
		conf.coreFreq = c1.coreFreq;
	else
		conf.coreFreq = c2.coreFreq;
	
	if (rand()%10 < 5)
		conf.threads = c1.threads;
	else
		conf.threads = c2.threads;

	if (rand()%10 < 5)
		conf.powerCapPKG = c1.powerCapPKG;
	else
		conf.powerCapPKG = c2.powerCapPKG;

	if (rand()%10 < 5)
		conf.powerCapPKG = c1.powerCapPKG;
	else
		conf.powerCapPKG = c2.powerCapPKG;		

	return conf;
}

void EVO_mute(configuration& conf)	//TODO stepsize
{
	int director;
	if (selectedParams.uncoreFreq)
	{
		director = rand()%100;
		if (director < 10)
		{
			if (director < 5 && conf.uncoreFreq < limit.maxUncoreFreq)
				conf.uncoreFreq += 100000000;
			else
			{
				if (conf.uncoreFreq > limit.minUncoreFreq)
					conf.uncoreFreq -= 100000000;
				else
					conf.uncoreFreq += 100000000;
			}
		}	
	}
	if (selectedParams.coreFreq)
	{
		director = rand()%100;
		if (director < 10)
		{
			if (director < 5 && conf.coreFreq < limit.maxCoreFreq)
				conf.coreFreq += 100000000;
			else
			{
				if (conf.coreFreq > limit.minCoreFreq)
					conf.coreFreq -= 100000000;
				else
					conf.coreFreq += 100000000;
			}
		}
	}
	if (selectedParams.threads)
	{
		director = rand()%100;
		if (director < 10)
		{
			if (director < 5 && conf.threads < limit.coresPerSocket * limit.socketsPerNode)
				conf.threads += 1;
			else
			{
				if (conf.threads > 2)
					conf.threads -= 1;
				else
					conf.threads += 1;
			}
		}
	}
	//TODO RAPL
}

void limitVelocity(configuration &c)
{
	if (c.uncoreFreq > 200000000)
		c.uncoreFreq = 200000000;
	else if(c.uncoreFreq < -200000000)
		c.uncoreFreq = -200000000;
	
	if (c.coreFreq > 200000000)
		c.coreFreq = 200000000;
	else if (c.coreFreq < -200000000)
		c.coreFreq = -200000000;

	if (c.threads > 2)
		c.threads = 2;
	else if (c.threads < -2)
		c.threads = -2;
			
	//TODO RAPL
}

configuration PSO_getNewVelocity(configuration v_k_old, configuration x_k_old, configuration best_k, configuration best_swarm)
{
	//double Rp = (rand()%100) / 100.0;
	//double Rg = (rand()%100) / 100.0;
	//v_k_new = omg * v_k_old + Cp*Rp*(best_k - x_k_old) + Cg*Rg*(best_swarm - x_k_old)
	//x_k_new = x_k_old + v_k_new
	configuration c;

	if (selectedParams.uncoreFreq)
	{
		c.uncoreFreq = (((long)(omg * v_k_old.uncoreFreq
			+ Cp*((rand()%100)/100.0) * (best_k.uncoreFreq - x_k_old.uncoreFreq)
			+ Cg*((rand()%100)/100.0) * (best_swarm.uncoreFreq - x_k_old.uncoreFreq)
			)) / 100000000) * 100000000;
	}
	if (selectedParams.coreFreq)
	{
		c.coreFreq = ((long)(omg * v_k_old.coreFreq
			+ Cp*((rand()%100)/100.0) * (best_k.coreFreq - x_k_old.coreFreq)
			+ Cg*((rand()%100)/100.0) * (best_swarm.coreFreq - x_k_old.coreFreq)
			) / 100000000) * 100000000;
	}
	if (selectedParams.threads)
	{
		c.threads = (omg * v_k_old.threads
			+ Cp*((rand()%100)/100.0) * (best_k.threads - x_k_old.threads)
			+ Cg*((rand()%100)/100.0) * (best_swarm.threads - x_k_old.threads)
			);
	}

	//TODO RAPL
	//if (selectedParams.powerCapPKG){}
	//if (selectedParams.powerCapPKGtime) {}

	limitVelocity(c);
	return c;
}

bool PSO_configurationValidation(configuration c)
{
	if (c.uncoreFreq < limit.minUncoreFreq || c.uncoreFreq > limit.maxUncoreFreq
	|| c.coreFreq < limit.minCoreFreq || c.coreFreq > limit.maxCoreFreq
	|| c.threads < 1 || c.threads > (limit.coresPerSocket * limit.socketsPerNode)
	|| c.powerCapPKG < limit.minPowerCapPKG || c.powerCapPKG > limit.maxPowerCapPKG
	|| c.powerCapPKGtime < 0 || c.powerCapPKGtime > limit.maxPowerCapPKGtime)
		return false;
	else
		return true;
}

//EVO - EXHAUSTIVE - PSO
size_t generateConfigurationsToTest(ALGORITHM alg)
{
/// EVO + PSO //////////////////////////////////////////////////////////////////
	const int POPULATION_SIZE = 10;	//EVO 10, PSO 10-20-30-40
	const int END_CONDITION = 3; //-times the best configuration has not been improved
////////////////////////////////////////////////////////////////////////////////

	//TODO take also into acount that all the possible configurations had been tested, so it will cause a deadlock in generation of new configurations

	//identify so-far the best configurations - exhaustive
	//currently default behavior is not evaluated
	if (newconfigurations != 0)
	{
		improvedSolutionCounter++;
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

		auto compareWithTheBest = [&] (std::string regName, configuration config)
		{
			size_t regIndex = getRegionIndex(regName);
			//open file that has measurement results for the region in the specified configuration
			double energyConsumption = 0.0;
			std::ifstream inputfile (constructConfigFileName(regName, config));
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
					size_t position = element.find(label);
					if (position != std::string::npos)
					{
						energyConsumption = std::stod(element.substr(label.size()+1+position));
						//we have found better solution
						if (energyConsumption < optimalConfiguration[regIndex].energy)
						{
							optimalConfiguration[regIndex].energy = energyConsumption;
							optimalConfiguration[regIndex].regConfiguration = config;
							improvedSolutionCounter = 0;
std::cerr << regName << " - improved solution\n";
						}

						break; //TODO just the first one
					}
				}
				inputfile.close();
			}
			return energyConsumption;
		};

std::cerr << "evaluation of the finished runs\n";
	
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
			i++;
		}
	}

	//RANDOM INITIAL GENERATION
	auto generateRandomGeneration = [&] ()
	{
		configuration v;// = {0, 0, 0, 0, 0};
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
		
		while (testConfigurationsToTry.size() < POPULATION_SIZE)
		{
			appConfiguration ac;
			//for (std::string regName : appRegs)
			std::string regName = "";
			{
				configuration c = getRandomConfig();
				while (testIfConfigExists(regName, c))
					c = getRandomConfig();
				ac.push_back({regName, c, std::numeric_limits<double>::max(), 0.0});
			}
			testConfigurationsToTry.push_back(ac);
		}
	};

/// PSO ////////////////////////////////////////////////////////////////////////	
	if (alg == PSO)
	{
		if (!testConfigurationsDone.size())	//INITIAL GENERATION
		{
			generateRandomGeneration();
			PSOhistory.resize(appRegions.size());
			//for (int idx=0; idx<appRegions.size(); idx++)
			for (std::string regName : appRegions)
			{
				size_t idx = getRegionIndex(regName);
				PSOhistory[idx].resize(POPULATION_SIZE);
				for (int i = 0; i<POPULATION_SIZE; i++)
				{
					PSOhistory[idx][i].energy = std::numeric_limits<double>::max();
					PSOhistory[idx][i].velocity = getRandomConfig() - getRandomConfig();
					limitVelocity(PSOhistory[idx][i].velocity);
				}
			}
		}
		else if (improvedSolutionCounter < END_CONDITION)
		{
				//update particle history
			for (size_t i = 0; i<testConfigurationsDone.size(); i++)
			{
				for (size_t j = 0; j < testConfigurationsDone[i].size(); j++)
				{
					if (testConfigurationsDone[i][j].regName != "")
					{
						size_t regIDX = getRegionIndex(testConfigurationsDone[i][j].regName);
						if (PSOhistory[regIDX][i].energy > testConfigurationsDone[i][j].energy)
						{
							PSOhistory[regIDX][i].bestPosition = testConfigurationsDone[i][j].regConfiguration;
							PSOhistory[regIDX][i].energy = testConfigurationsDone[i][j].energy;
							PSOhistory[regIDX][i].runtime = testConfigurationsDone[i][j].runtime;
						}			
					}
				}
			}//the best position must be evaluated before we set new position

					//set new position
			for (size_t i = 0; i < POPULATION_SIZE; i++)
			{
				appConfiguration ac;
				for (std::string regName : appRegions)
				{
					size_t regIdx = getRegionIndex(regName);
					configuration x_k_new;
/*					configuration v_k_new;
					configuration v_k_old = PSOhistory[regIdx][i].velocity;
					configuration x_k_old = testConfigurationsDone[i][<regIndex>].regConfiguration;
					configuration best_k = PSOhistory[regIdx][i].bestPosition;
					configuration best_swarm = optimalConfiguration[regIndex].regConfiguration;
					double Rp = (rand()%100) / 100.0;
					double Rg = (rand()%100) / 100.0;
					//v_k_new = omg * v_k_old + Cp*Rp*(best_k - x_k_old) + Cg*Rg*(best_swarm - x_k_old)
					//x_k_new = x_k_old + v_k_new
*/
					for (auto x_k_old : testConfigurationsDone[i])
					{
						if (x_k_old.regName == regName)
						{
							PSOhistory[regIdx][i].velocity = PSO_getNewVelocity(PSOhistory[regIdx][i].velocity, x_k_old.regConfiguration, PSOhistory[regIdx][i].bestPosition, optimalConfiguration[regIdx].regConfiguration);
							x_k_new = x_k_old.regConfiguration + PSOhistory[regIdx][i].velocity;

							if (!PSO_configurationValidation(x_k_new))
								x_k_new = getRandomConfig();
							break;
						}
					}
					ac.push_back({ regName, x_k_new, std::numeric_limits<double>::max(), 0.0 });
				}
				testConfigurationsToTry.push_back(ac);
			}
			testConfigurationsDone.clear(); //only PSO removes old configurations
		}
	}
/// EVO ////////////////////////////////////////////////////////////////////////
	else if (alg == EVO)
	{
		if (!testConfigurationsDone.size())	//INITIAL GENERATION
			generateRandomGeneration();
		else if (improvedSolutionCounter < END_CONDITION) //EVERY OTHER GENERATION of SOLUTIONS
		{
			std::vector <std::list<regionConfiguration>> population;
			population.resize(appRegions.size());

			//associate the value (energy) with each member of the population
			//each region has its own population
			for (size_t i = testConfigurationsDone.size()-newconfigurations; i<testConfigurationsDone.size(); i++)
			{
				for (size_t zz = 0; zz < testConfigurationsDone[i].size(); zz++)
				{
					if (testConfigurationsDone[i][zz].regName != "")
					{
						population[getRegionIndex(testConfigurationsDone[i][zz].regName)].push_back(testConfigurationsDone[i][zz]);
					}
				}
			}
			
			testConfigurationsToTry.resize(POPULATION_SIZE);
			for (std::string regName : appRegions)
			{
				size_t regIndex = getRegionIndex(regName);
				auto pop = population[regIndex];
				//select members for crossover - 50%
				//worse solutions will be removed with higher propability
				pop.sort();
				for (int i = 0; i<POPULATION_SIZE/2; i++)
				{
					std::list<regionConfiguration>::iterator it = pop.begin();
					size_t removeIndex = floor(sqrt(rand()%(int)pow(pop.size(),2.0)));
					for (int j=0;j<removeIndex;j++)
						it++;
					pop.erase(it);
				}
				//we add the best configuration so-far - elitism 
				//might be there twice - if region's optimalConfiguration was updated from this population and if this member was not removed during the russian rulette
				pop.push_back(optimalConfiguration[regIndex]);

				//generate new population				
				for (int i = 0; i<POPULATION_SIZE; i++)
				{
					//select 2 parents
					size_t parent1 = rand() % pop.size();
					size_t parent2 = rand() % pop.size();
					std::list<regionConfiguration>::iterator it1, it2;
					it1 = it2 = pop.begin();

					for (int j=0;j<parent1;j++)
						it1++;
					for (int j=0;j<parent2;j++)
						it2++;

					//crossover
					configuration newConf = EVO_crossover(it1->regConfiguration, it2->regConfiguration);

					//mutation - test if the configuration haven't been tested yet
					while (testIfConfigExists(regName, newConf, true))
					{
						EVO_mute(newConf);
					}

					//push them to testConfigurationsToTry
					testConfigurationsToTry[i].push_back({regName, newConf, std::numeric_limits<double>::max(), 0.0});
				}
			}
		}
	}
	
/// EXHAUSTIVE /////////////////////////////////////////////////////////////////
	else if (alg == EXHAUSTIVE && testConfigurationsDone.size() == 0)
	{
		std::vector <std::string> parameterName;
		std::vector <std::vector<long>> exhConf;
		std::vector <long> cfg;
		//threads
		for (long t = limit.coresPerSocket * limit.socketsPerNode; t >= 1; t--) //TODO stepSize
//		for (long t = limit.coresPerSocket * limit.socketsPerNode; t >= 1; t-= limit.coresPerSocket) //TODO stepSize
		{
			cfg.push_back(t);
		}
		if (cfg.size() != 0)
			exhConf.push_back(cfg);
		cfg.clear();

		//CF
		for (long c = limit.maxCoreFreq; c >= limit.minCoreFreq; c-= 100000000) //TODO
//		for (long c = limit.maxCoreFreq; c >= limit.minCoreFreq; c -= limit.maxCoreFreq-limit.minCoreFreq) //TODO
		{
			cfg.push_back(c);
		}
		if (cfg.size() != 0)
			exhConf.push_back(cfg);
		cfg.clear();

		//UCF
		for (long u = limit.maxUncoreFreq; u >= limit.minUncoreFreq; u-=100000000) //TODO
//		for (long u = limit.maxUncoreFreq; u >= limit.minUncoreFreq; u -= limit.maxUncoreFreq-limit.minUncoreFreq) //TODO
		{
			cfg.push_back(u);
		}
		if (cfg.size() != 0)
			exhConf.push_back(cfg);
		cfg.clear();
/*
		//TODO RAPL
*/

		std::vector <long> v;
		EXHAUSTIVE_generateConfigurationInRecursive(exhConf.size(), exhConf, v);
	}
////////////////////////////////////////////////////////////////////////////////
std::cerr << "improvedSolutionCounter = " << improvedSolutionCounter << std::endl;

	newconfigurations = 0;
	return testConfigurationsToTry.size();	//this should be the only return statement in this function
}


void runApplication(appConfiguration testRun)
{

	//testRun.size() == 0 ??
	
	if (testRun.size() == 1 && testRun[0].regName == "")
	{
		std::cerr << "Running: C" <<  testRun[0].regConfiguration.coreFreq << " U" << testRun[0].regConfiguration.uncoreFreq << " T" << testRun[0].regConfiguration.threads << std::endl;

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

//could be set if the behavior will change in MERIC, that will alow the tuning when the static parameters not set and just config file provided
		if (selectedParams.uncoreFreq)
			setenv("MERIC_UNCORE_FREQUENCY", std::to_string(limit.maxUncoreFreq).c_str(), 1);
		else
			setenv("MERIC_UNCORE_FREQUENCY", "0", 1);
			
		if (selectedParams.coreFreq)
			setenv("MERIC_FREQUENCY", std::to_string(limit.maxCoreFreq).c_str(), 1);
		else
			setenv("MERIC_FREQUENCY", "0", 1);
			
		if (selectedParams.threads)
			setenv("MERIC_NUM_THREADS", std::to_string(limit.coresPerSocket * limit.socketsPerNode).c_str(), 1);
		else
			setenv("MERIC_NUM_THREADS", "0", 1);
			
		if (selectedParams.powerCapPKG)
			setenv("MERIC_PWRCAP_POWER", std::to_string(limit.maxPowerCapPKG).c_str(), 1);
		else
			setenv("MERIC_PWRCAP_POWER", "0", 1);
			
		if (selectedParams.powerCapPKGtime)
			setenv("MERIC_PWRCAP_TIME", std::to_string(limit.maxPowerCapPKGtime).c_str(), 1);
		else
			setenv("MERIC_PWRCAP_TIME", "0", 1);

		setenv("MERIC_REGION_OPTIONS", MERIC_OUTPUT_FILENAME.c_str(), 1);

		std::cerr << "Running: dynamic configuration - " << MERIC_OUTPUT_FILENAME << std::endl;
	}

//	system ("printenv");
	system (APPLICATION.c_str());
	//TODO remove the test configuration file
}


int main(int argc, char **argv)
{
	constructStateSpace();
	ALGORITHM alg = readParameters(argc, argv);
	if (alg == NONE)
		return 1;
	appConfiguration defaultRun = {{ "", {0, 0, 0, 0, 0}, std::numeric_limits<double>::max(), 0.0 }};
	runApplication(defaultRun);
	constructCallPath();

//analyzis
	std::cerr << "RUN ANALYSIS\n";
	while (generateConfigurationsToTest(alg))
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
	std::cerr << MERIC_OUTPUT_DIR+"/MERICoptimalConfiguration.json" << std::endl;
	return 0;
}




















