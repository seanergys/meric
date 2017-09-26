/*
 * Tool for static CPU frequencies setting and energy measurement using RAPL counters via x86_adapt
 * 
 * This tool is based on MERIC raplwrapper and environmentwarpper
 *
 * 26.9.2017
 * Ondrej Vysocky
 * ondrej.vysocky@vsb.cz
 */

#include <unistd.h>
#include <iostream>
#include <fstream>
#include <x86_adapt.h>
#include <math.h>
#include <map>
#include <vector>
#include <string>
#include <time.h>
#include <limits>
#include <limits.h>
#include <iomanip>

/* RAPL description
 *
 * Intel_RAPL_Pckg_Energy - all energy consumtions
 * Intel_RAPL_PP0_Energy  - CPU
 * Intel_RAPL_PP1_Energy  - may reflect to uncore devices
 * Intel_RAPL_RAM_Energy  - RAM
 *
 * Intel_RAPL_Pckg_Energy ~ Intel_RAPL_PP0_Energy + Intel_RAPL_PP1_Energy
 * Intel_RAPL_Pckg_Energy ~ Intel_RAPL_PP0_Energy + Intel_RAPL_RAM_Energy
 * 
 *
 * MSR_RAPL_POWER_UNIT Register
 * Power Units (bits 3:0):
 	Power related information (in Watts) is based on the multiplier, 1/2^PU; where PU is an unsigned integer represented by bits 3:0. Default value is 0011b, indicating power unit is in 1/8 Watts 
increment.
 * Energy Status Units (bits 12:8): 
 	Energy related information (in Joules) is based on the multiplier, 1/2^ESU; where ESU is an unsigned integer represented by bits 12:8. Default value is 10000b, indicating energy status unit is in 15.3 micro-Joules increment.
 * Time Units (bits 19:16): 
 Time related information (in Seconds) is based on the multiplier, 1/ 2^TU; where TU is an unsigned integer represented by bits 19:16. Default value is 1010b, indicating time unit is in 976 micro-seconds increment.

TIME    ENERGY     POWER
1010|000|01110|0000|0011
9876 543 21098 7654 3210
*/

namespace RAPL_COUNTERS {

const std::string RAPL_PCKG_ENERGY = "Intel_RAPL_Pckg_Energy";
const std::string RAPL_RAM_ENERGY  = "Intel_RAPL_RAM_Energy";
const std::string RAPL_SUM_ENERGY  = "Energy summary";	//PCKG+RAM = not a counter to measure

const std::string RAPL_PP0_ENERGY  = "Intel_RAPL_PP0_Energy";	//Taurus doesn't support
const std::string RAPL_DRAM_CH0    = "Intel_DRAM_ENERGY_STATUS_CH0";
const std::string RAPL_DRAM_CH1    = "Intel_DRAM_ENERGY_STATUS_CH1";
const std::string RAPL_DRAM_CH2    = "Intel_DRAM_ENERGY_STATUS_CH2";
const std::string RAPL_DRAM_CH3    = "Intel_DRAM_ENERGY_STATUS_CH3";

}

const uint64_t RAPLpowerMask  = 0xF;
const uint64_t RAPLenergyMask = 0x1F00;
const uint64_t RAPLenergyShift= 8;
const uint64_t RAPLtimeMask   = 0xF0000;
const uint64_t RAPLtimeShift  = 16;

const std::string suffix = "_";	// set suffix you want
const int RAPL_BASELINE = 70;   // 70 W

const int X86ADAPT_FREQ_MULT = 256;
const int X86ADAPT_MAX_FREQ = 33;

static int SOCKET_SIZE;
std::string OUTPUT_FILE;

int RAPL_numOfDevices = 1;
long long RAPL_COUNTERMAX = pow(2,32);
std::map<std::string, long long> RAPL_counters;
std::map<std::string, double> RAPL_energyUnit;

////////////////////////////////////////////////////////////////////////////////
void EnvironmentInit(int CoreFreq, int UncoreFreq, int rank);
void EnvironmentClose();
double timeToDouble(const timespec &time);
void printHelp();

static void CPUCoreFrequency(uint64_t freq, int socket);
static void CPUUncoreFrequency(uint64_t freq, int socket);

