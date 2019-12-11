#ifndef SRC_WRAPPER_MSRS_H_
#define SRC_WRAPPER_MSRS_H_

/**
 * List of MSRs used in MERIC with official description from Intel documentation
 **/

#if defined HAVE_MSR_CORE_H
	#include <msr_core.h>
#endif

////////////////////////////////////////////////////////////////////////////////

/**
 * IA32_ENERGY_PERF_BIAS register *********************************************
 * Performance Energy Bias Hint (R/W)
 * 0x1B0 0x000000000000000F
 * Power Policy Preference (bits 3:0):
 	0 indicates preference to highest performance.
	15 indicates preference to maximize energy saving.
 * Reserved (bits 63:4)
 *
 * MSR_PLATFORM_INFO register *************************************************
 * Platform Information: Contains power management and other model specific 
 * features enumeration. See http://biosbits.org.
 * 0xCE 0x000000000000FF00
 * Reserved (bits 7:0)
 * Maximum Non-Turbo Ratio (R/O) (bits 15:8):
 	This is the ratio of the maximum frequency that does not require turbo. 
 	Frequency = ratio * Scalable Bus Frequency.
 * Reserved (bits 63:16)
 ******************************************************************************
 **/
#ifndef IA32_ENERGY_PERF_BIAS
	#define IA32_ENERGY_PERF_BIAS	0x1B0
#endif

#ifndef MSR_PLATFORM_INFO
	#define MSR_PLATFORM_INFO   	0xCE
#endif
const uint64_t MSR_PLATFORM_INFO_mask  = 0xFF00;
const uint64_t MSR_PLATFORM_INFO_shift = 8;

// CORE FREQUENCY //////////////////////////////////////////////////////////////
/**
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
 * IA32_MPERF register ********************************************************
 * TSC Frequency Clock Counter (R/Write to clear)
 * 0xE7 0xFFFFFFFFFFFFFFFF
 * C0_MCNT: C0 TSC Frequency Clock Count (bits 63:0):
 	Increments at fixed interval (relative to TSC freq.) when the logical 
 	processor is in C0. Cleared upon overflow / wrap-around of IA32_APERF.
 *
 * IA32_APERF register ********************************************************
 * Actual Performance Clock Counter (R/Write to clear)
 * 0xE8 0xFFFFFFFFFFFFFFFF
 * C0_ACNT: C0 Actual Frequency Clock Count (bits 63:0):
 	Accumulates core clock counts at the coordinated clock frequency, when the 
 	logical processor is in C0. Cleared upon overflow / wrap-around of 
 	IA32_MPERF.
 ******************************************************************************
 **/

#ifndef IA32_PERF_CTL
	#define IA32_PERF_CTL       	0x199
#endif
const uint64_t IA32_PERF_CTL_shift = 8;

#ifndef IA32_MPERF
	#define IA32_MPERF				0xE7
#endif

#ifndef IA32_APERF
	#define IA32_APERF				0xE8
#endif

// UNCORE FREQUENCY ////////////////////////////////////////////////////////////
/**
 * Intel CPU uncore frequency
 * From Intel 64 and IA-32 Architectures Software Developer's Manual 
 * Volume 4: Model-Specific Registers
 *
 * MSR_UNCORE_RATIO_LIMIT register ********************************************
 * Uncore Ratio Limit (R/W)
 * Out of reset, the min_ratio and max_ratio fields represent the widest 
 * possible range of uncore frequencies. Writing to these fields allows software 
 * to control the minimum and the maximum frequency that hardware will select.
 * 0x620 0x0000000000007F7F
 * MAX_RATIO (bits 6:0):
 	This field is used to limit the max ratio of the LLC/Ring.
 * Reserved (bit 7)
 * MIN_RATIO (bits 14:8):
 	Writing to this field controls the minimum possible ratio of the LLC/Ring.
 * Reserved (bits 63:15)
 *
 **/
