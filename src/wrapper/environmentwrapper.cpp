
#include "environmentwrapper.h"

/**
 * UNCORE Freq
 * From Intel 64 and IA-32 Architectures Software Developer's Manual 
 * Volume 4: Model-Specific Registers
 *
 * MSR_UNCORE_RATIO_LIMIT Register ********************************************
 * Out of reset, the min_ratio and max_ratio fields represent the widest 
 * possible range of uncore frequencies. Writing to these fields allows software 
 * to control the minimum and the maximum frequency that hardware will select.
 * 0x000000620 0x0000000000007F7F
 * MAX_RATIO (bits 6:0):
 	This field is used to limit the max ratio of the LLC/Ring.
 * Reserved (bit 7)
 * MIN_RATIO (bits 14:8):
 	Writing to this field controls the minimum possible ratio of the LLC/Ring.
 * Reserved (bits 63:15)
 *
 *
 * Intel RAPL power capping registers description
 * From Intel 64 and IA-32 Architectures Software Developer's Manual
 *
 * MSR_PKG_POWER_LIMIT register ***********************************************
 * Register allows a software agent to define power limitation for the package 
 * domain. Power limitation is defined in terms of average power usage (Watts) 
 * over a time window specified in MSR_PKG_POWER_LIMIT. Two power limits can be 
 * specified, corresponding to time windows of different sizes. Each power limit 
 * provides independent clamping control that would permit the processor cores 
 * to go below OS-requested state to meet the power limits. A lock mechanism 
 * allow the software agent to enforce power limit settings. Once the lock bit 
 * is set, the power limit settings are static and unmodifiable until next RESET
 * 0x00000610 0x00ffffff00ffffff
 * Package Power Limit #1 (bits 14:0): 
	Sets the average power usage limit of the package domain corresponding 
	to time window # 1. The unit of this field is specified by the "Power Units"
	field of MSR_RAPL_POWER_UNIT
 * Enable Power Limit #1 (bit 15): 
	0 = disabled; 1 = enabled.
 * Package Clamping Limitation #1 (bit 16):
 	0 = disabled; 1 = enabled.
	Allow going below OS-requested P/T state setting during time window 
	specified by bits 23:17.
 * Time Window for Power Limit #1 (bits 23:17): 
	Indicates the time window for power limit #1 
	Time limit = 2^Y * (1.0 + Z/4.0) * Time_Unit 
	Here "Y" is the unsigned integer value represented. by bits 21:17, 
	"Z" is an unsigned integer represented by bits 23:22. 
	"Time_Unit" is specified by the "Time Units" field of MSR_RAPL_POWER_UNIT.
 * Reserved (bits 31:24)
 * Package Power Limit #2 (bits 46:32): 
	Sets the average power usage limit of the package domain corre-sponding 
	to time window # 2. The unit of this field is specified by the "Power Units" 
	field of MSR_RAPL_POWER_UNIT.
 * Enable Power Limit #2 (bit 47): 
	0 = disabled; 1 = enabled.
 * Package Clamping Limitation #2 (bit 48):
	0 = disabled; 1 = enabled.
	Allow going below OS-requested P/T state setting during time window 
	specified by bits 23:17.
 * Time Window for Power Limit #2 (bits 55:49): 
	Indicates the time window for power limit #2 
	Time limit = 2^Y * (1.0 + Z/4.0) * Time_Unit 
	Here "Y" is the unsigned integer value represented. by bits 53:49, 
	"Z" is an unsigned integer represented by bits 55:54. 
	"Time_Unit" is specified by the "Time Units" field of MSR_RAPL_POWER_UNIT. 
	This field may have a hard-coded value in hardware and ignores values 
	written by software.
 * Reserved (bits 62:56)
 * Lock (bit 63):
	If set, all write attempts to this MSR are ignored until next 
	RESET.MSR_PKG_ENERGY_STATUS is a read-only MSR. It reports the actual energy 
	use for the package domain. This MSR is updated every ~1msec. It has 
	a wraparound time of around 60 secs when power consumption is high, and may 
	be longer otherwise.
 *
 * MSR_DRAM_POWER_LIMIT register **********************************************
 * allows a software agent to define power limitation for the DRAM domain. Power 
 * limitation is defined in terms of average power usage (Watts) over a time 
 * window specified in MSR_DRAM_POWER_LIMIT. A power limit can be specified 
 * along with a time window. A lock mechanism allow the software agent to 
 * enforce power limit settings. Once the lock bit is set, the power limit 
 * settings are static and unmodifiable until next RESET
 * 0x00000618 0x0000000000ffffff
 * DRAM Power Limit #1(bits 14:0):
	Sets the average power usage limit of the DRAM domain corresponding to time 
	window # 1. The unit of this field is specified by the "Power Units" field 
	of MSR_RAPL_POWER_UNIT.
 * Enable Power Limit #1(bit 15):
	0 = disabled; 1 = enabled.
 * Claping limit (bit 16):
	0 = disabled; 1 = enabled.
	Allow going below OS-requested P/T state setting during time window 
	specified by bits 23:17.
 * Time Window for Power Limit (bits 23:17):
	Indicates the length of time window over which the power limit will be used 
	by the processor. The numeric value encoded by bits 23:17 is represented by 
	the product of 2^Y *F; where F is a single-digit decimal floating-point 
	value between 1.0 and 1.3 with the fraction digit represented by bits 23:22, 
	Y is an unsigned integer represented by bits 21:17. The unit of this field 
	is specified by the "Time Units" field of MSR_RAPL_POWER_UNIT.
 * Registred (bits 30:24)
 * Lock (bit 31): 
	If set, all write attempts to this MSR are ignored until next RESET.
 * Registred (bits 63:32)
 **/


