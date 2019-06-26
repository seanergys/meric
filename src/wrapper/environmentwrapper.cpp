
#include "environmentwrapper.h"
#include "raplwrapper.h"

/**
 * Intel CPU uncore frequency
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
 ******************************************************************************
 *
 * Intel RAPL power capping registers description
 * From Intel 64 and IA-32 Architectures Software Developer's Manual
 * NOTE: RAPL power, energy and time units description is in the raplwrapper.
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
 * Reserved (bits 30:24)
 * Lock (bit 31): 
	If set, all write attempts to this MSR are ignored until next RESET.
 * Reserved (bits 63:32)
 *
 * IA32_PERF_CTL register *****************************************************
 * Performance Control MSR (R/W) Software makes a request for a new Performance 
 * state (P-State) by writing this MSR.
 * State transitions are initiated by writing a 16-bit value to the 
 * IA32_PERF_CTL register, see Figure 14-2. If a transition is already in 
 * progress, transition to a new value will subsequently take effect. Reads of 
 * IA32_PERF_CTL determine the last targeted operating point. The current 
 * operating point can be read from IA32_PERF_STATUS. IA32_PERF_STATUS is 
 * updated dynamically. The 16-bit encoding that defines valid operating points 
 * is model-specific. Applications and performance tools are not expected to use 
 * either IA32_PERF_CTL or IA32_PERF_STATUS and should treat both as reserved. 
 * Performance monitoring tools can access model-specific events and report the 
 * occurrences of state transitions.
 * Target performance State Value (bits 15:0)
 * Reserved (bits 31:16)
 * IDA Engage (R/W) (bit 32):
 	When set to 1: disengages IDA.
 * Reserved (63:33)
 *
 * IA32_ENERGY_PERF_BIAS register *********************************************
 * Performance Energy Bias Hint (R/W)
 * Power Policy Preference (bits 3:0):
 	0 indicates preference to highest performance.
	15 indicates preference to maximize energy saving.
 * Reserved (bits 63:4)
 *
 * MSR_PKG_POWER_INFO/MSR_DRAM_POWER_INFO register ****************************
 * Read-only MSR. It reports the package/DRAM power range information for RAPL 
 * usage. This MSR provides maximum/minimum values (derived from electrical 
 * specification), thermal specification power of the package/DRAM domain. It 
 * also provides the largest possible time window for software to program the 
 * RAPL interface.
 * Thermal Spec Power (bits 14:0):
	The unsigned integer value is the equivalent of thermal specification power
	of the package/DRAM domain. The unit of this field is specified by the 
	"Power Units" field of MSR_RAPL_POWER_UNIT.
 * Reserved (bit 15)
 * Minimum Power (bits 30:16):
	The unsigned integer value is the equivalent of minimum power derived from
	electrical spec of the package/DRAM domain. The unit of this field is 
	specified by the "Power Units" field of MSR_RAPL_POWER_UNIT.
 * Reserved (bit 31)
 * Maximum Power (bits 46:32):
	The unsigned integer value is the equivalent of maximum power derived from
	the electrical spec of the package/DRAM domain. The unit of this field is 
	specified by the "Power Units" field of MSR_RAPL_POWER_UNIT.
 * Reserved (bit 47)
 * Maximum Time Window (bits 53:48):
	The unsigned integer value is the equivalent of largest acceptable value to 
	program the time window of MSR_PKG_POWER_LIMIT/MSR_DRAM_POWER_LIMIT. The 
	unit of this field is specified by the "Time Units" field of 
	MSR_RAPL_POWER_UNIT.
 * Reserved (bits 63:54)
 *
 * MSR_PKG_PERF_STATUS/MSR_DRAM_PERF_STATUS register **************************
 * Read-only MSR. It reports the total time for which the package was throttled 
 * due to the RAPL power limits. Throttling in this context is defined as going 
 * below the OS-requested P-state or T-state. It has a wrap-around time of many 
 * hours. The availability of this MSR is platform specific (see Chapter 2, 
 * "Model-Specific Registers (MSRs)" in the Intel® 64 and IA-32 Architectures 
 * Software Developer's Manual, Volume 4).
 * Accumulated Package Throttled Time (bits 31:0):
	The unsigned integer value represents the cumulative time (since the last 
	time this register is cleared) that the package/DRAM domain has throttled. 
	The unit of this field is specified by the "Time Units" field of 
	MSR_RAPL_POWER_UNIT.
 * Reserved (bits 63:32)
 ******************************************************************************
 **/


using namespace meric;

// LIST OF POSSIBLE frequencies for JETSON CPUs [kHz]
//	102000 204000 307200 403200 518400 614400 710400 825600 921600 1036800 1132800 1224000 1326000 1428000 1555500 1632000 1734000
// LIST OF POSSIBLE frequencies for JETSON memory [kHz]
//	40800 68000 102000 204000 408000 665600 800000 1065600 1331200 1600000
// LIST OF POSSIBLE frequencies for DAVIDE [kHz]
//	4023000 3757000 3491000 3225000 2959000 2693000 2427000 2161000 2061000