/**
 * Following from Intel Xeon Processor E5 v2 and E7 v2 Product Families Uncore 
 * Performance Monitoring Reference Manual (Reference Number: 329468-002)
 * The following registers represent state governing all PMUs in the uncore, 
 * both to exert global control and collect box-level information.
 *
 * U_MSR_PMON_GLOBAL_CTL register *********************************************
 * UBox PMON Global Control (32 bit register) Contains a bit that can freeze 
 * (.frz_all) all the uncore counters. If an overflow is detected in any of the 
 * uncore's PMON registers, it will be summarized in U_MSR_PMON_GLOBAL_STATUS.
 * 0x700 0xE0007FFF since HSW
 * 0xC00 0xE0007FFF IVB EP and SDB EP chips 
 * PMI Core Select (bits 14:0):
 	Ex:
 	If counter overflow is sent to UBox...
 	000000000000000 - No PMI sent
 	000000000000001 - Send PMI to core 0
 	000000001000000 - Send PMI to core 6
 	000000001100010 - Send PMI to core 2, 5 & 6
 	etc.
 	NOTE: If wk_on_pmi is set to 1, a wake will be sent to any sleeping core in 
 	the mask prior to sending the PMI.
 * Reserved (bits 26:15)
 * Reserved (bits 28:27)
 * unfrz_all (bit 29):
 	Unfreeze all uncore performance monitors.
 * wk_on_pmi (bit 30):
 	If PMI event requested to send to core...
 	0 - Send event to cores already woken
 	1 - Wake any sleeping core and send PMI to all cores.
 * frz_all (bit 31):
 	Freeze all uncore performance monitors.
 *
 * U_MSR_PMON_GLOBAL_STATUS register ******************************************
 * This register accumulates overflows sent to it from the other uncore boxes. 
 * To reset these overflow bits, a user must set the corresponding bits in 
 * U_MSR_PMON_GLOBAL_STATUS to 1, which will act to clear them. 
 * (32 bit register)
 * 0x701 0x07FFFFFF since HSW
 * 0xC01 0x07FFFFFF IVB EP and SDB EP chips
 * ov_u_fixed (bit 0):
 	Set if overflow is detected from UBox fixed PMON register. 
 	NOTE: Write of '1' will clear the bit.
 * ov_u (bit 1):
 	Set if overflow is detected from a UBox PMON register.
 	NOTE: Write of '1' will clear the bit.
 * ov_p (bit 2):
 	Set if overflow is detected from a PCU PMON register.
 	NOTE: Write of '1' will clear the bit.
 * ov_c[14-0] (bits 17:3):
 	Set if overflow is detected from a CBo PMON register, 1 bit for each CBo 
 	where bit 5 corresponds CBo 0, etc.
 	NOTE: Write of '1' will clear the bit.
 * ov_h0 (bit 18):
 	Set if overflow is detected from an HA0 PMON register.
 	NOTE: Write of '1' will clear the bit.
 * ov_h1 (bit 19):
 	Set if overflow is detected from an HA1 PMON register.
 	NOTE: Write of '1' will clear the bit.
 * ov_m0 (bit 20):
 	Set if overflow is detected from an iMC0 PMON register.
 	NOTE: Write of '1' will clear the bit.
 * ov_m1 (bit 21):
 	Set if overflow is detected from an iMC1 PMON register.
 	NOTE: Write of '1' will clear the bit.
 * ov_q0 (bit 22):
 	Set if overflow is detected from a QPI0 PMON register.
 	NOTE: Write of '1' will clear the bit.
 * ov_q1 (bit 23):
 	Set if overflow is detected from a QPI1 PMON register.
 	NOTE: Write of '1' will clear the bit.
 * ov_rq0 (bit 24):
 	Set if overflow is detected from an R3QPI0 PMON register.
 	NOTE: Write of '1' will clear the bit.
 * ov_rq1 (bit 25):
 	Set if overflow is detected from an R3QPI1 PMON register.
 	NOTE: Write of '1' will clear the bit.
 * ov_rp (bit 26):
 	Set if overflow is detected from an R2PCIe PMON register.
 	NOTE: Write of '1' will clear the bit.
 * Reserved (31:27)
 *
 * U_MSR_PMON_FIXED_CTL register **********************************************
 * U-Box PMON UCLK Fixed Counter Control (32 bit register)
 * 0x703 0x00500000 since HSW
 * 0xC08 0x00500000 IVB EP and SDB EP chips
 * Reserved (bits 19:0)
 * ov_en (bit 20):
 	When this bit is set to 1 and the corresponding counter overflows, a UBox 
 	overflow message is sent to the UBox's global logic. Once received, the 
 	global status register will record the overflow in 
 	U_MSR_PMON_GLOBAL_STATUS.ov_u_fixed.
 * Reserved (bit 21)
 * Enable (bit 22):
 	Enable counter when global enable is set.
 * Reserved (bits 31:23)
 *
 * U_MSR_PMON_FIXED_CTR register **********************************************
 * U-Box PMON UCLK Fixed Counter
 * 0x704 0x0000FFFFFFFFFFFF since HSW
 * 0xC09 0x0000FFFFFFFFFFFF IVB EP and SDB EP chips
 * Event count (bits 47:0):
 	48-bit performance event counter
 * Reserved (63:48)
 ******************************************************************************
 **/