using namespace meric;

// LIST OF POSSIBLE frequencies for JETSON CPUs [kHz]
//	102000 204000 307200 403200 518400 614400 710400 825600 921600 1036800 1132800 1224000 1326000 1428000 1555500 1632000 1734000
// LIST OF POSSIBLE frequencies for JETSON memory [kHz]
//	40800 68000 102000 204000 408000 665600 800000 1065600 1331200 1600000
// LIST OF POSSIBLE frequencies for DAVIDE [kHz]
//	4023000 3757000 3491000 3225000 2959000 2693000 2427000 2161000 2061000

//systemLimits Environment::FreqLimits [4];
systemLimits Environment::FreqLimits [] = {
	[SYS_HASWELL] = { 3300000000, 2500000000, 1200000000, 3000000000, 3000000000, 1200000000 },
	[SYS_JETSON]  = { 1734000000,  518400000,  102000000, 1600000000,  408000000,   40800000 },
	[SYS_THUNDER] = { -1, -1, -1, -1, -1, -1 }, //DVFS+UFS not supported
	[SYS_DAVIDE]  = { 4023000000, 4023000000, 2061000000, -1, -1, -1 }, //UFS not supported
	[SYS_OTHER]   = { -1, -1, -1, -1, -1, -1 }
};

int Environment::NODE_SIZE_THREAD = 1;
int Environment::NODE_SIZE_CPU = 0;
int Environment::NODE_SIZE_SOCKET = 0;
int Environment::SOCKET_SIZE_CPU = 0;
int Environment::systemType = SYS_HASWELL;
uint64_t Environment::IA32_ENERGY_PERF_BIAS_default = 0;
bool Environment::usedDVFS = false;
bool Environment::usedUFS = false;
bool Environment::CPUFREQ_GOVERNOR = false;
std::string Environment::originalGovernor = "";

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

/**
 * x86_adapt must be initialized befor it us used here
 **/