systemLimits Environment::defaultSettings [] = {
	[SYS_HASWELL] = { 3300000000, 2500000000, 1200000000, //DVFS
	                  3000000000, 3000000000, 1200000000, //UFS 
	                         290,         66,      45898, 13239288264131720, //PKG powercap
	                          44,          4,      45898, 13230870095724886,//RAM powercap
	                         0x0},
	[SYS_JETSON]  = { 1734000000, 518400000, 102000000, 1600000000, 408000000, 40800000, -1, -1, -1, 0x0, -1, -1, -1, 0x0, 0x0},
	[SYS_THUNDER] = { -1, -1, -1, -1, -1, -1, -1, -1, -1, 0x0, -1, -1, -1, 0x0, 0x0}, //system tuning not supported
	[SYS_DAVIDE]  = { 4023000000, 4023000000, 2061000000, -1, -1, -1, -1, -1, -1, 0x0, -1, -1, -1, 0x0, 0x0}, //UFS+powercap not supported
	[SYS_OTHER]   = { -1, -1, -1, -1, -1, -1, -1, -1, -1, 0x0, -1, -1, -1, 0x0, 0x0}
};

int Environment::NODE_SIZE_THREAD = 1;
int Environment::NODE_SIZE_CPU = 0;
int Environment::NODE_SIZE_SOCKET = 0;
int Environment::SOCKET_SIZE_CPU = 0;
int Environment::systemType = SYS_OTHER;

std::string Environment::defaultGovernor = "";

bool Environment::usedDVFS = false;
bool Environment::usedUFS = false;
bool Environment::usedPWRCAP = false;
bool Environment::activatedDVFS = false;
bool Environment::activatedUFS = false;
bool Environment::activatedPWRCAP = false;

/**
 * change number of active threads
 **/
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
 * reads HW system sizes:
 *  - socket per node
 *  - CPUs per socket
 *  - threads per CPU
 *  - hyper-threading enabled
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

/**
 * reads system default CPU core and uncore frequencies and power capping limits
 **/
