
#include "environmentwrapper.h"

using namespace meric;

// LIST OF POSSIBLE frequencies for JETSON CPUs (kHz)
//	102000 204000 307200 403200 518400 614400 710400 825600 921600 1036800 1132800 1224000 1326000 1428000 1555500 1632000 1734000
// LIST OF POSSIBLE frequnecies for JETSON memory (kHz)
//	40800 68000 102000 204000 408000 665600 800000 1065600 1331200 1600000


systemLimits Environment::FreqLimits [] = {
	[SYS_HASWELL] = { 33, 25, 12, 30, 30, 12 },
	[SYS_JETSON]  = { 1734000, 518400, 102000, 1600000, 408000, 40800},
	[SYS_THUNDER] = { -1, -1, -1, -1, -1, -1}, //DVFS not supported
	[SYS_OTHER] = { -1, -1, -1, -1, -1, -1}
};

int Environment::NODE_SIZE_CPU = 0;
int Environment::NODE_SIZE_SOCKET = 0;
int Environment::SOCKET_SIZE_CPU = 0;
int Environment::systemType = SYS_HASWELL;
bool Environment::usedDVFS = false;
bool Environment::usedUFS = false;

void Environment::threadNumber(int nthreads)
{
	timespec start_time, stop_time;
	clock_gettime(CLOCK_REALTIME, &start_time);

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
			MERIC_INFO << "MERIC WARNING: parallel region, num threads did not change to: " << nthreads << std::endl;
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
	clock_gettime(CLOCK_REALTIME, &stop_time);
	MERIC_INFO << "MERIC number of OpenMP threads changed in " << mutils::timeToDouble(stop_time - start_time)*1000 << " ms\n";
}


void Environment::initHWSizes()
{
#ifdef HAVE_X86_ADAPT_H // and adapt must be initialized which makes it 
	NODE_SIZE_CPU = x86_adapt_get_nr_avaible_devices(X86_ADAPT_CPU);
	NODE_SIZE_SOCKET = x86_adapt_get_nr_avaible_devices(X86_ADAPT_DIE);
#else
	NODE_SIZE_CPU = sysconf(_SC_NPROCESSORS_ONLN);
	NODE_SIZE_SOCKET = numa_node_of_cpu(NODE_SIZE_CPU-1)+1;
	if (NODE_SIZE_SOCKET <= 0)
		NODE_SIZE_SOCKET=1;
#endif
	SOCKET_SIZE_CPU = NODE_SIZE_CPU / NODE_SIZE_SOCKET;
	MERIC_INFO << "CPUs " << NODE_SIZE_CPU << " SOCKETS " << NODE_SIZE_SOCKET << std::endl;
}

void Environment::getFrequenciesLimits()
{
	//CORE freq
	std::ifstream frequencyBounderyFile;
	std::string freqBound;
	frequencyBounderyFile.open ("/sys/devices/system/cpu/cpu0/cpufreq/cpuinfo_max_freq");
	frequencyBounderyFile >> freqBound;
	frequencyBounderyFile.close();
	FreqLimits[SYS_OTHER].maxCoreFreq = std::stoi(freqBound);
	FreqLimits[SYS_OTHER].defaultCoreFreq = FreqLimits[SYS_OTHER].maxCoreFreq / CPUFREQ_FREQ_MULT;
	FreqLimits[SYS_OTHER].maxCoreFreq = FreqLimits[SYS_OTHER].defaultCoreFreq + (FreqLimits[SYS_OTHER].maxCoreFreq % CPUFREQ_FREQ_MULT ? 1 : 0);
	frequencyBounderyFile.open ("/sys/devices/system/cpu/cpu0/cpufreq/cpuinfo_min_freq");
	frequencyBounderyFile >> freqBound;
	frequencyBounderyFile.close();
	FreqLimits[SYS_OTHER].minCoreFreq = std::stoi(freqBound) / CPUFREQ_FREQ_MULT;

	//UnCORE freq
#ifdef HAVE_X86_ADAPT_H
	int fd = x86_adapt_get_device_ro(X86_ADAPT_DIE, 0);
	int adaptHandlerUncore = x86_adapt_lookup_ci_name(X86_ADAPT_DIE, "Intel_UNCORE_MAX_RATIO");
	uint64_t setting;
	x86_adapt_get_setting(fd, adaptHandlerUncore, &setting);
	FreqLimits[SYS_OTHER].maxUncoreFreq = (int)setting;
	FreqLimits[SYS_OTHER].defaultUncoreFreq = FreqLimits[SYS_OTHER].maxUncoreFreq;
	adaptHandlerUncore = x86_adapt_lookup_ci_name(X86_ADAPT_DIE, "Intel_UNCORE_MIN_RATIO");
	x86_adapt_get_setting(fd, adaptHandlerUncore, &setting);
	FreqLimits[SYS_OTHER].minUncoreFreq = (int)setting;
#endif	
	MERIC_INFO << "CORE FREQ limits: " << FreqLimits[SYS_OTHER].maxCoreFreq << '\t' << FreqLimits[SYS_OTHER].minCoreFreq<< std::endl;
	MERIC_INFO << "UNCORE FREQ limits: " << FreqLimits[SYS_OTHER].maxUncoreFreq << '\t' << FreqLimits[SYS_OTHER].minUncoreFreq<< std::endl;
}

