
#include "environmentwrapper.h"

using namespace meric;

// LIST OF POSSIBLE frequencies for JETSON CPUs (kHz)
//	102000 204000 307200 403200 518400 614400 710400 825600 921600 1036800 1132800 1224000 1326000 1428000 1555500 1632000 1734000
// LIST OF POSSIBLE frequnecies for JETSON memory (kHz)
//	40800 68000 102000 204000 408000 665600 800000 1065600 1331200 1600000


systemLimits Environment::FreqLimits [] = {
	[SYS_HASWELL] = { 33, 25, 12, 30, 30, 12 },
	[SYS_JETSON]  = { 1734000, 518400, 102000, 1600000, 408000, 40800}
};

int Environment::systemType = SYS_HASWELL;

void Environment::threadNumber(int nthreads)
{
	if (nthreads <= 0)
		return;
#ifdef HAVE_OMP_H
	if (nthreads != omp_get_num_threads())
	{
		if (!omp_in_parallel())
		{
			MERIC_INFO << "OPENMP set num threads to: " << nthreads << std::endl;
			omp_set_num_threads(nthreads);
		}
		else
		{
			MERIC_INFO << "MERIC WARNING: paralel region, num threads did not change to: " << nthreads << std::endl;
		}
	}
#endif
/*
#ifdef HAVE_CILK_CILK_API_H
	//using cilk
	__cilkrts_end_cilk();  
	__cilkrts_set_param("nworkers", std::to_string(nthreads).c_str());
#endif
*/
}

void Environment::init(int system, RegionParameters defaultParameter, int rank)
{
#ifdef HAVE_X86_ADAPT_H
	if (x86_adapt_init())
	{
		std::cerr << "X86ADAPT ERROR: init error\n";
//		exit(1);
	}
	
	//test if it is possible to set uncore frequency
//		if (x86_adapt_lookup_ci_name(X86_ADAPT_DIE, "Intel_UNCORE_MAX_RATIO") < 0)
//			std::cerr << "X86ADAPT WARNING: UNCORE FREQUENCY SETTINGS IS NOT AVAILABLE\n";

	SOCKET_SIZE = x86_adapt_get_nr_avaible_devices(X86_ADAPT_CPU)/x86_adapt_get_nr_avaible_devices(X86_ADAPT_DIE);
#else
	if (system != SYS_JETSON)	//TODO - this is just fast fix
	{
		std::string line;
		std::ifstream jsonFile ("/proc/cpuifo");
		if (jsonFile.is_open())
		{
			while (getline(jsonFile,line))
			{
				if (line.find("cpu cores"))
				{
					line = line.substr(line.find(":"));
					break;
				}
			}
			jsonFile.close();
			SOCKET_SIZE = std::stoi(line);
		}
	}
#endif	
	systemType = system;
	
	if (systemType == SYS_JETSON)
	{
		std::ofstream governorFile;
		//core
		for (int i=0; i<4; i++)	//TODO 4 cores - how to detect? test if code will not fall if SOCKET_SIZE will be set to 4
		{
			governorFile.open ("/sys/devices/system/cpu/cpu"+std::to_string(i)+"/cpufreq/scaling_governor");
			governorFile << "userspace";
			governorFile.close();
		}
		//uncore
		governorFile.open ("/sys/kernel/debug/clock/override.emc/state");
		governorFile << "1";
		governorFile.close();
	}
	
	if(!rank)
	{
		frequency(defaultParameter.frequency);
		uncoreFrequency(defaultParameter.uncoreFrequency);
	}
	threadNumber(defaultParameter.nthreads);
}


void Environment::setDefaultFrequencies()
{
	frequency(FreqLimits[systemType].defaultCoreFreq);
	uncoreFrequency(FreqLimits[systemType].defaultUncoreFreq);

	if (systemType == SYS_JETSON)
	{
		std::ofstream governorFile;
		//core
		for (int i=0; i<4; i++)	//TODO 4 cores - how to detect?
		{
			governorFile.open ("/sys/devices/system/cpu/cpu"+std::to_string(i)+"/cpufreq/scaling_governor");
			governorFile << "ondemand";
			governorFile.close();
		}
		//uncore
		governorFile.open ("/sys/kernel/debug/clock/override.emc/state");
		governorFile << "0";
		governorFile.close();
	}
}

void Environment::close()
{
#ifdef HAVE_X86_ADAPT_H
	x86_adapt_finalize();
#endif
}