void Environment::getSystemDefaultSettings()
{

//TODO POWERcap limits - MSR-SAFE only /////////////////////////////////////////
//TODO store the values to the json ////////////////////////////////////////////
#ifdef HAVE_MSR_CORE_H
	// PKG
	uint64_t MSR_PKG_DRAM_POWER_INFO_bits;
	read_msr_by_idx(0, MSR_PKG_POWER_INFO, &MSR_PKG_DRAM_POWER_INFO_bits);
	defaultSettings[systemType].maxPowerCapPKG = ((MSR_PKG_DRAM_POWER_INFO_bits & MSR_PKG_DRAM_POWER_INFO_max_power_mask) >> MSR_PKG_DRAM_POWER_INFO_max_power_shift) * RAPL::getUnit(RAPL_PCKG_POWER_LIMIT);
	defaultSettings[systemType].minPowerCapPKG = ((MSR_PKG_DRAM_POWER_INFO_bits & MSR_PKG_DRAM_POWER_INFO_min_power_mask) >> MSR_PKG_DRAM_POWER_INFO_min_power_shift) * RAPL::getUnit(RAPL_PCKG_POWER_LIMIT);
	defaultSettings[systemType].maxPowerCapPKGtime = (((MSR_PKG_DRAM_POWER_INFO_bits & MSR_PKG_DRAM_POWER_INFO_max_time_mask) >> MSR_PKG_DRAM_POWER_INFO_max_time_shift) * RAPL::getUnit(RAPL_POWER_LIMIT_TIMEWINDOW)) * 1000000;

	// DRAM
	read_msr_by_idx(0, MSR_DRAM_POWER_INFO, &MSR_PKG_DRAM_POWER_INFO_bits);
	defaultSettings[systemType].maxPowerCapRAM = ((MSR_PKG_DRAM_POWER_INFO_bits & MSR_PKG_DRAM_POWER_INFO_max_power_mask) >> MSR_PKG_DRAM_POWER_INFO_max_power_shift) * RAPL::getUnit(RAPL_PCKG_POWER_LIMIT);
	defaultSettings[systemType].minPowerCapRAM = ((MSR_PKG_DRAM_POWER_INFO_bits & MSR_PKG_DRAM_POWER_INFO_min_power_mask) >> MSR_PKG_DRAM_POWER_INFO_min_power_shift) * RAPL::getUnit(RAPL_PCKG_POWER_LIMIT);
	defaultSettings[systemType].maxPowerCapRAMtime = (((MSR_PKG_DRAM_POWER_INFO_bits & MSR_PKG_DRAM_POWER_INFO_max_time_mask) >> MSR_PKG_DRAM_POWER_INFO_max_time_shift) * RAPL::getUnit(RAPL_POWER_LIMIT_TIMEWINDOW)) * 1000000;
#endif
////////////////////////////////////////////////////////////////////////////////

	std::string defaultSettingFile = std::string(MERIC_PATH)+"/build/systemDefault.json";
	struct stat info;
	//read a file that holds the system's default settings
	//we have created this file during a previous MERIC run
	struct json_value_s* root = NULL;
	if(stat(defaultSettingFile.c_str(), &info) == 0)
	{
		std::ifstream jsonFile (defaultSettingFile);
		std::string line, jsonString = "";
		if (jsonFile.is_open())
		{
			while (getline(jsonFile,line))
			{
				jsonString += " " + line;
			}
			jsonFile.close();
		}
		const char * json = jsonString.c_str();
		struct json_value_s* root = json_parse(json, strlen(json));
		if (root ==  NULL)
		{
			std::cerr << "MERIC ERROR: invalid " << defaultSettingFile << std::endl;
			rename(defaultSettingFile.c_str(), (defaultSettingFile+".err").c_str());
		}
		else
		{
			struct json_object_element_s* data = ((struct json_object_s*)root->payload)->start;

			while(data != NULL)
			{
				MERIC_INFO << "<>"<< data->name->string <<"\n";
				if (std::string(data->name->string) == "DVFS")
				{
					struct json_object_element_s* dvfs = ((struct json_object_s*)data->value->payload)->start;
					do
					{
						MERIC_INFO << "\t* " << std::string(dvfs->name->string) << std::endl;
						if (std::string(dvfs->name->string) == "GOVERNOR")
						{
							defaultGovernor = std::string(((struct json_string_s*)dvfs->value->payload)->string);
						}
						else if (std::string(dvfs->name->string) == "MAX")
						{
							defaultSettings[systemType].maxCoreFreq = std::stol(std::string(((struct json_string_s*)dvfs->value->payload)->string));
						}
						else if (std::string(dvfs->name->string) == "MIN")
						{
							defaultSettings[systemType].minCoreFreq = std::stol(std::string(((struct json_string_s*)dvfs->value->payload)->string));
						}
						else if (std::string(dvfs->name->string) == "DEFAULT")
						{
							defaultSettings[systemType].defaultCoreFreq = std::stol(std::string(((struct json_string_s*)dvfs->value->payload)->string));
						}
						else if (std::string(dvfs->name->string) == "ENERGY_PERF_BIAS")
						{
							defaultSettings[systemType].IA32_ENERGY_PERF_BIAS_default = std::strtoull(std::string(((struct json_string_s*)dvfs->value->payload)->string).c_str(), NULL, 0);
						}
						dvfs = dvfs->next;
					}while(dvfs != NULL);
				}
				else if (std::string(data->name->string) == "UFS")
				{
					struct json_object_element_s* ufs = ((struct json_object_s*)data->value->payload)->start;
					do
					{
						if (std::string(ufs->name->string) == "MAX")
						{
							defaultSettings[systemType].maxUncoreFreq = std::stol(std::string(((struct json_string_s*)ufs->value->payload)->string));
						}
						else if (std::string(ufs->name->string) == "MIN")
						{
							defaultSettings[systemType].minUncoreFreq = std::stol(std::string(((struct json_string_s*)ufs->value->payload)->string));
						}
						ufs = ufs->next;
					}while(ufs != NULL);
					defaultSettings[systemType].defaultUncoreFreq = defaultSettings[systemType].maxUncoreFreq;
				}
				else if (std::string(data->name->string) == "PWRCAP")
				{
					struct json_object_element_s* pwr = ((struct json_object_s*)data->value->payload)->start;
					do
					{
						if (std::string(pwr->name->string) == "PKG")
						{
							defaultSettings[systemType].defaultPowerCapPKG = std::strtoull(std::string(((struct json_string_s*)pwr->value->payload)->string).c_str(), NULL, 0);
						}
						else if (std::string(pwr->name->string) == "DRAM")
						{
							defaultSettings[systemType].defaultPowerCapRAM = std::strtoull(std::string(((struct json_string_s*)pwr->value->payload)->string).c_str(), NULL, 0);
						}
						pwr = pwr->next;
					}while(pwr != NULL);
				}
				data = data->next;
			}
		}
	}
	
	if (root != NULL)
		free(root);
	else	//the json file does not exists or is corrupted - we have to read the default settings from the system
	{
// CORE freq -----------------------------------------------------------------//
		std::ifstream governorFile;
		governorFile.open("/sys/devices/system/cpu/cpu0/cpufreq/scaling_governor");
		governorFile >> defaultGovernor;
		governorFile.close();

#ifdef HAVE_MSR_CORE_H
		read_msr_by_idx(0, IA32_ENERGY_PERF_BIAS, &defaultSettings[systemType].IA32_ENERGY_PERF_BIAS_default);
#endif

		std::ifstream frequencyBounderyFile;
		std::string freqBound; //files have the value in KHz
		frequencyBounderyFile.open ("/sys/devices/system/cpu/cpu0/cpufreq/cpuinfo_max_freq");
		frequencyBounderyFile >> freqBound;
		frequencyBounderyFile.close();
		defaultSettings[systemType].maxCoreFreq = std::stol(freqBound) * 1000;
		defaultSettings[systemType].defaultCoreFreq = defaultSettings[systemType].maxCoreFreq;
		frequencyBounderyFile.open ("/sys/devices/system/cpu/cpu0/cpufreq/cpuinfo_min_freq");
		frequencyBounderyFile >> freqBound;
		frequencyBounderyFile.close();
		defaultSettings[systemType].minCoreFreq = std::stol(freqBound) * 1000;

// UnCORE freq ---------------------------------------------------------------//
#ifdef HAVE_MSR_CORE_H
		uint64_t MSR_UNCORE_RATIO_LIMIT_bits;
		read_msr_by_idx(0, MSR_UNCORE_RATIO_LIMIT, &MSR_UNCORE_RATIO_LIMIT_bits);

		defaultSettings[systemType].minUncoreFreq = (MSR_UNCORE_RATIO_LIMIT_bits & MSR_UNCORE_RATIO_LIMIT_max_mask) * 100000000;
		defaultSettings[systemType].defaultUncoreFreq = defaultSettings[systemType].maxUncoreFreq;
		defaultSettings[systemType].minUncoreFreq = ((MSR_UNCORE_RATIO_LIMIT_bits & MSR_UNCORE_RATIO_LIMIT_min_mask) >> MSR_UNCORE_RATIO_LIMIT_min_shift) * 100000000;
#elif defined HAVE_X86_ADAPT_H
		int fd = x86_adapt_get_device_ro(X86_ADAPT_DIE, 0);
		int adaptHandlerUncore = x86_adapt_lookup_ci_name(X86_ADAPT_DIE, "Intel_UNCORE_MAX_RATIO");
		uint64_t setting;
		x86_adapt_get_setting(fd, adaptHandlerUncore, &setting);
		defaultSettings[systemType].maxUncoreFreq = setting * 100000000;
		defaultSettings[systemType].defaultUncoreFreq = defaultSettings[systemType].maxUncoreFreq;
		adaptHandlerUncore = x86_adapt_lookup_ci_name(X86_ADAPT_DIE, "Intel_UNCORE_MIN_RATIO");
		x86_adapt_get_setting(fd, adaptHandlerUncore, &setting);
		defaultSettings[systemType].minUncoreFreq = setting * 100000000;
#endif

// POWERcap ------------------------------------------------------------------//
		uint64_t MSR_PKG_POWER_LIMIT_bits = 0x0;
		uint64_t MSR_DRAM_POWER_LIMIT_bits = 0x0;
#ifdef HAVE_MSR_CORE_H
		read_msr_by_idx(0, MSR_PKG_POWER_LIMIT, &MSR_PKG_POWER_LIMIT_bits);
		read_msr_by_idx(0, MSR_DRAM_POWER_LIMIT, &MSR_DRAM_POWER_LIMIT_bits);
#elif defined HAVE_X86_ADAPT_H
		int adaptRegisterHandler;
		uint64_t MSR_PKG_1_POWER_LIMIT_bits, MSR_PKG_2_POWER_LIMIT_bits;
		
		adaptRegisterHandler = x86_adapt_lookup_ci_name(X86_ADAPT_DIE, "Intel_RAPL_PKG_POWER_LIMIT_1");
		x86_adapt_get_setting(fd, adaptRegisterHandler, &MSR_PKG_1_POWER_LIMIT_bits);
		adaptRegisterHandler = x86_adapt_lookup_ci_name(X86_ADAPT_DIE, "Intel_RAPL_PKG_POWER_LIMIT_2");
		x86_adapt_get_setting(fd, adaptRegisterHandler, &MSR_PKG_2_POWER_LIMIT_bits);
		MSR_PKG_POWER_LIMIT_bits = MSR_PKG_1_POWER_LIMIT_bits | MSR_PKG_2_POWER_LIMIT_bits;
		
		adaptRegisterHandler = x86_adapt_lookup_ci_name(X86_ADAPT_DIE, "Intel_RAPL_RAM_POWER_LIMIT");
		x86_adapt_get_setting(fd, adaptRegisterHandler, &MSR_DRAM_POWER_LIMIT_bits);

		if (x86_adapt_put_device(X86_ADAPT_DIE, 0))
			std::cerr << "X86ADAPT RAPL INIT error while closing file descriptor\n";
#endif
		defaultSettings[systemType].defaultPowerCapPKG = MSR_PKG_POWER_LIMIT_bits;
		defaultSettings[systemType].defaultPowerCapRAM = MSR_DRAM_POWER_LIMIT_bits;
	}

#if defined HAVE_MSR_CORE_H || defined HAVE_X86_ADAPT_H
	if (defaultSettings[systemType].defaultUncoreFreq != -1)
		activatedUFS = true;
#endif
	if (defaultSettings[systemType].defaultPowerCapPKG != 0x0)
		activatedPWRCAP = true;
}

