
#include "environmentwrapper.h"

using namespace meric;

// LIST OF POSSIBLE frequencies for JETSON CPUs (kHz)
//	102000 204000 307200 403200 518400 614400 710400 825600 921600 1036800 1132800 1224000 1326000 1428000 1555500 1632000 1734000
// LIST OF POSSIBLE frequnecies for JETSON memory (kHz)
//	40800 68000 102000 204000 408000 665600 800000 1065600 1331200 1600000


systemLimits Environment::FreqLimits [] = {
	[SYS_HASWELL] = { 33, 25, 12, 30, 30, 12 },
	[SYS_JETSON]  = { 1734000, 518400, 102000, 1600000, 408000, 40800},
	[SYS_THUNDER] = { -1, -1, -1, -1, -1, -1} //DVFS not supported
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

	SOCKET_SIZE = sysconf(_SC_NPROCESSORS_ONLN);
	SOCKET_SIZE = SOCKET_SIZE/(numa_node_of_cpu(SOCKET_SIZE-1)+1); //requires compilation with -lnuma
	MERIC_INFO << "SOCKET SIZE is " << SOCKET_SIZE <<std::endl;
#endif	
	systemType = system;
	
	if(!rank)
	{
		if (systemType == SYS_JETSON)
		{
			std::ofstream governorFile;
			if (defaultParameter.frequency != 0) //core
			{
				for (int i=0; i<sysconf(_SC_NPROCESSORS_ONLN); i++)
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

		frequency(defaultParameter.frequency);
		uncoreFrequency(defaultParameter.uncoreFrequency);
	}
	threadNumber(defaultParameter.nthreads);
}


void Environment::frequency(uint64_t freq, int socket)
{
	if (freq == 0)
		return;

	if (systemType == SYS_THUNDER)
		return;

	if (freq > FreqLimits[systemType].maxCoreFreq || freq < FreqLimits[systemType].minCoreFreq)
	{
		std::cerr << "CPU frequency has not changed to invalid value: " << freq << std::endl;
		return;
	}

	if (systemType == SYS_JETSON)
	{
		std::ofstream coreFile;	//TODO what if I cannot write to file
		for (int i=0; i<sysconf(_SC_NPROCESSORS_ONLN); i++)
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
		if (freq <= CPUFREQ_MAX_FREQ)
		{
			MERIC_INFO << "CPUFREQ set frequency to: " << freq;
			int ret;
			int nodeSize = sysconf(_SC_NPROCESSORS_ONLN);
			int cpu = 0;
			if(socket != -1)
			{
				nodeSize = SOCKET_SIZE*(socket+1);
				cpu = SOCKET_SIZE*socket;
				MERIC_INFO << "; socket: " << socket;
			}
			MERIC_INFO << "; cpu " << cpu << " - " << nodeSize-1<< std::endl;

			for (; cpu < nodeSize; cpu++)
			{
				ret = cpufreq_set_frequency(cpu,freq*CPUFREQ_FREQ_MULT);

				if(ret != 0 || freq*CPUFREQ_FREQ_MULT != cpufreq_get_freq_kernel(cpu))
				{
					std::cerr << "CPUFREQ ERROR: failed to set frequency for cpu "<< cpu << std::endl;
	//				MPI_Abort(MPI_COMM_WORLD, 1);
				}
			}
		}
		else
#endif
#ifdef HAVE_X86_ADAPT_H
		{	//x86adapt allows to set cpu frequency up to 3.3 GHz
			if (x86_adapt_lookup_ci_name(X86_ADAPT_CPU, "Intel_Target_PState") < 0)
			{
				std::cerr << "X86ADAPT ERROR: frequency settings not allowed\n";
				return;
			}
			MERIC_INFO << "X86ADAPT set frequency to: " << freq;

			int nodeSize = x86_adapt_get_nr_avaible_devices(X86_ADAPT_CPU);
			int cpu=0;
			if(socket != -1)
			{
				nodeSize = SOCKET_SIZE*(socket+1);
				cpu = SOCKET_SIZE*socket;
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

				if(x86_adapt_set_setting(fd, x86_adapt_lookup_ci_name(X86_ADAPT_CPU, "Intel_Target_PState"),freq*X86ADAPT_FREQ_MULT) != 8)
				{
					std::cerr << "X86ADAPT ERROR: error while setting core frequecy\n";
	//					exit(3);
				}
		
				if ( x86_adapt_put_device(X86_ADAPT_CPU, cpu))
					std::cerr << "X86ADAPT ERROR: error while closing file descriptor\n";
			}
		}
#else
		{
			std::cerr << "CPU frequency has not changed to value: " << freq << std::endl;
		}
#endif
	}
}

void Environment::uncoreFrequency(uint64_t freq, int socket)
{
	if (freq == 0)
		return;

	if (systemType == SYS_THUNDER)
		return;
		
	if (freq > FreqLimits[systemType].maxUncoreFreq || freq < FreqLimits[systemType].minUncoreFreq)
	{
		std::cerr << "Uncore frequency has not changed to invalid value: " << freq << std::endl;
		return;
	}

	if (systemType == SYS_JETSON)
	{
		std::ofstream uncoreFile;
		uncoreFile.open ("/sys/kernel/debug/clock/override.emc/rate");
		uncoreFile << freq << "000";
		uncoreFile.close();
		MERIC_INFO << "JETSON set uncore frequency to: "<< freq << std::endl;
	}
	else
	{
#ifdef HAVE_X86_ADAPT_H
		if (x86_adapt_lookup_ci_name(X86_ADAPT_DIE, "Intel_UNCORE_MAX_RATIO") < 0)
		{
			std::cerr << "X86ADAPT ERROR: uncore frequency settings not allowed\n";
			return;
		}

		MERIC_INFO << "X86ADAPT set uncore frequency to: "<< freq;
		int max = x86_adapt_get_nr_avaible_devices(X86_ADAPT_DIE);
		int die=0;
		if (socket != -1 && socket < max)
		{
			die = socket;
			max = socket+1;
		}
		MERIC_INFO << "; soc " << die << " - " << max-1<< std::endl;
		
		for (; die<max; die++)
		{
			int fd = x86_adapt_get_device(X86_ADAPT_DIE, die);
			if (fd < 0)
			{
				std::cerr << "X86ADAPT ERROR: error while accessing file descriptor\n";
//				exit(2);
			}

			auto setFreq = [&] (const char * description)
			{
				if(x86_adapt_set_setting(fd, x86_adapt_lookup_ci_name(X86_ADAPT_DIE, description),freq ) != 8)
				{
					std::cerr << "X86ADAPT ERROR: error while setting uncore frequency\n";
//					exit(3);
				}
			};

			setFreq("Intel_UNCORE_MAX_RATIO");
			setFreq("Intel_UNCORE_MIN_RATIO");
		
			if ( x86_adapt_put_device(X86_ADAPT_DIE, die))
				std::cerr << "X86ADAPT ERROR: error while closing file descriptor\n";
		}
#else
#endif
	}
}

void Environment::setDefaultFrequencies()
{
	frequency(FreqLimits[systemType].defaultCoreFreq);
	uncoreFrequency(FreqLimits[systemType].defaultUncoreFreq);

	if (systemType == SYS_JETSON)
	{
		std::ofstream governorFile;
		//core
		for (int i=0; i<sysconf(_SC_NPROCESSORS_ONLN); i++)
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