void RAPL_init();
void RAPL_initMap(std::map<std::string, long long> & dict);
double RAPL_getValues(std::map<std::string, long long> & record);
void RAPL_cleanSystem();

#if defined(START)
void RAPL_storeValues(std::map<std::string, long long> &record, double time);
#endif
#if defined(STOP)
double RAPL_readStoredValues(std::map<std::string, long long> &oldRecord);
void RAPL_printValues(std::map<std::string, long long> &oldRecord, std::map<std::string, long long> &newRecord, double runtime);
unsigned long long RAPL_getResultValue(long long startValue, long long stopValue, std::string counter);
#endif

//----------------------------------------------------------------------------//
//--------------------------------------------------- ENERGY MEASUREMENT -----//
//----------------------------------------------------------------------------//
double timeToDouble(const timespec &time)
{
	return time.tv_sec + time.tv_nsec / 1e9;
}

//	set list of counters to measure
void RAPL_initMap(std::map<std::string, long long> & dict)
{
// fill this vector with counter you want to read
	std::vector <std::string> names;
	names.push_back(RAPL_COUNTERS::RAPL_PCKG_ENERGY);
	names.push_back(RAPL_COUNTERS::RAPL_RAM_ENERGY);
/*
RAPL_COUNTERS::RAPL_PP0_ENERGY	//Taurus doesn't support
RAPL_COUNTERS::RAPL_DRAM_CH0
RAPL_COUNTERS::RAPL_DRAM_CH1
RAPL_COUNTERS::RAPL_DRAM_CH2
RAPL_COUNTERS::RAPL_DRAM_CH3
*/

// RAPL gives us values for each cpu 
// this is a reason for adding a suffix with number of a socket for each counter

	for(int die = 0; die != RAPL_numOfDevices; die++)
	{
		for(std::vector<std::string>::iterator it = names.begin(); it != names.end(); ++it)
		{
			dict[*it + suffix + std::to_string(die)] = 0;
		}
	}
}

// set counters units
void RAPL_init()
{
	RAPL_numOfDevices = x86_adapt_get_nr_avaible_devices(X86_ADAPT_DIE);
	RAPL_initMap(RAPL_counters);

	//set unit modifier for Intel_RAPL_Pckg_Energy
	uint64_t unit;
	//ignoring the fact that different sockets may have different units
	const int fd = x86_adapt_get_device_ro(X86_ADAPT_DIE, 0);
	if (fd < 0)
	{
		std::cerr << "X86ADAPT RAPL INIT error while accessing file descriptor\n";
		std::cerr << "WARNING: RAPL values will be without any unit\n";
		exit(1);
	}
	const int itemId = x86_adapt_lookup_ci_name(X86_ADAPT_DIE, "Intel_RAPL_Power_Unit");
	x86_adapt_get_setting(fd, itemId, &unit);
	unit &= RAPLenergyMask;
	unit >>= RAPLenergyShift;
	if (x86_adapt_put_device(X86_ADAPT_DIE, 0))
	{
		std::cerr << "X86ADAPT RAPL INIT error while closing file descriptor\n";
		exit(1);
	}
	
	RAPL_energyUnit[RAPL_COUNTERS::RAPL_PCKG_ENERGY] = 1.0/pow(2,unit);
	//the rest unit modifiers are set according to x86_adapt "documentation"
	//https://github.com/tud-zih-energy/x86_energy/blob/eac130973b2fbfa0b5a3e72ef9912af617584d97/rapl.c#L763
	RAPL_energyUnit[RAPL_COUNTERS::RAPL_RAM_ENERGY] = 1.0/pow(2,16);
	RAPL_energyUnit[RAPL_COUNTERS::RAPL_DRAM_CH0] = 1.0/pow(2,16);	//1.0/pow(2,18)
	RAPL_energyUnit[RAPL_COUNTERS::RAPL_DRAM_CH1] = RAPL_energyUnit[RAPL_COUNTERS::RAPL_DRAM_CH0];
	RAPL_energyUnit[RAPL_COUNTERS::RAPL_DRAM_CH2] = RAPL_energyUnit[RAPL_COUNTERS::RAPL_DRAM_CH0];
	RAPL_energyUnit[RAPL_COUNTERS::RAPL_DRAM_CH3] = RAPL_energyUnit[RAPL_COUNTERS::RAPL_DRAM_CH0];

}