void Environment::storeSystemDefaultSettings()
{
	std::string defaultSettingFile = std::string(MERIC_PATH)+"/build/systemDefault.json";
	struct stat info;
	if(stat(defaultSettingFile.c_str(), &info ) != 0 && (activatedDVFS || activatedUFS || activatedPWRCAP))
	{
		std::ofstream defaultRestoreFile;
		defaultRestoreFile.open (defaultSettingFile);
	
		std::string content  = "{\n";
		content += "\t\"DVFS\" : {\n\t\t\"GOVERNOR\" : \"" + defaultGovernor + "\",\n"
			     + "\t\t\"MAX\" : " + std::to_string(defaultSettings[systemType].maxCoreFreq) + ",\n"
			     + "\t\t\"MIN\" : " + std::to_string(defaultSettings[systemType].minCoreFreq) + ",\n"
			     + "\t\t\"DEFAULT\" : " + std::to_string(defaultSettings[systemType].defaultCoreFreq) + ",\n"
			     + "\t\t\"ENERGY_PERF_BIAS\" : " + std::to_string(defaultSettings[systemType].IA32_ENERGY_PERF_BIAS_default) + "\n\t},\n";

		content += "\t\"UFS\" : {\n\t\t\"MAX\" : " + std::to_string(defaultSettings[systemType].maxUncoreFreq) + ",\n"
			     + "\t\t\"MIN\" : " + std::to_string(defaultSettings[systemType].minUncoreFreq) + "\n\t},\n";

		content+= "\t\"PWRCAP\" : {\n\t\t\"PKG\" : "  +std::to_string(defaultSettings[systemType].defaultPowerCapPKG) + ",\n"
			    + "\t\t\"DRAM\" : " +std::to_string(defaultSettings[systemType].defaultPowerCapRAM) +"\n\t}";
		content += "\n}\n";
		defaultRestoreFile << content;
		defaultRestoreFile.close();
	}
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
	initHWSizes();
	RAPL::initCountersUnits();
	systemType = system;
//SET AFFINITY
	#pragma omp parallel
	{
		cpu_set_t cpu_set;
		CPU_ZERO(&cpu_set);
		CPU_SET(sched_getcpu(), &cpu_set);
		sched_setaffinity(0, sizeof(cpu_set_t), &cpu_set);
	}

	if(!rank)
	{
		getSystemDefaultSettings();
//ACTIVATE DVFS
		std::ifstream governorFileI;
		if (defaultParameter.frequency != 0)
		{
			//using userspace governor
			if (defaultGovernor == "userspace")
			{
				activatedDVFS=true;
			}
			else
			{
				if (access("/sys/devices/system/cpu/cpu0/cpufreq/scaling_governor", W_OK) == 0)
				{
					governorFileI.open("/sys/devices/system/cpu/cpu0/cpufreq/scaling_available_governors");
					std::string availGovernor;
					while (governorFileI >> availGovernor)
					{
						if (availGovernor == "userspace")
						{
							activatedDVFS = true;
							break;
						}
					}
					governorFileI.close();
				}
				if (activatedDVFS == true)
				{
					std::ofstream governorFileO;
					for (int i=0; i<NODE_SIZE_CPU; i++)
					{
						governorFileO.open ("/sys/devices/system/cpu/cpu"+std::to_string(i)+"/cpufreq/scaling_governor");
						governorFileO << "userspace";
						governorFileO.close();
					}
#if !defined(HAVE_MSR_CORE_H) && !defined(HAVE_X86_ADAPT_H) && !defined(HAVE_CPUFREQ_H)
					if (access("/sys/devices/system/cpu/cpu0/cpufreq/scaling_setspeed", W_OK) != 0)
						activatedDVFS = false;
#endif
				}
			// Intel P-State
#ifdef HAVE_MSR_CORE_H
				else
				{
					if (access("/sys/devices/system/cpu/intel_pstate/no_turbo", W_OK) == 0 && access("/sys/devices/system/cpu/cpu0/cpufreq/scaling_max_freq", W_OK) == 0)
					{
						activatedDVFS = true;
						std::ofstream defSet;
						MERIC_INFO << "Disable turbo logic of Intel P-state driver\n";
						defSet.open ("/sys/devices/system/cpu/intel_pstate/no_turbo");
						defSet << "1";
						defSet.close();
						
						MERIC_INFO << "Disable scaling frequency of Intel P-state driver and set maximum P-state to all cores and set energy performance bias to maximum performance\n";
						for (int cpu=0; cpu<NODE_SIZE_CPU; cpu++)
						{
							defSet.open ("/sys/devices/system/cpu/cpu"+std::to_string(cpu)+"/cpufreq/scaling_max_freq");
							defSet << defaultSettings[systemType].minCoreFreq / 1000; //kHz
							defSet.close();
							write_msr_by_idx(cpu, IA32_ENERGY_PERF_BIAS, 0);
						}
						//p_state may still rewrite the IA32_PERF_CTL register during a short time period, so we wait
						usleep(800000); //700 ms is not enough
					}
				}
#endif
#ifdef HAVE_X86_ADAPT_H
				activatedDVFS = true;
#endif
			}
		}
//ACTIVATE UFS
		if (systemType == SYS_JETSON)
		{
			if (defaultParameter.uncoreFrequency != 0 && access("/sys/kernel/debug/clock/override.emc/state", W_OK) == 0)
			{
				std::ofstream governorFile;
				governorFile.open ("/sys/kernel/debug/clock/override.emc/state");
				governorFile << "1";
				governorFile.close();
				activatedUFS = true;
				MERIC_INFO << "MERIC: unCORE FREQUENCY scaling activated\n";
			}
		}
		
		storeSystemDefaultSettings();

		//set the settings that user want to apply for the application
		frequency(defaultParameter.frequency);
		uncoreFrequency(defaultParameter.uncoreFrequency, defaultParameter.uncoreFrequency);
		powercap(defaultParameter.powerCapPKG, defaultParameter.powerCapPKGtime);
	}
	threadNumber(defaultParameter.nthreads);
}