void Environment::initHWSizes()
{
#ifdef HAVE_MSR_CORE_H
	int Hthreads;
	uint64_t cs = 0;
	uint64_t ts = 0;
	uint64_t ss = 0;
	core_config(&cs, &ts, &ss, &Hthreads);
	NODE_SIZE_SOCKET = ss;
	SOCKET_SIZE_CPU  = cs;
	NODE_SIZE_CPU    = SOCKET_SIZE_CPU * NODE_SIZE_SOCKET;
	NODE_SIZE_THREAD = ts * NODE_SIZE_CPU;
	MERIC_INFO << "HThreads: " << ((Hthreads == 0) ? "inactive" : "active") << std::endl;
#elif defined HAVE_X86_ADAPT_H
	NODE_SIZE_CPU    = x86_adapt_get_nr_avaible_devices(X86_ADAPT_CPU);
	NODE_SIZE_SOCKET = x86_adapt_get_nr_avaible_devices(X86_ADAPT_DIE);
	//NODE_SIZE_THREAD = 1;	TODO get real number of avail threads per core - not used now
	SOCKET_SIZE_CPU = NODE_SIZE_CPU / NODE_SIZE_SOCKET;
#else //ifdef HAVE_NUMA_H
	NODE_SIZE_CPU    = sysconf(_SC_NPROCESSORS_ONLN);
	NODE_SIZE_SOCKET = numa_node_of_cpu(NODE_SIZE_CPU-1)+1;
	//NODE_SIZE_THREAD = 1;	TODO
	if (NODE_SIZE_SOCKET <= 0)
		NODE_SIZE_SOCKET=1;
	SOCKET_SIZE_CPU = NODE_SIZE_CPU / NODE_SIZE_SOCKET;
#endif
	MERIC_INFO << "CPUs " << NODE_SIZE_CPU << " SOCKETS " << NODE_SIZE_SOCKET << std::endl;
}