//	reads counters values
double RAPL_getValues(std::map<std::string, long long> & record)
{

	for(std::map<std::string, long long>::iterator iterator = RAPL_counters.begin(); iterator != RAPL_counters.end(); iterator++)
		record[iterator->first] = 0;
	
	for (int die=0; die<RAPL_numOfDevices; die++)
	{
		uint64_t dieEnergy = 0;
		const int fd = x86_adapt_get_device_ro(X86_ADAPT_DIE, die);
		if (fd < 0)
		{
			std::cerr << "X86ADAPT RAPL error while accessing file descriptor\n";
			exit(2);
		}
	
		for(std::map<std::string, long long>::iterator iterator = RAPL_counters.begin(); iterator != RAPL_counters.end(); iterator++)
		{
// this version doesn't count with more than 10 sockets per node (should be enough for this century)
			if ((iterator->first.back()-'0') != die)
			{
				continue;
			}
			std::string itemName = iterator->first.substr(0,iterator->first.length() - suffix.length() - 1);			
			const int itemId = x86_adapt_lookup_ci_name(X86_ADAPT_DIE, itemName.c_str());
			if (itemId < 0) //== -ENXIO
			{
				std::cerr << "X86ADAPT RAPL wrong counter name - " << itemName <<std::endl;
				continue;
			}
			if (!RAPL_energyUnit.count(itemName))
			{
				std::cerr << "X86ADAPT RAPL missing unit modifier for counter - " << itemName <<" Add modifier in RAPL::init()"<<std::endl;
				continue;					
			}
			x86_adapt_get_setting(fd, itemId, &dieEnergy);
			record [itemName+suffix+std::to_string(die)] = dieEnergy; // *RAPL_energyUnit[itemName];
		}
		if (x86_adapt_put_device(X86_ADAPT_DIE, die))
		{
			std::cerr << "X86ADAPT RAPL error while closing file descriptor\n";
			exit(4);
		}
	}
	
	timespec time;
	clock_gettime(CLOCK_REALTIME, &time);
	return timeToDouble(time);
}

void RAPL_cleanSystem()
{
	std::remove(OUTPUT_FILE.c_str());
}

#if defined(START)
void RAPL_storeValues(std::map<std::string, long long> &record, double time)
{
	//open file
	std::ofstream ss (OUTPUT_FILE, std::ofstream::out);
	
	//store time
	ss << std::setprecision(std::numeric_limits<double>::digits10 + 1) << time << std::endl;

	//store counters
	for(std::map<std::string, long long>::iterator iterator = record.begin(); iterator != record.end(); iterator++)
	{
		ss << iterator->first << '\t' << iterator->second << std::endl;
	}
	
	//close file
	ss.close();
}
#endif

#if defined(STOP)
// solve counter overflow and applies unit modifier
unsigned long long RAPL_getResultValue(long long startValue, long long stopValue, std::string counter)
{
	std::string counterName = counter.substr(0,counter.length() - suffix.length() - 1);
	
	if (stopValue < startValue)
	{	
		if (!RAPL_energyUnit.count(counterName))
		{
			//missing unit modifier
			return 0;
		}
		else
		{
			std::cerr << "RAPL OVERFLOW, MAX VALUE: " <<RAPL_COUNTERMAX << "*"<< RAPL_energyUnit[counterName] <<" " <<counterName << std::endl;
			std::cerr << "RAPL REPAIR " << startValue << " .. " << stopValue << " = " << ((RAPL_COUNTERMAX - startValue) + stopValue) * RAPL_energyUnit[counterName] << std::endl;
			return ((RAPL_COUNTERMAX - startValue) + stopValue) * RAPL_energyUnit[counterName];
		}
	}
	else
		return (stopValue - startValue) * RAPL_energyUnit[counterName];
}