void Environment::frequency(uint64_t freq, int socket)
{
	timespec start_time, stop_time;
	clock_gettime(CLOCK_REALTIME, &start_time);

	if (freq == 0)
		return;

	if (!activatedDVFS)
		return;

	if (freq < defaultSettings[systemType].minCoreFreq || freq > defaultSettings[systemType].maxCoreFreq)
	{
		std::cerr << "CPU frequency has not changed to invalid value: " << freq << std::endl;
		return;
	}
/*	//would work if defaultCoreFreq is set at the nominal frequency, which is currently not
	else if (freq > defaultSettings[systemType].defaultCoreFreq)
	{
		MERIC_INFO << "Trying to apply turbo CPU frequency\n";
	}
*/
	usedDVFS = true;
	int nodeSize = NODE_SIZE_CPU;
	int cpu=0;
	if(socket != -1)
	{
		nodeSize = SOCKET_SIZE_CPU*(socket+1);
		cpu = SOCKET_SIZE_CPU*socket;
	}

	MERIC_INFO << "MERIC set frequency to: " << freq;
	if (socket != -1)
		MERIC_INFO << "; socket: " << socket;
	MERIC_INFO << "; cpu " << cpu << " - " << nodeSize-1;
	
#ifdef HAVE_MSR_CORE_H
	MERIC_INFO << "; using LIBMSR\n";
	for (; cpu<nodeSize; cpu++)
	{
		write_msr_by_idx(cpu, IA32_PERF_CTL, (freq/100000000) << IA32_PERF_CTL_shift);
	}
#elif defined HAVE_X86_ADAPT_H
	MERIC_INFO << "; using X86ADAPT\n";
	int adaptHandler = x86_adapt_lookup_ci_name(X86_ADAPT_CPU, "Intel_Target_PState");
	if (adaptHandler < 0)
	{
		std::cerr << "X86ADAPT ERROR: frequency settings not allowed\n";
		return;
	}

	for (; cpu<nodeSize; cpu++)
	{
		int fd = x86_adapt_get_device(X86_ADAPT_CPU, cpu);
		if (fd < 0)
		{
			std::cerr << "X86ADAPT ERROR: error while accessing file descriptor\n";
//			exit(2);
		}

		if(x86_adapt_set_setting(fd, adaptHandler, (freq/100000000) << IA32_PERF_CTL_shift) != 8)
		{
			std::cerr << "X86ADAPT ERROR: error while setting core frequecy\n";
//			exit(3);
		}

		if (x86_adapt_put_device(X86_ADAPT_CPU, cpu))
			std::cerr << "X86ADAPT ERROR: error while closing file descriptor\n";
	}
#elif defined HAVE_CPUFREQ_H
	MERIC_INFO << "; using CPUFREQ\n";
	for (; cpu < nodeSize; cpu++)
	{
		if(cpufreq_set_frequency(cpu,freq) != 0)
		{
			std::cerr << "CPUFREQ ERROR: failed to set frequency for cpu "<< cpu << std::endl;
		}
	}
	MERIC_INFO << "CPUFREQ: core frequency is " << cpufreq_get_freq_kernel(nodeSize-1) << std::endl;
#else
	std::ofstream coreFile;
	for (; cpu < nodeSize; cpu++)
	{
		coreFile.open ("/sys/devices/system/cpu/cpu"+std::to_string(cpu)+"/cpufreq/scaling_setspeed");
		coreFile << freq / 1000;
		coreFile.close();
	}
#endif
	clock_gettime(CLOCK_REALTIME, &stop_time);
	MERIC_INFO << "MERIC CPU core frequency changed in " << mutils::timeToDouble(stop_time - start_time)*1000 << " ms\n";
}