const uint64_t MSR_UNCORE_RATIO_LIMIT_max_mask  = 0x7F;
const uint64_t MSR_UNCORE_RATIO_LIMIT_min_mask  = 0x7F00;
const uint64_t MSR_UNCORE_RATIO_LIMIT_min_shift = 8;

const uint64_t U_MSR_PMON_FIXED_CTL_shift       = 22;

#ifndef MSR_UNCORE_RATIO_LIMIT
	#define MSR_UNCORE_RATIO_LIMIT  0x620
#endif

#ifndef MSR_UNCORE_CURRENT_RATIO
	#define MSR_UNCORE_CURRENT_RATIO	0x621
#endif

#ifndef U_MSR_PMON_FIXED_CTR
	#define U_MSR_PMON_FIXED_CTR	0x704	//since HSW
//	#define U_MSR_PMON_FIXED_CTR	0xC09	//IVB EP and SDB EP chips
#endif

#ifndef U_MSR_PMON_FIXED_CTL
	#define U_MSR_PMON_FIXED_CTL	0x703	//since HSW
//	#define U_MSR_PMON_FIXED_CTL	0xC08	//IVB EP and SDB EP chips
#endif

// POWER-CAP ///////////////////////////////////////////////////////////////////
/**
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
 * 0x610 0x00FFFFFF00FFFFFF
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
 * 0x618 0x0000000000FFFFFF
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
 * MSR_PKG_POWER_INFO/MSR_DRAM_POWER_INFO register ****************************
 * Read-only MSR. It reports the package/DRAM power range information for RAPL 
 * usage. This MSR provides maximum/minimum values (derived from electrical 
 * specification), thermal specification power of the package/DRAM domain. It 
 * also provides the largest possible time window for software to program the 
 * RAPL interface.
 * 0x614 0x003F7FFF7FFF7FFF MSR_PKG_POWER_INFO
 * 0x61C 0x003F7FFF7FFF7FFF MSR_DRAM_POWER_INFO
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
 * 0x613 0x00000000FFFFFFFF MSR_PKG_PERF_STATUS
 * 0x61B 0x00000000FFFFFFFF MSR_DRAM_PERF_STATUS
 * Accumulated Package Throttled Time (bits 31:0):
	The unsigned integer value represents the cumulative time (since the last 
	time this register is cleared) that the package/DRAM domain has throttled. 
	The unit of this field is specified by the "Time Units" field of 
	MSR_RAPL_POWER_UNIT.
 * Reserved (bits 63:32)
 ******************************************************************************
 **/