double RAPL_readStoredValues(std::map<std::string, long long> & oldRecord)
{
	std::ifstream ss (OUTPUT_FILE);
	if (!ss)
	{
		std::cerr << "ERROR: running energyMeasureStop before energyMeasureStart!\n";
		exit(5);
	}

	std::string counter;
	unsigned long long value;
	double time;	//TODO nacteni casu ze souboru
	ss >> time ;
	
	while (ss >> counter >> value)
	{
		oldRecord[counter] = value;
	}

	ss.close();
	RAPL_cleanSystem();
	
	return time;
}

void RAPL_printValues(std::map<std::string, long long> &oldRecord, std::map<std::string, long long> &newRecord, double runtime)
{
	std::map<std::string, long long> record;
	unsigned long long summary = 0;
	std::cout << "Runtime =\t" << runtime << " s\n";
	for(std::map<std::string, long long>::iterator iterator = oldRecord.begin(); iterator != oldRecord.end(); iterator++)
	{
		unsigned long long value = RAPL_getResultValue(oldRecord[iterator->first], newRecord[iterator->first], iterator->first);
		std::cout << iterator->first << " =\t" << value << " J\n";
		summary += value;
	}
	summary += RAPL_BASELINE*runtime;
	std::cout << std::endl << RAPL_COUNTERS::RAPL_SUM_ENERGY << " =\t" << summary << " J\n";
	
	//print the summary energy also to the file
	std::ofstream ss (OUTPUT_FILE, std::ofstream::out);
	ss << summary << std::endl;
	ss.close();
	
}
#endif

//----------------------------------------------------------------------------//
//---------------------------------------------------------- ENVIRONMENT -----//
//----------------------------------------------------------------------------//
static void CPUCoreFrequency(uint64_t freq, int socket = -1)
{
	if (freq == 0)	//do not change frequency
		return;

	//x86adapt allows to set cpu frequency in range from 1.2 GHz to 3.3 GHz
	if (freq > 33 || freq < 12)	
	{
		std::cerr << "CPU frequency has not changed to invalid value: " << freq << std::endl;
		return;
	}

	if (x86_adapt_lookup_ci_name(X86_ADAPT_CPU, "Intel_Target_PState") < 0)
	{
		std::cerr << "X86ADAPT ERROR: frequency settings not allowed\n";
		return;
	}
	std::cout << "X86ADAPT set frequency to: " << freq;

	int nodeSize = x86_adapt_get_nr_avaible_devices(X86_ADAPT_CPU);
	int cpu=0;
	if(socket != -1)
	{
		nodeSize = SOCKET_SIZE*(socket+1);
		cpu = SOCKET_SIZE*socket;
	}
	std::cout << "; cpu " << cpu << " - " << nodeSize-1<< std::endl;
	
	for (; cpu<nodeSize; cpu++)
	{
		int fd = x86_adapt_get_device(X86_ADAPT_CPU, cpu);
		if (fd < 0)
		{
			std::cerr << "X86ADAPT ERROR: error while accessing file descriptor\n";
			exit(2);
		}

		if(x86_adapt_set_setting(fd, x86_adapt_lookup_ci_name(X86_ADAPT_CPU, "Intel_Target_PState"),freq*X86ADAPT_FREQ_MULT) != 8)
		{
			std::cerr << "X86ADAPT ERROR: error while setting core frequecy\n";
			exit(2);
		}

		if ( x86_adapt_put_device(X86_ADAPT_CPU, cpu))
		{
			std::cerr << "X86ADAPT ERROR: error while closing file descriptor\n";
			exit(2);
		}
	}
}