void Environment::uncoreFrequency(uint64_t maxFreq, uint64_t minFreq, int socket)
{
	timespec start_time, stop_time;
	clock_gettime(CLOCK_REALTIME, &start_time);

	if (maxFreq == 0 || minFreq == 0)
		return;
	
	if (!activatedUFS)
		return; //UFS not supported
		
	if (maxFreq > defaultSettings[systemType].maxUncoreFreq || minFreq < defaultSettings[systemType].minUncoreFreq || maxFreq < minFreq)
	{
		std::cerr << "Uncore frequency has not changed to invalid value: " << maxFreq << std::endl;
		MERIC_INFO << defaultSettings[systemType].maxUncoreFreq << " - " << defaultSettings[systemType].minUncoreFreq << std::endl;
		return;
	}
	usedUFS = true;
#if defined HAVE_MSR_CORE_H || defined HAVE_X86_ADAPT_H
	MERIC_INFO << "MERIC set uncore frequency to: "<< maxFreq;
	if (maxFreq != minFreq)
		MERIC_INFO << " - " << minFreq;
	int max = NODE_SIZE_SOCKET;
	int die=0;
	if (socket != -1 && socket < max)
	{
		die = socket;
		max = socket+1;
	}
	MERIC_INFO << "; soc " << die << " - " << max-1;
#endif

#ifdef HAVE_MSR_CORE_H
	MERIC_INFO << "; using LIBMSR\n";

	uint64_t MSR_UNCORE_RATIO_LIMIT_bits = ((minFreq/100000000) << MSR_UNCORE_RATIO_LIMIT_min_shift) + maxFreq/100000000;
	for (; die<max; die++)
	{
		write_msr_by_idx(die * SOCKET_SIZE_CPU, MSR_UNCORE_RATIO_LIMIT, MSR_UNCORE_RATIO_LIMIT_bits);
	}
#elif defined HAVE_X86_ADAPT_H
	MERIC_INFO << "; using X86ADAPT\n";
	if (x86_adapt_lookup_ci_name(X86_ADAPT_DIE, "Intel_UNCORE_MAX_RATIO") < 0)
	{
		std::cerr << "X86ADAPT ERROR: uncore frequency settings not allowed\n";
		return;
	}
	
	int adaptHandlerUncoreMax = x86_adapt_lookup_ci_name(X86_ADAPT_DIE, "Intel_UNCORE_MAX_RATIO");
	int adaptHandlerUncoreMin = x86_adapt_lookup_ci_name(X86_ADAPT_DIE, "Intel_UNCORE_MIN_RATIO");
	for (; die<max; die++)
	{
		int fd = x86_adapt_get_device(X86_ADAPT_DIE, die);
		if (fd < 0)
		{
			std::cerr << "X86ADAPT ERROR: error while accessing file descriptor\n";
//			exit(2);
		}

		auto setFreq = [&] (int adaptHandler, uint64_t freq)
		{
			if(x86_adapt_set_setting(fd, adaptHandler, freq) != 8)
			{
				std::cerr << "X86ADAPT ERROR: error while setting uncore frequency\n";
//				exit(3);
			}
		};

		setFreq(adaptHandlerUncoreMax, maxFreq/100000000);
		setFreq(adaptHandlerUncoreMin, minFreq/100000000);
	
		if (x86_adapt_put_device(X86_ADAPT_DIE, die))
			std::cerr << "X86ADAPT ERROR: error while closing file descriptor\n";
	}
#else
	// SYS_JETSON
	std::ofstream uncoreFile;
	uncoreFile.open ("/sys/kernel/debug/clock/override.emc/rate");
	uncoreFile << maxFreq;
	uncoreFile.close();
	MERIC_INFO << "MERIC set uncore frequency to: "<< maxFreq << std::endl;
#endif
	clock_gettime(CLOCK_REALTIME, &stop_time);
	MERIC_INFO << "MERIC uncore frequency changed in " << mutils::timeToDouble(stop_time - start_time)*1000 << " ms\n";
}