void Environment::getFrequenciesLimits()
{
	//CORE freq
	std::ifstream frequencyBounderyFile;
	std::string freqBound; //files has the value in KHz
	frequencyBounderyFile.open ("/sys/devices/system/cpu/cpu0/cpufreq/cpuinfo_max_freq");
	frequencyBounderyFile >> freqBound;
	frequencyBounderyFile.close();
	FreqLimits[SYS_OTHER].maxCoreFreq = std::stol(freqBound) * 1000;
	FreqLimits[SYS_OTHER].defaultCoreFreq = FreqLimits[SYS_OTHER].maxCoreFreq;
	frequencyBounderyFile.open ("/sys/devices/system/cpu/cpu0/cpufreq/cpuinfo_min_freq");
	frequencyBounderyFile >> freqBound;
	frequencyBounderyFile.close();
	FreqLimits[SYS_OTHER].minCoreFreq = std::stol(freqBound) * 1000;

	//UnCORE freq
#ifdef HAVE_MSR_CORE_H
	uint64_t MSR_UNCORE_RATIO_LIMIT_bits;
	read_msr_by_idx(0, MSR_UNCORE_RATIO_LIMIT, &MSR_UNCORE_RATIO_LIMIT_bits);

	FreqLimits[SYS_OTHER].maxUncoreFreq = (MSR_UNCORE_RATIO_LIMIT_bits & MSR_UNCORE_RATIO_LIMIT_max_mask) * 100000000;
	FreqLimits[SYS_OTHER].defaultUncoreFreq = FreqLimits[SYS_OTHER].maxUncoreFreq;
	FreqLimits[SYS_OTHER].minUncoreFreq = ((MSR_UNCORE_RATIO_LIMIT_bits & MSR_UNCORE_RATIO_LIMIT_min_mask) >> MSR_UNCORE_RATIO_LIMIT_min_shift) * 100000000;

#elif defined HAVE_X86_ADAPT_H
	int fd = x86_adapt_get_device_ro(X86_ADAPT_DIE, 0);
	int adaptHandlerUncore = x86_adapt_lookup_ci_name(X86_ADAPT_DIE, "Intel_UNCORE_MAX_RATIO");
	uint64_t setting;
	x86_adapt_get_setting(fd, adaptHandlerUncore, &setting);
	FreqLimits[SYS_OTHER].maxUncoreFreq = setting * 100000000;
	FreqLimits[SYS_OTHER].defaultUncoreFreq = FreqLimits[SYS_OTHER].maxUncoreFreq;
	adaptHandlerUncore = x86_adapt_lookup_ci_name(X86_ADAPT_DIE, "Intel_UNCORE_MIN_RATIO");
	x86_adapt_get_setting(fd, adaptHandlerUncore, &setting);
	FreqLimits[SYS_OTHER].minUncoreFreq = setting * 100000000;
#endif

#if defined HAVE_MSR_CORE_H || defined HAVE_X86_ADAPT_H
	struct stat info;
	std::string uncoreFrequencyDefaultFile = std::string(MERIC_PATH)+"/build/ucfRestore.csv";
	if(stat(uncoreFrequencyDefaultFile.c_str(), &info ) != 0)
	{
		if (FreqLimits[SYS_OTHER].minUncoreFreq != FreqLimits[SYS_OTHER].maxUncoreFreq)
		{
			std::ofstream ucfRestoreFile;
			ucfRestoreFile.open (uncoreFrequencyDefaultFile);
			ucfRestoreFile << FreqLimits[SYS_OTHER].minUncoreFreq << " " << FreqLimits[SYS_OTHER].maxUncoreFreq << std::endl;
			ucfRestoreFile.close();
		}
		else
		{
			std::cerr << "MERIC error: system does not have correctly set uncore frequency range\n";
		}
	}
	else if (FreqLimits[SYS_OTHER].minUncoreFreq == FreqLimits[SYS_OTHER].maxUncoreFreq)
	{
		std::string ucfString;
		std::ifstream ucfRestoreFile;
		ucfRestoreFile.open (uncoreFrequencyDefaultFile);
		ucfRestoreFile >> ucfString;
		FreqLimits[SYS_OTHER].minUncoreFreq = std::stoi(ucfString);
		ucfRestoreFile >> ucfString;
		FreqLimits[SYS_OTHER].maxUncoreFreq = std::stoi(ucfString);
		ucfRestoreFile.close();
		FreqLimits[SYS_OTHER].defaultUncoreFreq = FreqLimits[SYS_OTHER].maxUncoreFreq;
	}
#endif
	MERIC_INFO << "CORE FREQ limits   [Hz]: " << FreqLimits[SYS_OTHER].maxCoreFreq << '\t' << FreqLimits[SYS_OTHER].minCoreFreq << std::endl;
	MERIC_INFO << "UNCORE FREQ limits [Hz]: " << FreqLimits[SYS_OTHER].maxUncoreFreq << '\t' << FreqLimits[SYS_OTHER].minUncoreFreq<< std::endl;
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

#ifdef HAVE_MSR_CORE_H
	if (init_msr())
	{
		std::cerr << "LIBMSR ERROR: Unable to initialize\n";
//		exit(1);
	}
#endif
	if (SOCKET_SIZE_CPU <= 0)
		initHWSizes();
	MERIC_INFO << "SOCKET SIZE is " << SOCKET_SIZE_CPU <<std::endl;
	systemType = system;
	
	if(!rank)
	{
		if (systemType == SYS_OTHER)
		{
			getFrequenciesLimits();
			//GOVERNOR
			std::ifstream governorFileI;
			if (defaultParameter.frequency != 0)
			{
				governorFileI.open("/sys/devices/system/cpu/cpu0/cpufreq/scaling_governor");
				governorFileI >> originalGovernor;
				governorFileI.close();
// USERSPACE GOVERNOR
				if (originalGovernor == "userspace")
				{
					CPUFREQ_GOVERNOR=true;
				}
				else
				{
					governorFileI.open("/sys/devices/system/cpu/cpu0/cpufreq/scaling_available_governors");
					std::string availGovernor;
					while (governorFileI >> availGovernor)
					{
						if (availGovernor == "userspace")
						{
							CPUFREQ_GOVERNOR = true;
							break;
						}
					}
					governorFileI.close();
					if (CPUFREQ_GOVERNOR == true)
					{
						std::ofstream governorFileO;
						for (int i=0; i<NODE_SIZE_CPU; i++)
						{
							governorFileO.open ("/sys/devices/system/cpu/cpu"+std::to_string(i)+"/cpufreq/scaling_governor");
							governorFileO << "userspace";
							governorFileO.close();
						}
					}
// POWERSAVE GOVERNOR TODO - zjistit s kterymi to presne funguje
#ifdef HAVE_MSR_CORE_H
					else
					{
						std::ofstream defaultSettings;
						MERIC_INFO << "Disable turbo logic of Intel P-state driver\n";
						defaultSettings.open ("/sys/devices/system/cpu/intel_pstate/no_turbo");
						defaultSettings << "1";
						defaultSettings.close();
						
						MERIC_INFO << "Disable scaling frequency of Intel P-state driver and set maximum P-state to all cores and set energy performance bias to maximum performance\n";
						read_msr_by_idx(0, IA32_ENERGY_PERF_BIAS, &IA32_ENERGY_PERF_BIAS_default);
						uint64_t REG_MAX_PSTATE = (FreqLimits[systemType].maxCoreFreq/100000000) << 8;
						for (int cpu=0; cpu<NODE_SIZE_CPU; cpu++)
						{
							defaultSettings.open ("/sys/devices/system/cpu/cpu"+std::to_string(cpu)+"/cpufreq/scaling_max_freq");
							defaultSettings << FreqLimits[systemType].minCoreFreq;
							defaultSettings.close();

							write_msr_by_idx(cpu, IA32_ENERGY_PERF_BIAS, 0);
							write_msr_by_idx(cpu, IA32_PERF_CTL, REG_MAX_PSTATE);
						}
					}
#endif
				}
			}
		}
		//activate userspace governor
		else if (systemType == SYS_JETSON)
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

	if (FreqLimits[SYS_OTHER].defaultCoreFreq == -1)
		return;	//DVFS not supported


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
		int nodeSize = NODE_SIZE_CPU;
		int cpu=0;
		if(socket != -1)
		{
			nodeSize = SOCKET_SIZE_CPU*(socket+1);
			cpu = SOCKET_SIZE_CPU*socket;
		}
#ifdef HAVE_MSR_CORE_H
		MERIC_INFO << "LIBMSR set frequency to: " << freq;
		if (socket != -1)
			MERIC_INFO << "; socket: " << socket;
		MERIC_INFO << "; cpu " << cpu << " - " << nodeSize-1<< std::endl;

		for (; cpu<nodeSize; cpu++)
		{
			write_msr_by_idx(cpu, IA32_PERF_CTL, (freq/100000000) << 8);
		}
#else
	#ifdef HAVE_CPUFREQ_H
		if (CPUFREQ_GOVERNOR)
		{
				int ret;
				MERIC_INFO << "CPUFREQ set frequency to: " << freq;
				if(socket != -1)
					MERIC_INFO << "; socket: " << socket;
				MERIC_INFO << "; cpu " << cpu << " - " << nodeSize-1<< std::endl;

				for (; cpu < nodeSize; cpu++)
				{
					ret = cpufreq_set_frequency(cpu,freq);

					if(ret != 0) // || freq*100000 != cpufreq_get_freq_kernel(cpu))	//TODO test turbo freq setting
					{
						std::cerr << "CPUFREQ ERROR: failed to set frequency for cpu "<< cpu << std::endl;
					}
				}
				MERIC_INFO << "CPUFREQ: core frequency is " << cpufreq_get_freq_kernel(nodeSize-1) << std::endl;
		}
		else
	#endif
	#ifdef HAVE_X86_ADAPT_H
		{
				int adaptHandler = x86_adapt_lookup_ci_name(X86_ADAPT_CPU, "Intel_Target_PState");
				if (adaptHandler < 0)
				{
					std::cerr << "X86ADAPT ERROR: frequency settings not allowed\n";
					return;
				}
				MERIC_INFO << "X86ADAPT set frequency to: " << freq;
				if(socket != -1)
					MERIC_INFO << "; socket: " << socket;
				MERIC_INFO << "; cpu " << cpu << " - " << nodeSize-1<< std::endl;
		
				for (; cpu<nodeSize; cpu++)
				{
					int fd = x86_adapt_get_device(X86_ADAPT_CPU, cpu);
					if (fd < 0)
					{
						std::cerr << "X86ADAPT ERROR: error while accessing file descriptor\n";
		//				exit(2);
					}

					if(x86_adapt_set_setting(fd, adaptHandler, (freq/100000000)*MSR_FREQ_MULT) != 8)
					{
						std::cerr << "X86ADAPT ERROR: error while setting core frequecy\n";
		//				exit(3);
					}
	
					if ( x86_adapt_put_device(X86_ADAPT_CPU, cpu))
						std::cerr << "X86ADAPT ERROR: error while closing file descriptor\n";
				}
		}
	#elif defined HAVE_CPUFREQ_H
			std::cerr << "CPUFREQ error: governor not specified as userspace, CPU core frequency not changed\n";
	#endif
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
	
	if (FreqLimits[SYS_OTHER].defaultUncoreFreq == -1)
		return; //UFS not supported
		
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
		uncoreFile << maxFreq;
		uncoreFile.close();
		MERIC_INFO << "JETSON set uncore frequency to: "<< maxFreq << std::endl;
	}
	else
	{
#ifdef HAVE_MSR_CORE_H
		MERIC_INFO << "LIBMSR set uncore frequency to: "<< maxFreq;
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

		uint64_t MSR_UNCORE_RATIO_LIMIT_bits = ((minFreq/100000000) << MSR_UNCORE_RATIO_LIMIT_min_shift) + maxFreq/100000000;
		for (; die<max; die++)
		{
			write_msr_by_idx(die * SOCKET_SIZE_CPU, MSR_UNCORE_RATIO_LIMIT, MSR_UNCORE_RATIO_LIMIT_bits);
		}
		
#elif defined HAVE_X86_ADAPT_H
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

			setFreq(adaptHandlerUncoreMax, maxFreq/100000000);
			setFreq(adaptHandlerUncoreMin, minFreq/100000000);
		
			if (x86_adapt_put_device(X86_ADAPT_DIE, die))
				std::cerr << "X86ADAPT ERROR: error while closing file descriptor\n";
		}
#endif
	}
	clock_gettime(CLOCK_REALTIME, &stop_time);
	MERIC_INFO << "MERIC uncore frequency changed in " << mutils::timeToDouble(stop_time - start_time)*1000 << " ms\n";
}