static void CPUUncoreFrequency(uint64_t freq, int socket = -1)
{
	if (freq == 0)
		return;

	if (freq > 30 || freq < 12)
	{
		std::cerr << "Uncore frequency has not changed to invalid value: " << freq << std::endl;
		return;
	}


	if (x86_adapt_lookup_ci_name(X86_ADAPT_DIE, "Intel_UNCORE_MAX_RATIO") < 0)
	{
		std::cerr << "X86ADAPT ERROR: uncore frequency settings not allowed\n";
		return;
	}

	std::cout << "X86ADAPT set uncore frequency to: "<< freq;
	int max = x86_adapt_get_nr_avaible_devices(X86_ADAPT_DIE);
	int die=0;
	if (socket != -1 && socket < max)
	{
		die = socket;
		max = socket+1;
	}
	std::cout << "; soc " << die << " - " << max-1<< std::endl;
	
	for (; die<max; die++)
	{
		int fd = x86_adapt_get_device(X86_ADAPT_DIE, die);
		if (fd < 0)
		{
			std::cerr << "X86ADAPT ERROR: error while accessing file descriptor\n";
			exit(3);
		}

		auto setFreq = [&] (const char * description)
		{
			if(x86_adapt_set_setting(fd, x86_adapt_lookup_ci_name(X86_ADAPT_DIE, description),freq ) != 8)
			{
				std::cerr << "X86ADAPT ERROR: error while setting uncore frequecy\n";
				exit(3);
			}
		};

		setFreq("Intel_UNCORE_MAX_RATIO");
		setFreq("Intel_UNCORE_MIN_RATIO");
	
		if ( x86_adapt_put_device(X86_ADAPT_DIE, die))
		{
			std::cerr << "X86ADAPT ERROR: error while closing file descriptor\n";
			exit(3);
		}
	}
}

void EnvironmentInit(int CoreFreq, int UncoreFreq, int rank = 0) //int system, 
{

	if (x86_adapt_init())
	{
		std::cerr << "X86ADAPT ERROR: init error\n";
		exit(1);
	}
	
	SOCKET_SIZE = x86_adapt_get_nr_avaible_devices(X86_ADAPT_CPU)/x86_adapt_get_nr_avaible_devices(X86_ADAPT_DIE);
//	systemType = system;
	
	if(!rank)
	{
		CPUCoreFrequency(CoreFreq);
		CPUUncoreFrequency(UncoreFreq);
	}
	
	char hostname[HOST_NAME_MAX];
	int result = gethostname(hostname, HOST_NAME_MAX);
	OUTPUT_FILE = "/scratch/temp/.MERICmeasure-" + std::string(hostname) +".data";

}

void EnvironmentClose()
{
	x86_adapt_finalize();
}

//----------------------------------------------------------------------------//
//----------------------------------------------------------------- MAIN -----//
//----------------------------------------------------------------------------//
void printHelp()
{
	std::cerr << "\nMERIC static tool help:\n\n"
	          << "Run energyMeasureStart to start the energy measurement\n"
	          << "Run energyMeasureStop to stop it and get the result energy at std::out\n"
	          << "It is possible to change CPU core and uncore frequencies using:\n"
	          << "\t-c = CPU core frequency in range 12 to 33 [0.1 GHz]\n"
	          << "\t-u = CPU uncore frequency in range 12 to 30 [0.1 GHz]\n"
	          << std::endl;
	exit(1);
}

int main(int argc, char **argv)
{
	int coreFreq = 0;
	int uncoreFreq = 0;
	for (int i = 1; i < argc; ++i)
	{
		std::string arg = argv[i];
		if (arg == "-c")
		{
			if (i + 1 < argc)
			{
				try
				{
					coreFreq = std::stoi(argv[++i]);
				} catch (std::exception const &e)
				{
					printHelp();
				}
			}else
			{
				printHelp();
			}
		}else if (arg == "-u")
		{
			if (i + 1 < argc)
			{
				try{
					uncoreFreq = std::stoul (argv[++i]);
				} catch (std::exception const &e)
				{
					printHelp();
				}
			}else
			{
				printHelp();
			}
		} else
		{
			printHelp();
		}
	}
/*
TODO
 - multinode measuremenet
*/
	EnvironmentInit(coreFreq, uncoreFreq);

	#if defined(START)
		RAPL_cleanSystem();
		RAPL_init();
		std::map<std::string, long long> record;
		double startTime = RAPL_getValues(record);
		RAPL_storeValues(record, startTime);
	#elif defined(STOP)
		RAPL_init();
		std::map<std::string, long long> old_record, new_record;
		double stopTime = RAPL_getValues(new_record);
		double startTime = RAPL_readStoredValues(old_record);
		RAPL_printValues(old_record, new_record, stopTime-startTime);
	#endif
	
	EnvironmentClose();
}