const uint64_t MSR_DRAM_POWER_LIMIT_mask = 0x7FFF;
const uint64_t MSR_DRAM_POWER_LIMIT_enable_mask  = 0x8000;
const uint64_t MSR_DRAM_POWER_LIMIT_enable_shift = 15;
const uint64_t MSR_DRAM_POWER_LIMIT_clamping_mask = 0x10000;
const uint64_t MSR_DRAM_POWER_LIMIT_clamping_shift = 16;
const uint64_t MSR_DRAM_POWER_LIMIT_time_f_mask  = 0xC00000;
const uint64_t MSR_DRAM_POWER_LIMIT_time_y_mask  = 0x3E0000;
const uint64_t MSR_DRAM_POWER_LIMIT_time_f_shift = 22;
const uint64_t MSR_DRAM_POWER_LIMIT_time_y_shift = 17;

const uint64_t MSR_PKG_POWER_LIMIT_mask = 0x7FFF;
const uint64_t MSR_PKG_POWER_LIMIT_shift = 0;
const uint64_t MSR_PKG_POWER_LIMIT_enable_mask = 0x8000;
const uint64_t MSR_PKG_POWER_LIMIT_enable_shift = 15;
const uint64_t MSR_PKG_POWER_LIMIT_clamping_mask = 0x10000;
const uint64_t MSR_PKG_POWER_LIMIT_clamping_shift = 16;
const uint64_t MSR_PKG_POWER_LIMIT_time_y_mask = 0x3E0000;
const uint64_t MSR_PKG_POWER_LIMIT_time_y_shift = 17;
const uint64_t MSR_PKG_POWER_LIMIT_time_z_mask = 0xC00000;
const uint64_t MSR_PKG_POWER_LIMIT_time_z_shift = 22;

const uint64_t MSR_PKG_POWER_LIMIT_power1_shift = 0;
const uint64_t MSR_PKG_POWER_LIMIT_power2_shift = 32; //use "half register-length" shift instead of specifying all the mask and shifts for the second half of the register

const uint64_t MSR_PKG_POWER_LIMIT_lock_mask  = 0x8000000000000000;
const uint64_t MSR_DRAM_POWER_LIMIT_lock_mask = 0x80000000;

const uint64_t MSR_PKG_DRAM_POWER_INFO_min_power_mask = 0x7FFF0000;
const uint64_t MSR_PKG_DRAM_POWER_INFO_min_power_shift = 16;
const uint64_t MSR_PKG_DRAM_POWER_INFO_max_power_mask = 0x7FF700000000;
const uint64_t MSR_PKG_DRAM_POWER_INFO_max_power_shift = 32;
const uint64_t MSR_PKG_DRAM_POWER_INFO_max_time_mask = 0x3F000000000000;
const uint64_t MSR_PKG_DRAM_POWER_INFO_max_time_shift = 48;

#ifndef MSR_PKG_POWER_LIMIT
	#define MSR_PKG_POWER_LIMIT 	0x610
#endif

#ifndef MSR_PKG_POWER_INFO
	#define MSR_PKG_POWER_INFO  	0x614
#endif

#ifndef MSR_PKG_PERF_STATUS
	#define MSR_PKG_PERF_STATUS 	0x613
#endif

#ifndef MSR_DRAM_POWER_LIMIT
	#define MSR_DRAM_POWER_LIMIT 	0x618
#endif

#ifndef MSR_DRAM_POWER_INFO
	#define MSR_DRAM_POWER_INFO 	0x61C
#endif

#ifndef MSR_DRAM_PERF_STATUS
	#define MSR_DRAM_PERF_STATUS 	0x61B
#endif