void Environment::init(int system, RegionParameters defaultParameter, int rank)
{

#ifdef HAVE_X86_ADAPT_H
	if (x86_adapt_init())
	{
		std::cerr << "X86ADAPT ERROR: init error\n";
//		exit(1);
	}
#endif
	if (SOCKET_SIZE_CPU <= 0)
		initHWSizes();
	MERIC_INFO << "SOCKET SIZE is " << SOCKET_SIZE_CPU <<std::endl;
	systemType = system;
	
	if (systemType == SYS_DAVIDE)
		return; //temporary solution when HW parameters tuning not supported
	
	if(!rank)
	{
		if (systemType == SYS_OTHER)
			getFrequenciesLimits();
		//activate userspace governor
		if (systemType == SYS_JETSON)
		{
			std::ofstream governorFile;
			if (defaultParameter.frequency != 0) //core
			{
				for (int i=0; i<NODE_SIZE_CPU; i++)
				{
					governorFile.open ("/sys/devices/system/cpu/cpu"+std::to_string(i)+"/cpufreq/scaling_governor");
					governorFile << "userspace";
					governorFile.close();
				}
				MERIC_INFO << "CORE FREQUENCY activated\n";
			}
			if (defaultParameter.uncoreFrequency != 0) //uncore
			{
				governorFile.open ("/sys/kernel/debug/clock/override.emc/state");
				governorFile << "1";
				governorFile.close();
				MERIC_INFO << "unCORE FREQUENCY activated\n";
			}
		}

	//set the settings that user want to apply for the application
		frequency(defaultParameter.frequency);
		uncoreFrequency(defaultParameter.uncoreFrequency, defaultParameter.uncoreFrequency);
	}
	threadNumber(defaultParameter.nthreads);
}


void Environment::frequency(uint64_t freq, int socket)
{
	timespec start_time, stop_time;
	clock_gettime(CLOCK_REALTIME, &start_time);

	if (freq == 0)
		return;

	if (systemType == SYS_THUNDER || systemType == SYS_DAVIDE)
		return;

	if (freq < FreqLimits[systemType].minCoreFreq)
	{
		std::cerr << "CPU frequency has not changed to invalid value: " << freq << std::endl;
		return;
	}
	else if (freq > FreqLimits[systemType].maxCoreFreq)
	{
		MERIC_INFO << "Trying to apply turbo CPU frequency\n";
	}
	usedDVFS = true;

	if (systemType == SYS_JETSON)
	{
		std::ofstream coreFile;	//TODO what if I cannot write to file
		for (int i=0; i<NODE_SIZE_CPU; i++)
		{
			coreFile.open ("/sys/devices/system/cpu/cpu"+std::to_string(i)+"/cpufreq/scaling_setspeed");
			coreFile << freq;
			coreFile.close();
		}
		MERIC_INFO << "JETSON set frequency to: " << freq << std::endl;
	}
	else
	{
#ifdef HAVE_CPUFREQ_H
		MERIC_INFO << "CPUFREQ set frequency to: " << freq;
		int ret;
		int nodeSize = NODE_SIZE_CPU;
		int cpu = 0;
		if(socket != -1)
		{
			nodeSize = SOCKET_SIZE_CPU*(socket+1);
			cpu = SOCKET_SIZE_CPU*socket;
			MERIC_INFO << "; socket: " << socket;
		}
		MERIC_INFO << "; cpu " << cpu << " - " << nodeSize-1<< std::endl;

		for (; cpu < nodeSize; cpu++)
		{
			ret = cpufreq_set_frequency(cpu,freq*CPUFREQ_FREQ_MULT);

			if(ret != 0) // || freq*CPUFREQ_FREQ_MULT != cpufreq_get_freq_kernel(cpu))	//TODO test turbo freq setting
			{
				std::cerr << "CPUFREQ ERROR: failed to set frequency for cpu "<< cpu << std::endl;
			}
		}
		MERIC_INFO << "CPUFREQ: core frequency is " << cpufreq_get_freq_kernel(nodeSize-1) << std::endl;
#elif defined HAVE_X86_ADAPT_H
		int adaptHandler = x86_adapt_lookup_ci_name(X86_ADAPT_CPU, "Intel_Target_PState");
		if (adaptHandler < 0)
		{
			std::cerr << "X86ADAPT ERROR: frequency settings not allowed\n";
			return;
		}
		MERIC_INFO << "X86ADAPT set frequency to: " << freq;

		int nodeSize = NODE_SIZE_CPU;
		int cpu=0;
		if(socket != -1)
		{
			nodeSize = SOCKET_SIZE_CPU*(socket+1);
			cpu = SOCKET_SIZE_CPU*socket;
		}
		MERIC_INFO << "; cpu " << cpu << " - " << nodeSize-1<< std::endl;
		
		for (; cpu<nodeSize; cpu++)
		{
			int fd = x86_adapt_get_device(X86_ADAPT_CPU, cpu);
			if (fd < 0)
			{
				std::cerr << "X86ADAPT ERROR: error while accessing file descriptor\n";
//				exit(2);
			}

			if(x86_adapt_set_setting(fd, adaptHandler, freq*X86ADAPT_FREQ_MULT) != 8)
			{
				std::cerr << "X86ADAPT ERROR: error while setting core frequecy\n";
//				exit(3);
			}
	
			if ( x86_adapt_put_device(X86_ADAPT_CPU, cpu))
				std::cerr << "X86ADAPT ERROR: error while closing file descriptor\n";
		}
#endif
	}
	clock_gettime(CLOCK_REALTIME, &stop_time);
	MERIC_INFO << "MERIC core frequency changed in " << mutils::timeToDouble(stop_time - start_time)*1000 << " ms\n";
}