/**
 * RAPL power capping - PKG only
 * power [W]
 * timeWindow [us]
 *
 * NOTE: if power limit is not specified timeWindow is ignored
 **/
void Environment::powercap(uint64_t power, uint64_t timeWindow, int socket)
{
	if (power == 0)
		return;

#ifdef HAVE_MSR_CORE_H
	MERIC_INFO << "LIBMSR set powercap to: "<< power << " W";
#elif defined HAVE_X86_ADAPT_H
	MERIC_INFO << "X86ADAPT set powercap to: "<< power << " W";
#else
	return;
#endif
	if (timeWindow != 0)
		MERIC_INFO << " for " << timeWindow << " us";
	usedPWRCAP = true;
	int max = NODE_SIZE_SOCKET;
	int die=0;
	if (socket != -1 && socket < max)
	{
		die = socket;
		max = socket+1;
	}
	MERIC_INFO << "; soc " << die << " - " << max-1<< std::endl;
	
	uint64_t MSR_PKG_POWER_LIMIT_bits;
#ifdef HAVE_MSR_CORE_H
	read_msr_by_idx(0, MSR_PKG_POWER_LIMIT, &MSR_PKG_POWER_LIMIT_bits);
#elif defined HAVE_X86_ADAPT_H
	int adaptRegisterHandler, fd;
	fd = x86_adapt_get_device_ro(X86_ADAPT_DIE, 0);
	if (fd < 0)
	{
		std::cerr << "X86ADAPT RAPL INIT error while accessing file descriptor\n";
		return;
	}

	adaptRegisterHandler = x86_adapt_lookup_ci_name(X86_ADAPT_DIE, "Intel_RAPL_PKG_POWER_LIMIT_1");
	x86_adapt_get_setting(fd, adaptRegisterHandler, &MSR_PKG_POWER_LIMIT_bits);

	if (x86_adapt_put_device(X86_ADAPT_DIE, 0))
		std::cerr << "X86ADAPT RAPL INIT error while closing file descriptor\n";
#endif
	
	MSR_PKG_POWER_LIMIT_bits = (MSR_PKG_POWER_LIMIT_bits & (~MSR_PKG_POWER_LIMIT_mask)) | (((uint64_t)(power / RAPL::getUnit(RAPL_PCKG_POWER_LIMIT))) & MSR_PKG_POWER_LIMIT_mask) | MSR_PKG_POWER_LIMIT_enable_mask;

	if (timeWindow != 0)
	{
		/** PKG Time window size equation
		 * Time limit = 2^Y * (1.0 + Z/4.0) * Time_Unit
		 * Y = 0 - 31
		 * Z = 0 - 3 => 1.0 - 1.75
		 * DRAM uses different equation
		 **/
		uint64_t Y = 0;
		uint64_t Z = 0;
		timeWindow = timeWindow/(1000000 * RAPL::getUnit(RAPL_POWER_LIMIT_TIMEWINDOW));
		if (timeWindow > 1)
		{
			Y = log2(timeWindow);
			int baseY = pow(2,Y);
			int baseZ = baseY/4;
			int ZLimitPart = timeWindow - baseY;
			Z = ZLimitPart / baseZ;
			if (ZLimitPart % baseZ > (baseZ/2))
				Z++;
		}
		MSR_PKG_POWER_LIMIT_bits = (MSR_PKG_POWER_LIMIT_bits & (~(MSR_PKG_POWER_LIMIT_time_z_mask | MSR_PKG_POWER_LIMIT_time_y_mask))) | 
			(Y << MSR_PKG_POWER_LIMIT_time_y_shift) | (Z << MSR_PKG_POWER_LIMIT_time_z_shift);
	}

	for (; die<max; die++)
	{
#ifdef HAVE_MSR_CORE_H
		write_msr_by_idx(die * SOCKET_SIZE_CPU, MSR_PKG_POWER_LIMIT, MSR_PKG_POWER_LIMIT_bits);
#elif defined HAVE_X86_ADAPT_H
		fd = x86_adapt_get_device_ro(X86_ADAPT_DIE, die);
		if (fd < 0)
		{
			std::cerr << "X86ADAPT RAPL INIT error while accessing file descriptor\n";
			return;
		}
		
		adaptRegisterHandler = x86_adapt_lookup_ci_name(X86_ADAPT_DIE, "Intel_RAPL_PKG_POWER_LIMIT_1");
		x86_adapt_set_setting(fd, adaptRegisterHandler, MSR_PKG_POWER_LIMIT_bits);

		if (x86_adapt_put_device(X86_ADAPT_DIE, die))
			std::cerr << "X86ADAPT ERROR: error while closing file descriptor\n";
#endif
	}
}