// RAPL energy measurement /////////////////////////////////////////////////////
/**
 * Intel RAPL energy consumption mearurement registers description
 * From Intel 64 and IA-32 Architectures Software Developer's Manual
 *
 * Intel_RAPL_Pckg_Energy - all energy consumptions
 * Intel_RAPL_PP0_Energy  - CPU
 * Intel_RAPL_PP1_Energy  - may reflect to uncore devices
 * Intel_RAPL_RAM_Energy  - RAM
 *
 * Intel_RAPL_Pckg_Energy ~ Intel_RAPL_PP0_Energy + Intel_RAPL_PP1_Energy
 * Intel_RAPL_Pckg_Energy ~ Intel_RAPL_PP0_Energy + Intel_RAPL_RAM_Energy
 * 
 *
 * MSR_RAPL_POWER_UNIT Register ***********************************************
 * Unit Multipliers used in RAPL Interfaces (R/O)
 * 0x606 0x00000000000F1F0F
 * Power Units (bits 3:0):
 	Power related information (in Watts) is based on the multiplier, 1/2^PU; 
 	where PU is an unsigned integer represented by bits 3:0. 
 	Default value is 0011b, indicating power unit is in 1/8 Watts increment.
 * Reserved (bits 7:4)
 * Energy Status Units (bits 12:8): 
 	Energy related information (in Joules) is based on the multiplier, 1/2^ESU; 
 	where ESU is an unsigned integer represented by bits 12:8. 
 	Default value is 10000b, indicating energy status unit is 15.3uJ increment.
 * Reserved (bits 15:13)
 * Time Units (bits 19:16): 
	Time related information (in Seconds) is based on the multiplier, 1/2^TU; 
	where TU is an unsigned integer represented by bits 19:16. 
	Default value is 1010b, indicating time unit is in 976us increment.
 * Reserved (bits 63:20)
 *
 * MSR_PKG_ENERGY_STATUS register *********************************************
 * 0x611 0x00000000FFFFFFFF
 * Total energy consumed (bits 31:0):
	The unsigned integer value represents the total amount of energy consumed 
	since that last time this register is cleared. The unit of this field 
	is specified by the "Energy Status Units" field of MSR_RAPL_POWER_UNIT.
 * Reserved (bits 63:32)
 *
 * MSR_PP0_ENERGY_STATUS/MSR_PP1_ENERGY_STATUS register ***********************
 * 0x639 0x00000000FFFFFFFF MSR_PP0_ENERGY_STATUS
 * 0x641 0x00000000FFFFFFFF MSR_PP1_ENERGY_STATUS
 * Total Energy Consumed (bits 31:0): 
	The unsigned integer value represents the total amount of energy consumed 
	since the last time this register was cleared. The unit of this field 
	is specified by the "Energy Status Units" field of MSR_RAPL_POWER_UNIT.
 * Reserved (bits 63:32)
 *
 * MSR_DRAM_ENERGY_STATUS register ********************************************
 * 0x619 0x00000000FFFFFFFF
 * Total Energy Consumed (bits 31:0):
	The unsigned integer value represents the total amount of energy consumed 
	since that last time this register is cleared. The unit of this field 
	is specified by the "Energy Status Units" field of MSR_RAPL_POWER_UNIT.
 * Reserved (bits 63:32)
 *
 * MSR_PLATFORM_ENERGY_COUNTER register ***************************************
 * Additional MSRs Supported by 6th Generation Intel Core Processors and the 
 * Intel Xeon Processor Scalable Family Based on Skylake Microarchitecture, 7th 
 * Generation Intel Core Processors Based on Kaby Lake Microarchitecture, 8th 
 * Generation and 9th Generation Intel Core Processors and Intel Xeon E 
 * Processors Based on Coffee Lake Microarchitecture, and 8th Generation Intel 
 * Core i3 Processors Based on Cannon Lake Microarchitecture
 * Platform Energy Counter (R/O) - platform scope
 * This MSR is valid only if both platform vendor hardware implementation and 
 * BIOS enablement support it. This MSR will read 0 if not valid.
 * 0x64D 0x00000000FFFFFFFF
 * Total energy consumed (bits 31:0):
 	Total energy consumed by all devices in the platform that receive power from 
 	integrated power delivery mechanism, included platform devices are processor 
 	cores, SOC, memory, add-on or peripheral devices that get powered directly 
 	from the platform power delivery means. The energy units are specified in 
 	the MSR_RAPL_POWER_UNIT.Enery_Status_Unit.
 * Reserved (bits 63:32)
 **/