void Environment::setDefaultFrequencies()
{
	if (usedDVFS)
	{
		frequency(FreqLimits[systemType].defaultCoreFreq);
		if (CPUFREQ_GOVERNOR == true && originalGovernor == "userspace")
		{
			std::ofstream governorFile;
			for (int i=0; i<NODE_SIZE_CPU; i++)
			{
				governorFile.open ("/sys/devices/system/cpu/cpu"+std::to_string(i)+"/cpufreq/scaling_governor");
				governorFile << originalGovernor;
				governorFile.close();
			}
		}
#ifdef HAVE_MSR_CORE_H
		else
		{
			std::ofstream defaultSettings;
			MERIC_INFO << "Enable turbo logic of Intel P-state driver\n";
			defaultSettings.open ("/sys/devices/system/cpu/intel_pstate/no_turbo");
			defaultSettings << "0";
			defaultSettings.close();
			
			MERIC_INFO << "Enable scaling frequency of Intel P-state driver and set default energy performance bias\n";
			for (int cpu=0; cpu<NODE_SIZE_CPU; cpu++)
			{
				defaultSettings.open ("/sys/devices/system/cpu/cpu"+std::to_string(cpu)+"/cpufreq/scaling_max_freq");
				defaultSettings << FreqLimits[systemType].maxCoreFreq;
				defaultSettings.close();
				write_msr_by_idx(cpu, IA32_ENERGY_PERF_BIAS, IA32_ENERGY_PERF_BIAS_default);
			}
		}
#endif
	}
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
#ifdef HAVE_MSR_CORE_H
	finalize_msr();
#endif
}