/**
 * sets default governor, CPU core frequency, CPU uncore frequency limits and powercap limits
 **/
void Environment::restoreSystemSettings()
{
	if (activatedDVFS)
	{
		if (usedDVFS)
			frequency(defaultSettings[systemType].defaultCoreFreq);
		if (defaultGovernor != "userspace" && access("/sys/devices/system/cpu/cpu0/cpufreq/scaling_governor", W_OK) == 0)
		{
			std::ofstream governorFile;
			for (int i=0; i<NODE_SIZE_CPU; i++)
			{
				governorFile.open ("/sys/devices/system/cpu/cpu"+std::to_string(i)+"/cpufreq/scaling_governor");
				governorFile << defaultGovernor;
				governorFile.close();
			}
		}
#ifdef HAVE_MSR_CORE_H
		std::ofstream defSet;
		MERIC_INFO << "Enable turbo logic of Intel P-state driver\n";
		defSet.open ("/sys/devices/system/cpu/intel_pstate/no_turbo");
		defSet << "0";
		defSet.close();
		
		MERIC_INFO << "Enable scaling frequency of Intel P-state driver and set default energy performance bias\n";
		for (int cpu=0; cpu<NODE_SIZE_CPU; cpu++)
		{
			defSet.open ("/sys/devices/system/cpu/cpu"+std::to_string(cpu)+"/cpufreq/scaling_max_freq");
			defSet << defaultSettings[systemType].maxCoreFreq / 1000;
			defSet.close();
			write_msr_by_idx(cpu, IA32_ENERGY_PERF_BIAS, defaultSettings[systemType].IA32_ENERGY_PERF_BIAS_default);
		}
#endif
	}

	if (activatedUFS)
	{
		if (systemType == SYS_JETSON)
		{
			if(usedUFS)
				uncoreFrequency(defaultSettings[SYS_JETSON].defaultUncoreFreq, defaultSettings[SYS_JETSON].defaultUncoreFreq);	//TODO remove the last reason to use defaultSettings[SYS_JETSON]
			//uncore
			std::ofstream memTuningFile;
			memTuningFile.open ("/sys/kernel/debug/clock/override.emc/state");
			memTuningFile << "0";
			memTuningFile.close();
		}
		else
		{
			if(usedUFS)
				uncoreFrequency(defaultSettings[systemType].maxUncoreFreq, defaultSettings[systemType].minUncoreFreq);
		}
	}

	if (usedPWRCAP)
	{
#ifdef HAVE_MSR_CORE_H
		for (int die=0; die<NODE_SIZE_SOCKET; die++)
		{
			write_msr_by_idx(die * SOCKET_SIZE_CPU, MSR_PKG_POWER_LIMIT, defaultSettings[SYS_OTHER].defaultPowerCapPKG);
//			write_msr_by_idx(die * SOCKET_SIZE_CPU, MSR_DRAM_POWER_LIMIT, defaultSettings[SYS_OTHER].defaultPowerCapRAM);
		}
#elif defined HAVE_X86_ADAPT_H
		for (int die=0; die<NODE_SIZE_SOCKET; die++)
		{
			const int fd = x86_adapt_get_device_ro(X86_ADAPT_DIE, die);
			if (fd < 0)
			{
				std::cerr << "X86ADAPT RAPL INIT error while accessing file descriptor\n";
				return;
			}
			int adaptRegisterHandler;
			uint64_t POWER_LIMIT_bits = defaultSettings[SYS_OTHER].defaultPowerCapPKG & MSR_PKG_POWER_LIMIT_mask;
			adaptRegisterHandler = x86_adapt_lookup_ci_name(X86_ADAPT_DIE, "Intel_RAPL_PKG_POWER_LIMIT_1");
			x86_adapt_set_setting(fd, adaptRegisterHandler, POWER_LIMIT_bits);
			
			POWER_LIMIT_bits = defaultSettings[SYS_OTHER].defaultPowerCapPKG >> MSR_PKG_POWER_LIMIT_power2_shift;
			adaptRegisterHandler = x86_adapt_lookup_ci_name(X86_ADAPT_DIE, "Intel_RAPL_PKG_POWER_LIMIT_2");
			x86_adapt_set_setting(fd, adaptRegisterHandler, POWER_LIMIT_bits);
			
//			adaptRegisterHandler = x86_adapt_lookup_ci_name(X86_ADAPT_DIE, "Intel_RAPL_RAM_POWER_LIMIT");
//			x86_adapt_set_setting(fd, adaptRegisterHandler, &defaultSettings[SYS_OTHER].defaultPowerCapRAM);

			if (x86_adapt_put_device(X86_ADAPT_DIE, die))
				std::cerr << "X86ADAPT ERROR: error while closing file descriptor\n";
		}
#endif
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