void Environment::uncoreFrequency(uint64_t maxFreq, uint64_t minFreq, int socket)
{
	timespec start_time, stop_time;
	clock_gettime(CLOCK_REALTIME, &start_time);

	if (maxFreq == 0 || minFreq == 0)
		return;
	
	if (systemType == SYS_THUNDER || systemType == SYS_DAVIDE)
		return;
		
	if (maxFreq > FreqLimits[systemType].maxUncoreFreq || minFreq < FreqLimits[systemType].minUncoreFreq || maxFreq < minFreq)
	{
		std::cerr << "Uncore frequency has not changed to invalid value: " << maxFreq << std::endl;
		MERIC_INFO << FreqLimits[systemType].maxUncoreFreq << " - " << FreqLimits[systemType].minUncoreFreq << std::endl;
		return;
	}
	usedUFS = true;

	if (systemType == SYS_JETSON)
	{
		std::ofstream uncoreFile;
		uncoreFile.open ("/sys/kernel/debug/clock/override.emc/rate");
		uncoreFile << maxFreq << "000";
		uncoreFile.close();
		MERIC_INFO << "JETSON set uncore frequency to: "<< maxFreq << std::endl;
	}
	else
	{
#ifdef HAVE_X86_ADAPT_H
		if (x86_adapt_lookup_ci_name(X86_ADAPT_DIE, "Intel_UNCORE_MAX_RATIO") < 0)
		{
			std::cerr << "X86ADAPT ERROR: uncore frequency settings not allowed\n";
			return;
		}

		MERIC_INFO << "X86ADAPT set uncore frequency to: "<< maxFreq;
		if (maxFreq != minFreq)
			MERIC_INFO << " - " << minFreq;
		int max = NODE_SIZE_SOCKET;
		int die=0;
		if (socket != -1 && socket < max)
		{
			die = socket;
			max = socket+1;
		}
		MERIC_INFO << "; soc " << die << " - " << max-1<< std::endl;
		
		int adaptHandlerUncoreMax = x86_adapt_lookup_ci_name(X86_ADAPT_DIE, "Intel_UNCORE_MAX_RATIO");
		int adaptHandlerUncoreMin = x86_adapt_lookup_ci_name(X86_ADAPT_DIE, "Intel_UNCORE_MIN_RATIO");
		for (; die<max; die++)
		{
			int fd = x86_adapt_get_device(X86_ADAPT_DIE, die);
			if (fd < 0)
			{
				std::cerr << "X86ADAPT ERROR: error while accessing file descriptor\n";
//				exit(2);
			}

			auto setFreq = [&] (int adaptHandler, uint64_t freq)
			{
				if(x86_adapt_set_setting(fd, adaptHandler, freq) != 8)
				{
					std::cerr << "X86ADAPT ERROR: error while setting uncore frequency\n";
//					exit(3);
				}
			};

			setFreq(adaptHandlerUncoreMax, maxFreq);
			setFreq(adaptHandlerUncoreMin, minFreq);
		
			if (x86_adapt_put_device(X86_ADAPT_DIE, die))
				std::cerr << "X86ADAPT ERROR: error while closing file descriptor\n";
		}
#else
#endif
	}
	clock_gettime(CLOCK_REALTIME, &stop_time);
	MERIC_INFO << "MERIC uncore frequency changed in " << mutils::timeToDouble(stop_time - start_time)*1000 << " ms\n";
}

void Environment::setDefaultFrequencies()
{
	if (usedDVFS)
		frequency(FreqLimits[systemType].defaultCoreFreq);
	if (systemType == SYS_JETSON)
	{
		uncoreFrequency(FreqLimits[SYS_JETSON].defaultUncoreFreq, FreqLimits[SYS_JETSON].defaultUncoreFreq);
	
		std::ofstream governorFile;
		//core
		for (int i=0; i<NODE_SIZE_CPU; i++)
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
	else
	{
		if(usedUFS)
			uncoreFrequency(FreqLimits[systemType].maxUncoreFreq, FreqLimits[systemType].minUncoreFreq);
	}
}

void Environment::close()
{
#ifdef HAVE_X86_ADAPT_H
	x86_adapt_finalize();
#endif
}