const uint64_t MSR_RAPL_POWER_UNIT_power_mask   = 0xF;
const uint64_t MSR_RAPL_POWER_UNIT_energy_mask  = 0x1F00;
const uint64_t MSR_RAPL_POWER_UNIT_energy_shift = 8;
const uint64_t MSR_RAPL_POWER_UNIT_time_mask    = 0xF0000;
const uint64_t MSR_RAPL_POWER_UNIT_time_shift   = 16;
const uint64_t MSR_PKG_DRAM_PPx_ENERGY_STATUS_mask = 0xFFFFFFFF;
//const uint64_t MSR_DRAM_ENERGY_STATUS_mask    = 0xFFFFFFFF;
//const uint64_t MSR_PP0_ENERGY_STATUS_mask     = 0xFFFFFFFF;
//const uint64_t MSR_PP1_ENERGY_STATUS_mask     = 0xFFFFFFFF;

#ifndef MSR_RAPL_POWER_UNIT
	#define MSR_RAPL_POWER_UNIT 	0x606
#endif

#ifndef MSR_PKG_ENERGY_STATUS
	#define MSR_PKG_ENERGY_STATUS 	0x611
#endif

#ifndef MSR_PP0_ENERGY_STATUS
	#define MSR_PP0_ENERGY_STATUS 	0x639
#endif

#ifndef MSR_PP1_ENERGY_STATUS
	#define MSR_PP1_ENERGY_STATUS 	0x641
#endif

#ifndef MSR_DRAM_ENERGY_STATUS
	#define MSR_DRAM_ENERGY_STATUS 	0x619
#endif

#ifndef MSR_PLATFORM_ENERGY_COUNTER
	#define MSR_PLATFORM_ENERGY_COUNTER 0x64D
#endif



////////////////////////////////////////////////////////////////////////////////

const std::map<uint64_t, uint64_t> msr_whitelist = {
	{ MSR_PLATFORM_INFO,      0x0000FF007080FF00 },	//MSR_PLATFORM_INFO/PLATFORM_INFO_ADDR
	{ U_MSR_PMON_FIXED_CTR,   0x0000FFFFFFFFFFFF }, //U_MSR_PMON_FIXED_CTR/MSR_U_PMON_UCLK_FIXED_CTR
	{ U_MSR_PMON_FIXED_CTL,   0x00500000         }, //U_MSR_PMON_FIXED_CTL/MSR_U_PMON_UCLK_FIXED_CTL
	{ MSR_UNCORE_RATIO_LIMIT, 0x0000000000007F7F },
	{ MSR_PKG_POWER_LIMIT,    0x00FFFFFF00FFFFFF },
	{ MSR_PKG_POWER_INFO,     0x003F7FFF7FFF7FFF },
	{ MSR_PKG_PERF_STATUS,    0x00000000FFFFFFFF },
	{ MSR_DRAM_POWER_LIMIT,   0x0000000000FFFFFF },
	{ MSR_DRAM_POWER_INFO,    0x003F7FFF7FFF7FFF },
	{ MSR_DRAM_PERF_STATUS,   0x00000000FFFFFFFF },
	{ IA32_ENERGY_PERF_BIAS,  0x000000000000000F },
	{ IA32_PERF_CTL,          0x000000010000FFFF },
	{ IA32_MPERF,             0xFFFFFFFFFFFFFFFF },
	{ IA32_APERF,             0xFFFFFFFFFFFFFFFF },
	{ MSR_RAPL_POWER_UNIT,    0x00000000000F1F0F },
	{ MSR_PKG_ENERGY_STATUS,  0x00000000FFFFFFFF },
	{ MSR_DRAM_ENERGY_STATUS, 0x00000000FFFFFFFF }
//	{ MSR_PP0_ENERGY_STATUS,  0x00000000FFFFFFFF }
//	{ MSR_PP1_ENERGY_STATUS,  0x00000000FFFFFFFF }
//	{ MSR_PLATFORM_ENERGY_COUNTER, 0x00000000FFFFFFFF } // not yet supportd by MERIC
};

#endif /* SRC_WRAPPER_MSRS_H_ */
