<img
  src="https://code.it4i.cz/vys0053/meric/uploads/e96a5c46ce9fa0e52d163945d992429c/MERIC_logo_green.png"
  title="MERIC logo"
  width="500">

# MERIC energy-efficient runtime system #

Lightweight C/C++ library with Fortran and Python interface for HPC applications dynamic behavior detection with a goal in energy consumption reduction - applying [READEX](https://www.readex.eu/) approach.

The library was originally developed for Intel x86 systems (tested on HSW, BDW, KNL, SKL, CSL, ICX, and SPR) but additionally supports AMD EPYC (17h and 19h), IBM OpenPOWER (8 and 9), selected ARM systems (e.g., Fujitsu A64FX, Grace), NVIDIA GPUs (Volta, Ampere, Hopper, Blackwell), and AMD GPUs (MI2xx).

--------------------------------------------------------------------------------
#      README Content                                                          #
--------------------------------------------------------------------------------
<!---
	README format notes
	- only Heading level 1, 2 and 5 are used
-->

<!--- master branch links -->
 1. [System parameters tuning](https://code.it4i.cz/energy-efficiency/meric-suite/meric#1-system-parameters-tuning)
 2. [Supported energy measurement systems](https://code.it4i.cz/energy-efficiency/meric-suite/meric#2-supported-energy-measurement-systems)
 3. [MERIC API](https://code.it4i.cz/energy-efficiency/meric-suite/meric#3-meric-api)
 4. [TIMEPROF library](https://code.it4i.cz/energy-efficiency/meric-suite/meric#4-timeprof-library)
 5. [MERIC binary instrumentation](https://code.it4i.cz/energy-efficiency/meric-suite/meric#5-meric-binary-instrumentation)
 6. [Compilation](https://code.it4i.cz/energy-efficiency/meric-suite/meric#6-compilation)
 7. [MERIC input parameters](https://code.it4i.cz/energy-efficiency/meric-suite/meric#7-meric-input-parameters)
 8. [Content of test folder and example application run](https://code.it4i.cz/energy-efficiency/meric-suite/meric#8-content-of-test-folder-and-example-application-run)
 9. [Analysis of MERIC output files](https://code.it4i.cz/energy-efficiency/meric-suite/meric#9-analysis-of-meric-output-files)
10. [MERIC with a Fortran code](https://code.it4i.cz/energy-efficiency/meric-suite/meric#10-meric-with-a-fortran-code)
11. [MERIC with a Python code](https://code.it4i.cz/energy-efficiency/meric-suite/meric#11-meric-with-a-python-code)
12. [Command line utilities](https://code.it4i.cz/energy-efficiency/meric-suite/meric#12-command-line-utilities)
13. [Acknowledgment](https://code.it4i.cz/energy-efficiency/meric-suite/meric#13-acknowledgment)


<!--- dev branch links
 1. [System parameters tuning](https://code.it4i.cz/vys0053/meric/tree/dev#1-system-parameters-tuning)
 2. [Supported energy measurement systems](https://code.it4i.cz/vys0053/meric/tree/dev#2-supported-energy-measurement-systems)
 3. [MERIC API](https://code.it4i.cz/vys0053/meric/-/tree/dev#3-meric-api)
 4. [TIMEPROF library](https://code.it4i.cz/vys0053/meric/tree/dev#4-timeprof-library)
 5. [MERIC binary instrumentation](https://code.it4i.cz/vys0053/meric/tree/dev#5-meric-binary-instrumentation)
 6. [Compilation](https://code.it4i.cz/vys0053/meric/tree/dev#6-compilation)
 7. [MERIC input parameters](https://code.it4i.cz/vys0053/meric/tree/dev#7-meric-input-parameters)
 8. [Content of test folder and example application run](https://code.it4i.cz/vys0053/meric/tree/dev#8-content-of-test-folder-and-example-application-run)
 9. [Analysis of MERIC output files](https://code.it4i.cz/vys0053/meric/tree/dev#9-analysis-of-meric-output-files)
10. [MERIC with a Fortran code](https://code.it4i.cz/vys0053/meric/tree/dev#10-meric-with-a-fortran-code)
11. [Tool for static tuning](https://code.it4i.cz/vys0053/meric/tree/dev#11-command-line-utilities))
12. [Acknowledgment](https://code.it4i.cz/vys0053/meric/tree/dev#12-acknowledgment)
-->

--------------------------------------------------------------------------------
#     1] System parameters tuning                                              #
--------------------------------------------------------------------------------
MERIC allows to set selected system parameters for the whole application run or for each instrumented part of the analyzed application. Following list of the system parameters can be tuned using MERIC. Default and valid values of the following parameters can be read using `systemInfo` utility (part of the MERIC repository, located in the bin/ directory).


## CPU core frequency ##
MERIC provides several ways how to control the CPU core frequency, which highly depends on CPU vendor and available scaling driver.

##### Intel #####
Support for frequency scaling under `Intel Hardware Performance State (HWP)`, `intel_pstate` scaling driver, `intel_cpufreq` scaling driver, or `userspace` scaling governor of any scaling driver.

Under `HWP` requires additional write access to `/sys/devices/system/cpu/cpu*/cpufreq/scaling_max_freq` and `/sys/devices/system/cpu/cpu*/cpufreq/scaling_min_freq`. 

Under `intel_pstate` scaling driver requires additional write access to `/sys/devices/system/cpu/intel_pstate/no_turbo` and `/sys/devices/system/cpu/cpu*/cpufreq/scaling_max_freq`.

Under `intel_cpufreq` requires access to MSRs, or write access to `/sys/devices/system/cpu/cpu*/cpufreq/scaling_max_freq`.

If scaling driver allows to specify `userspace` scaling governor (`/sys/devices/system/cpu/cpu*/cpufreq/scaling_available_governors`), MERIC must have right to set the `userspace` governor in `/sys/devices/system/cpu/cpu*/cpufreq/scaling_governor` (if not already set), and have write access to `/sys/devices/system/cpu/cpu*/cpufreq/scaling_setspeed`. This does not apply in case of `intel_cpufreq` scaling driver which ignores `scaling_setspeed` input.

- Using write to MSRs - require [msr-safe](https://github.com/LLNL/msr-safe), ideally together with [libmsr](https://github.com/LLNL/libmsr). **Preferred interface for Intel CPUs, the only interface that supports tuning under HWP.**
- Using [x86_adapt](https://github.com/tud-zih-energy/x86_adapt).
- Using [geopm](https://geopm.github.io/).

##### AMD #####

- AMD 19h+ CPU using [EPYC System Management Software (E-SMS)](https://www.amd.com/en/developer/e-sms.html) stack - require [Host System Management Port (HSMP)](https://github.com/amd/amd_hsmp) and [EPYC System Management Interface (E-SMI) In-band Library](https://developer.amd.com/e-sms/e-smi-in-band-library/). **Preferred interface for AMD CPUs.**
- AMD 17h+ CPU via MSR - require [msr-safe](https://github.com/LLNL/msr-safe).

##### Any (other) CPU with userspace scaling driver #####

- Using cpufreq/[cpupower](https://github.com/torvalds/linux/tree/master/tools/power/cpupower).
- Using direct write to the `/sys/devices/system/cpu/cpu*/cpufreq/scaling_setspeed`.

## Intel CPU uncore frequency ##
Intel uncore frequency refers to frequency of subsystems in the physical processor package that are shared by multiple processor cores. E.g., L3 cache or on-chip ring interconnect.

MERIC requires [msr-safe](https://github.com/LLNL/msr-safe), ideally together with [libmsr](https://github.com/LLNL/libmsr) or [x86_adapt](https://github.com/tud-zih-energy/x86_adapt) or [GEOPM](https://geopm.github.io/) to be able to change the system uncore frequency. 


## Power capping ##
Available for Intel and AMD CPUs.

##### Intel #####
Intel processors from Sandy Bridge architecture provides Running Average Power Limit (RAPL), that keeps the power consumption of the available domains bellow the specified power limit in a specific time window (long-term and short-term windows). MERIC allows to specify power limit of the Package power domain within its long-term time window, which can be also adjusted.

MERIC requires [msr-safe](https://github.com/LLNL/msr-safe), ideally together with [libmsr](https://github.com/LLNL/libmsr) or [x86_adapt](https://github.com/tud-zih-energy/x86_adapt) or [GEOPM](https://geopm.github.io/) to be able change the power limits. Currently only PKG power cap control is supported.
	
Please, note that RAPL power capping system uses equation to specify the window size for PKG: `PKGTimeLimit = 2^Y * (1.0 + Z/4.0) * TimeUnit`, where Y=<0, 31>, Z=<0, 3> and TimeUnit is usually a fraction of microsecond. <!--- and window size for DRAM: `DRAMTimeLimit = 2^Y*F * TimeUnit`, where F={1.0, 1.1, 1.2, 1.3} and Y=<0, 31>. --> Due to that it might be impossible to set exactly the window size you have specified.

##### AMD #####
AMD EPYC CPUs from Zen3 (19h) architecture provides [EPYC System Management Software (E-SMS)](https://developer.amd.com/e-sms/) stack, that gives access to in-band (via HSMP) and out-of-band (via APML) power capping mechanism. MERIC allows to control the in-band power limit using [Host System Management Port (HSMP)](https://github.com/amd/amd_hsmp) and [EPYC System Management Interface (E-SMI) In-band Library](https://developer.amd.com/e-sms/e-smi-in-band-library/). In contrast to Intel RAPL, AMD power capping system has one power domain only, no time window support.



## Number of active OMP threads ##
Using OpenMP API.


## NVIDIA GPU streaming multiprocessors frequency ##
Some NVIDIA GPUs provide possibility to tune frequency of streaming multiprocessors. The frequencies are much more finegrain then frequencies of server processors, while only specific values are possible to set. Also, the available SM frequencies depend on frequency of the GPU memory, if the memory frequency is possible to tune. User may specify any frequency from the minimum - maximum frequency range, while MERIC will find the closest available frequency and will set it.
To tune the SM frequency, MERIC requires [NVIDIA Management Library (NVML)](https://developer.nvidia.com/nvidia-management-library-nvml). To tune GPU frequencies without sudo rights, superuser must execute command `sudo nvidia-smi -acp UNRESTRICTED` (feature removed from driver 465.xx). Alternatively, it is also possible to raise sudo rights to `nvidia-smi -ac *` and `nvidia-smi -rac` commands using `/etc/sudoers`.


## NVIDIA GPU memory frequency ##
NVIDIA GPUs with GDDR memory provides possibility to tune its frequency. As well as in case of the SMs, MERIC sets the closest available frequency, which is possible to set. Requires [NVIDIA Management Library (NVML)](https://developer.nvidia.com/nvidia-management-library-nvml). To tune GPU frequencies without sudo rights, superuser must execute command `sudo nvidia-smi -acp UNRESTRICTED` (feature removed from driver 465.xx). Alternatively, it is also possible to raise sudo rights to `nvidia-smi -ac *` and `nvidia-smi -rac` commands using `/etc/sudoers`.


## NVIDIA GPU power capping ##
Using [NVIDIA Management Library (NVML)](https://developer.nvidia.com/nvidia-management-library-nvml) to set power limit of NVIDIA GPUs. Requires sudo rights. Alternatively, it is also possible to raise sudo rights to `nvidia-smi -pl *` command using `/etc/sudoers`.


## ARM Jetson memory frequency ##
To simplify MERIC's interface, the memory frequency is set the same way as Intel CPU uncore frequency using MERIC_UNCORE_FREQUENCY environment variable.


--------------------------------------------------------------------------------
#     2] Supported energy measurement systems                                  #
--------------------------------------------------------------------------------
Energy measurement system can be selected using `MERIC_MEASURE` environment variable.

## RAPL ##
From the Sandy Bridge microarchitecture all Intel processors provide software-based energy meter Running Average Power Limit (RAPL) with sampling frequency 1 kHz of several power domains within the CPU (for server CPUs Package and DRAM power domains). The RAPL does not cover energy consumption of the node itself. From the Zen architecture also AMD CPUs have similar energy consumption measurement system, exposing energy consumption of the Package and each CPU core separately.

Since RAPL measurement is stored in a performance counter, overflow may happen during the application run. MERIC implements a overflow detection and recovery system, however it cannot fix the values if two or more overflows happen during a region that does not have any nested region. Please, keep it in mind when specifying a region size. We suggest not to have last-level region (does not have any nested region) that takes more than one minute.

Intel uses RAPL system to limit power consumption of their CPUs. The power limit can be controlled by MERIC, see section [System parameters tuning](https://code.it4i.cz/vys0053/meric/tree/wrapper#1-system-parameters-tuning).

MERIC supports reading RAPL energy consumption od Intel and AMD CPUs using virtual sysfs created by [intel_rapl/intel_rapl_common](https://github.com/torvalds/linux/blob/master/drivers/powercap/intel_rapl_common.c) linux kernel module. Similar interface is available in AMD CPUs created by [amd_energy](https://github.com/amd/amd_energy) kernel module. To reduce energy reading overhead and to receive more detailed information, MERIC supports [msr-safe](https://github.com/LLNL/msr-safe) (works well for both Intel and AMD). If MERIC is compiled at a system with the msr-safe available, then the msr-safe is always used for the readings. In case of Intel, it is recommended to compile MERIC also with [libmsr](https://github.com/LLNL/libmsr) (support for Icelake and older Intel architectures) or [x86_adapt](https://github.com/tud-zih-energy/x86_adapt) or [GEOPM](https://geopm.github.io/). In the detailed mode, MERIC stores energy consumption for each power domain, otherwise it aggregates the values.


## HDEEM ##
[High Definition Energy Efficiency Monitoring (HDEEM)](https://tu-dresden.de/zih/forschung/projekte/hdeem) is Atos|Bull's stand-alone energy measurement system located on the node itself, with power sampling frequency 1 kHz (blade), 100 Hz (on node components) respectively. HDEEM stores power samples in its internal memory and MERIC can read the energy consumption when a region starts and ends (HDEEM-C mode) with an overhead of the HDEEM system, or when the application execution is over (HDEEM-S mode), without any impact on the application run. When evaluating the energy consumption from the power samples (HDEEM-S) it is possible to store the raw samples to the output.


## DiG ##
Dwarf in Giant (DiG) is a hardware stand-alone power sampling system that stores the samples, that can be accessed through the [EXAMON](http://projects.eees.dei.unibo.it/monitoring/wordpress/) framework. MERIC does not access these power samples, so it cannot be stored.

To activate possibility of energy measurement provided by DiG system in the MERIC, [REST-client library](https://github.com/mrtazz/restclient-cpp) must be available on the target system. MERIC and the tuned applications must be compiled with the library.


## NVML ##
[NVIDIA Managemenet Library (NVML)](https://developer.nvidia.com/nvidia-management-library-nvml) allows to measure energy consumption of NVIDIA GPUs Volta or newer architectures. NVIDIA Volta GPUs samples the power consumption once every 20 ms, regions should be at least a second long to retrieve reliable energy measurement.

## ROCm SMI ##
[ROCm System Management Interface (ROCm SMI)](https://github.com/ROCm/rocm_smi_lib) allows to measure energy consumption of AMD GPUs of MI2xx or newer architectures. 


<!--- FUTURE dependency 
## AMD SMI ##
[AMD System Management Interface (AMD SMI)](https://github.com/ROCm/amdsmi) library is part of the AMD ROCm. 
-->

## HWMON ##
[The Linux Hardware Monitoring kernel](https://docs.kernel.org/hwmon/index.html) allows to measure energy consumption on AMD platforms (exposes AMD RAPL) and [NVIDIA platforms](https://docs.nvidia.com/dccpu/grace-perf-tuning-guide/power-thermals.html#power-telemetry) (NVIDIA Grace, Grace Hopper, Grace Blackwell). The data is exposed via [sysfs interface](https://docs.kernel.org/hwmon/sysfs-interface.html). Thus, no dependecy necessary.


## OCC ##
[On Chip Controller (OCC)](https://github.com/open-power/occ) provides power monitoring of compute nodes power by OpenPOWER CPUs. Available from IBM POWER8 processors. The performance counters are updated every 500 us.


## Fujisu A64FX ##
Energy measurement of ARM-based A64FX is provided by performance counters of three power domains (CORE, MEMORY, and L2 cache). MERIC uses perf to read them. Requires `/proc/sys/kernel/perf_event_paranoid = 0`.

## PDU ##
Some server systems are powered by Power Distribution Unit (PDU) which allows to expose power consumption of the server to the user space. A library providing the power samples from the PDU to the MERIC is necessary. 

## BSC ARM Jetson ##
Part of the MERIC repository is a Python power measurement script that runs on node background and effects CPU. The script stores the power samples into a file, and MERIC evaluates energy consumption of the instrumented regions when the application run is over. Ten samples per second was selected in the measurement script as a compromise - this sampling rate takes ~2% of the CPU load. It is possible to change the rate in tools/getJTX1measurements.py.


## BSC ARM ThunderX ##
BSC's ThunderX energy measurement system doesn't effects the CPUs, however it measures energy consumed by all available nodes (one must allocate all four nodes), and its energy measurement samples frequency is approximately 4 samples per second.

--------------------------------------------------------------------------------
#     3] MERIC API                                                             #
--------------------------------------------------------------------------------
## MERIC interface ##
If you want to use MERIC with a parallel application, keep in mind, that all processes and all running threads must call every inserted MERIC function, otherwise MERIC behavior is undefined. It is not possible to set runtime environment for each process separately, because MERIC does environment changes that effects whole node (or socket). To guarantee that selected settings is applied for selected region, each measurement start and stop begins with an MPI and OpenMP barrier. MERIC interface is defined in `include/meric.h`.

 *	`void MERIC_Init()`

	Initialization of the library. Insert directly after MPI_Init().
	MERIC automatically starts a region called same as the analyzed application binary with suffix "_static".

 *	`void MERIC_Close()`

	Finalization of the library run to store the measurement results. Insert directly before MPI_Finalize().
	Ends the region that started at the MERIC_Init() time.

 *	`void MERIC_MeasureStart(const char * region)`

	Starting measurement of a region. 
	Region name cannot contain any of `/`, `\`, `:`, `*`, `?`, `"`, `<`, `>`, `|`, `@`, `#`, end with `_static`, or exceed 255 characters.

 *	`double MERIC_MeasureStop(const char * region)`

	End of the measurement of the last started region.
	Returns runtime of the stopped region in seconds. Only single MPI process per node returns the runtime, the others returns 0.0.

 *	`double MERIC_MeasureStopStart(const char * stopRegion, const char * startRegion)`

	End of the measurement of the last started region and start of a new region.
	Removes environment switching to configuration of the region the application is nested in.
	Returns runtime of the stopped region in seconds. Only single MPI process per node returns the runtime, the others returns 0.0.

 *	`void MERIC_CaptureScope(const char * region)`

	Starts region with this function call, and automatically stops it at the end of the scope. Useful to capture a function that has several return statements.
	This functionality is based on [RAII](https://en.cppreference.com/w/cpp/language/raii) technique, which is supported by C++ and C compilers (no support for Fortran) with cleanup attribute (such as GNU or Clang). When using a compiler without the support the MERIC_CaptureScope function will be removed from the code. C applications may require compilation with `-fno-exceptions` or `-lstdc++` flag. 

 *	`void MERIC_IgnoreStart()`

	From this point MERIC doesn't store resources consumption of the following regions but the requested settings of the nested regions is set.
	It is not possible to nest ignore sections of the code.

 *	`void MERIC_IgnoreStop()`

	Cancels the ignore section of the code.

If you want to insert MERIC regions into code you don't know, instead of MERIC probes insert into the code MPI and OpenMP barriers and make sure, that the code works fine. After that replace barriers with Meric_MeasureStart/Stop.

There is one more restriction in placing probes into the code. In current version MERIC does not support recursively nested regions and regions with at least three starts where the first and the third calls are at the same level, but second call is at higher level. This case is shown in example:

```
MERIC_MeasureStart("X") //region X wraps others, this region is not necessary in this example
	MERIC_MeasureStart("A")	//first call of region A
	MERIC_MeasureStop("A")

	MERIC_MeasureStart("B")
		MERIC_MeasureStart("A")	//region A called at higher nested level
		MERIC_MeasureStop("A")
	MERIC_MeasureStop("B")

	MERIC_MeasureStart("A")	//next region A call at the same nested level as the first one
		MERIC_MeasureStart("C")	//the problem is here, when region A has another nested region
		MERIC_MeasureStop("C")	//region C will cause defect in its call tree
	MERIC_MeasureStop("A")
MERIC_MeasureStop("X")
```

<!---
## Advanced manual instrumentation ##
To instrument an application manually we recommend to use [Instrument API](https://code.it4i.cz/energy-efficiency/instrumentapi) instead of MERIC API. The Instrument API provides unified interface of the most common open-source HPC tools for application behavior analysis. 
The API is using preprocessor directives, which allows user to select at the compilation time which tool's instrumentation should be used, as well as it is possible remove the instrumentation at all.
-->

--------------------------------------------------------------------------------
#     4] TIMEPROF library                                                      #
--------------------------------------------------------------------------------

The TIMEPROF library provides light-weight runtime measurement of the instrumented regions in the code. The library is part of the MERIC repository since its main task is to identify regions for future instrumentation with MERIC.

## TIMEPROF configuration ##
TIMEPROF can be configured using following environment variables:
 * `export TIMEPROF_TIME=100`
 	- Specification of minimum region runtime in milliseconds to include the region to the output. In default no time threshold.

 * `export TIMEPROF_MASTER=1`
	- In default `TIMEPROF_MASTER=0` all processes trace the application, and moreover checks that the regions have been executed by all processes the same number of times. `TIMEPROF_MASTER=0` option causes that only master thread of MPI_WORLD_COMM rank 0 traces the instrumented application.

 * `export TIMEPROF_OUTPUT=""`
	- Specify name of output file, in default `TIMEPROF.log`. `TIMEPROF_OUTPUT=""` cause storing the output to stderr.
 
## TIMEPROF API ##
The interface is defined in `include/timeprof.h`. 

 *	`void TIMEPROF_regionStart(const char * regionName)`
	- Start time measurement of region called regionName.

 *	`double TIMEPROF_regionStop(const char * regionName)`
	- End of the time measurement, returns region duration in seconds.

 *	`void TIMEPROF_evaluate()`
	- At the end of the application run should be called this function to evaluate the measurements.

 *	`void TIMEPROF_captureScope(const char * regionName)`
	- [RAII](https://en.cppreference.com/w/cpp/language/raii) time measurement of the scope, where specified.
	- In case of C applications, the application may require compilation with `-fno-exceptions` or `-lstdc++` flag if you want to use this function.

 *	`double TIMEPROF_getLastRegionDuration()`
	- Since scope time measurement does not return the time measured, it can be obtained using this function.

--------------------------------------------------------------------------------
#     5] MERIC binary instrumentation                                          #
--------------------------------------------------------------------------------
Manual application instrumentation is straightforward, however it requires at least some basic knowledge about the target application, access to the source files and also time to instrument the code. All these steps can be overcome when using static binary instrumentation (SBI). We provide two tools based on [Dyninst library](https://dyninst.org/):
 * `dinst_profile` - Provides list of all functions that are defined in the binary (shared libraries are not taken into account) and provides information whether the function can or cannot be instrumented.
 * `dinst_instrument` - Provides binary instrumentation with MERIC or TIMEPROF based on list of functions that should be instrumented. In case of TIMEPROF, the list of functions does not have to be provided, in that case all the instrumentable functions are selected. Besides the instrumentation tools also adds all the necessary dependencies, so it is not necessary to recompile the application and link it with them. In case of MPI applications it is not only the application that is instrumented, but also the MPI library itself too. For this purpose shared MPI library that is used for the application must be provided. Please, specify full path to the MPI library to omit any possible mistake. Use `ldd` command to detect which MPI library is used for the analyzed application. When running instrumented MPI application `LD_PRELOAD` must be specified to replace default MPI library with the instrumented one. 

See tools' help message for more information.

To run the SBI tools it is not only necessary to update LD_LIBRARY_PATH, but also export `DYNINSTAPI_RT_LIB` variable to specify `libdyninstAPI_RT.so` location. This shared library, which does not have a compiler dependency, is the only added dependency to generated binary in compare to manually instrumented application.

##### known issues #####
 * Instrumented function with the return command only may lost the return value, you may increase `instlimit` TIMEPROF parameter for skipping such function.
 * If instrumented application is compiled with the same list of modules as the Dyninst tools, sometimes the new binary is corrupted. Can be tested using `ldd` command. If it happens, we suggest to use different set of modules for the application or the Dyninst tool.

--------------------------------------------------------------------------------
#    6] Compilation                                                            #
--------------------------------------------------------------------------------
It is possible to have a single MERIC compilation for several hardware platforms, however since MERIC identifies the default configuration of the hardware just once, it is necessary to have more compilations if having the same hardware, each in a specific default configuration.

MERIC is compiled using [waf build system](https://waf.io/), since the system is not well known, Makefile in the repository root folder is provided. Please, modify libs and include paths according your system paths:

## MERIC used libraries ##
* mandatory  - [Python3](https://www.python.org/)
* mandatory  - [rt](https://man.cx/librt(3))
* optionally - OpenMP (used in default)
* optionally - MPI
* optionally - [PAPI](http://icl.cs.utk.edu/papi/)
* optionally - [perf_event](http://man7.org/linux/man-pages/man2/perf_event_open.2.html) - required for Fujitsu A64FX energy measurement
* optionally - [msr-safe](https://github.com/LLNL/msr-safe) - required for AMD RAPL monitoring, prefered interface for Intel CPUs parameters tuning
* optionally - [libmsr](https://github.com/LLNL/libmsr) - prefered to be used for Intel CPUs parameters tuning
* optionally - [E-SMI in-band](https://github.com/amd/esmi_ib_library) - prefered interface for AMD 19h CPUs parameters tuning
* optionally - [HSMP](https://github.com/amd/amd_hsmp) - mandatory for E-SMI in-band library
* optionally - [OCC](https://github.com/open-power/occ) - mandatory for OpenPOWER CPU power monitoring
* optionally - [HDEEM](https://tu-dresden.de/zih/forschung/projekte/hdeem?set_language=en)
* optionally - [FreeIPMI](http://www.gnu.org/software/freeipmi/) - mandatory for the HDEEM power monitoring
* optionally - cpufreq/[cpupower](https://github.com/torvalds/linux/tree/master/tools/power/cpupower)
* optionally - [x86_adapt](https://github.com/tud-zih-energy/x86_adapt)
* optionally - [GEOPM](https://geopm.github.io/)
* optionally - [NVML](https://developer.nvidia.com/nvidia-management-library-nvml) - mandatory for the NVIDIA GPUs monitoring and tuning
* optionally - [ROCm](https://github.com/ROCm/rocm_smi_lib)
* optionally - [numactl](https://github.com/numactl/numactl)
* optionally - [REST-client](https://github.com/mrtazz/restclient-cpp) - mandatory for the DiG power monitoring
* optionally - [net-snmp](https://github.com/net-snmp/net-snmp/) - mandatory for E4 PDU power monitoring
* integrated - [sheredom json parser](https://github.com/sheredom/json.h)

During runtime MERIC create one or a few temporary files. `MERIC_TMP_PATH` must be defined in the configuration stage to specify location where to store these files, otherwise these files are created in the MERIC root directory.

## TIMEPROF used libraries ##
* mandatory  - [rt](https://man.cx/librt(3))
* optionally - OpenMP
* optionally - MPI

## SBI tools ##
The Static Binary Instrumentation tools use Dyninst library for binary analysis and following modification. The Dyninst can be compiled using GNU compiler only, despite it supports also binary instrumentation generated by others compilers. Due to this limitation the SBI tools are compiled together with MERIC only when using GNU compiler. It is possible to compile SBI tools only when using `make sbi`.
* mandatory - [Dyninst](https://dyninst.org/)
* mandatory - [Boost](https://www.boost.org/) - date_time, system, and thread

The compilation mechanism detects the compiler type, however if the detection fails, the default is GNU C++ compiler. Export `CXX` and `MPICXX` environment variables to specify the compiler. The compilation compiles MERIC, TIMEPROF and all the tools, except the SBI tools when GNU compiler is not used, or any of the dependencies is missing. If MPI compiler is available, than compilation will produce both MPI and non-MPI versions of the libraries, both using OpenMP. If an MPI application without OpenMP should be analyzed, use `make noopenmp` command. Accordingly, link your application with `-lmeric`/`-ltimeprof` or `-lmericmpi`/`-ltimeprofmpi` for your OpenMP+MPI application or`-lmericmpionly`/`-ltimeprofmpionly` for pure MPI application.

To use MERIC for Fortran application analysis a Fortran module is provided. The module is being compiled separately from MERIC, use `make fortran` command.

--------------------------------------------------------------------------------
#     7] MERIC input parameters                                                #
--------------------------------------------------------------------------------

## Static tuning - mandatory parameters ##
Specify following parameters to zero if you do not want to tune the selected system parameter.

* `export MERIC_FREQUENCY=2400MHz`
* `export MERIC_UNCORE_FREQUENCY=2GHz`
	- Both frequencies can be specified as integer in Hz (default), kHz, MHz or GHz.

* `export MERIC_NUM_THREADS=24`
	- Number of openMP threads that should be used to run the application.
	- *IMPORTANT:* When using hyper-threading, thread pinning should be specified, otherwise DVFS may not work.

* `export MERIC_PWRCAP_POWER=100000`
	- Average processor PKG power in milliwatts under the selected time window.

* `export MERIC_PWRCAP_TIME=3000`
	- Time window size for power capping specified in milliseconds.

* `export MERIC_GPU_FREQUENCY=1400MHz`
* `export MERIC_GPU_MEM_FREQUENCY=1800MHz`
	- Specification of GPU streaming multiprocessors frequency and frequency of the GPU memory.
	- As well as CPU frequencies these are also specified as integer in Hz (default), kHz, MHz or GHz.
	- Only GDDR memories are possible to tune.
	- *NOTE:* Available frequencies of streaming multiprocessors depend on frequency of the memory.

* `export MERIC_PWRCAP_GPU=140000`
	- GPU power limit in milliwatts.

## Dynamic tuning ##
While environment variables allows to specify a configuration that persists for the whole application runtime, a configuration for a specific application region must be done using a configuration file. Moreover it allows to specify additional  MERIC configuration file uses JSON format and `.opts` suffix. For following visualisation of an application behavior analysis using RADAR visualizer, the MERIC configuration file should be located in the MERIC output directory (see section [MERIC input parameters: Output files/folder name](https://code.it4i.cz/vys0053/meric/-/tree/wrapper#output-filesfolder-name)).

* `export MERIC_REGION_OPTIONS=/path/to/configurationfile.opts`
	- Specify a MERIC configuration file to be used to tune an application.

The configuration file has a following format:
- `region name` - mandatory, object, for each region that should have a specified configuration
   	- `parameter` - mandatory, int/float/string, for each tuned parameter, specified parameter value in the parameter's default unit, the list of parameters must be the same for each specified region except the static region, which may have additional parameters
- `"@IGNORE"` - optional, object, specify that MERIC should not measure, tune, or store output for regions if any of the following is true
	- `"@REGIONS"` - optional, list of strings, list of regions that should be ignored by MERIC
	- `"@RUNTIME"` - optional, float, minimum region runtime [s] to be tracked by MERIC, measured during the first execution of the region
- `"@RADAR"` - optional, object, information for application behavior visualization
	- `"@METRIC"` - mandatory, object, 
	metrics used for identification of optimal configuration
   		- `metric` - section in the MERIC output file, mandatory, string, metric of the specified section
	- `"@BASELINE"` - optional, float, static power baseline [W]

Example MERIC configuration file:
```
{
	"a.out_static" : {
		"FREQUENCY" : 2100000000,
		"UNCORE_FREQUENCY" : 2100000000,
		"NUM_THREADS" : 12
	},
	"A" : {
		"FREQUENCY" : 1200000000,
		"UNCORE_FREQUENCY" : 1200000000
	},
	"C" : { 
		"FREQUENCY" : 2500000000, 
		"UNCORE_FREQUENCY" : 2300000000,
	},
	"@RADAR" : {
		"@METRIC" : { 
			"# COUNTERS - HDEEM:" : "Energy consumption [J]",
			"# COUNTERS - NVML:" : "Energy consumption [J]"
			},
		"@BASELINE" : 0.0
	},
	"@IGNORE" : {
		"@REGIONS" : ["B", "D"],
		"@RUNTIME" : 0.2
	}
}

```
More examples of MERIC configuartion files are in [test/config directory](https://code.it4i.cz/vys0053/meric/-/tree/wrapper/test/config).


## Measurement mode ##
* `export MERIC_MEASURE=RAPL,HDEEM-S`
	- NONE    = Default option, no time or energy measurement.
	- HDEEM-S = Atos|Bull HDEEM energy and power measurement, evaluating energy consumption from power samples ("continual mode").
	- HDEEM-C = Atos|Bull HDEEM energy and power measurement, the system provides energy consumption directly at the beginning and end of each region. This mode may cause extra overhead in compare to evaluating from power samples.
	- HDEEM   = Equals to HDEEM-S option.
	- RAPL    = Intel/AMD RAPL energy measurement.
	- NVML    = NVIDIA GPUs energy measurement.
	- HWMON   = NVIDIA CPU/platform, AMD RAPL energy measurement.
	- OCC     = IBM POWER OCC energy measurement.
	- ROCM    = AMD GPU/APU energy measurement.
	- A64FX   = Fujitsu A64FX energy measurement.
	- DAVIDE  = DiG energy and power measurement at CINECA D.A.V.I.D.E. system.
	- JETSON  = BSC Jetson TX1 energy and power measurement.
	- THUNDER = BSC ThunderX energy and power measurement.
	- PDU     = PDU energy measurement.
	- TIME    = Runtime measurement, default for other modes.

* `export MERIC_BARRIERS=all`
	- all  = all barriers are applied (default)
	- mpi  = use MPI barriers only
	- omp  = use OpenMP barriers only
	- none = do not use barriers

## Output format ##
* `export MERIC_AGGREGATE=0`
	- When running an MPI application, MERIC aggregate the data from all the processes and stores the aggregated results. There are both average values and summary included in the output files.
	- Exporting MERIC_AGGREGATE=0 turns off this behavior and MERIC will store the results for each node separately.

* `export MERIC_COUNTERS=papi` or `perfevent` or `msr`
	- MERIC reads selected hardware performance counters using PAPI or perfevent or msr.
	- From the counters we also evaluate some ratios (e.g. CPU core and uncore frequency, computational and arithmetic intensity).
	- To add a counter you want to measure it is necessary to follow the instructions in src/wrapper/counters.h.

* `export MERIC_DETAILED=1`
	- If energy measurement system provides not only data from blade, but also data from other Voltage Regulators (e.g. CPUx, DRAMx) are gathered too, MERIC will provide information about each of them. 

* `export MERIC_SAMPLES=1`
	- If the energy measurement system provides power samples, MERIC will store also the samples to the output directory. It will be located in directory `samples` in the directory of the static region.
	- Available for HDEEM-S, RAPL, HWMON, and NVML.
	- Available only in non-aggregate (MERIC_AGGREGATE=0) continual mode (such as HDEEM-S).
	- WARNING: Output may become very big when measured regions run for a long time.

* `export MERIC_SAMPLING_FREQUENCY=<sampling frequency in Hz>`
	- If the energy measurement system provides power samples, MERIC will attempt to collect the samples at specified frequency.
	- The maximum frequency is automatically limited by each sampling subsystem (HDEEM-S, RAPL, HWMON, and NVML) to avoid unnecessary oversampling (and associated overhead).
	- The sampling frequency is also used to collect the data for derived energy counters on platforms that don't provide native energy counters (such as NVIDIA GH200).
	- The sampling overhead can be minimized by setting this variable to low frequency (such as MERIC_SAMPLING_FREQUENCY=1). Note that this will negatively impact accuracy of energy measurements in cases, where the value is derived from power sampling.
	- If MERIC_SAMPLING_FREQUENCY is not set (or set to 0), the sampling runs at the default sampling frequency, which varies by power monitoring system. The default and the maximum sampling frequency of available power monitoring systems can be read using systemInfo -p.

* `export MERIC_DEBUG=1`
	- Uses HDEEM-S and HDEEM-C energy measurement at the same time.
	- Ignores MERIC_DETAILED input parameter and evaluates all the voltage regulators.
	- Only for measurement check - there can be larger overhead because of two types of data processing performed simultaneously.

## Output files/folder name ##
* `export MERIC_OUTPUT_DIR="hdeemMeasurement"`
	- Default name is mericMeasurement.

* `export MERIC_OUTPUT_FORMAT="CF_UnCF"`
	- Underscore separated list of parameters. MERIC automatically determines which parameters are tuned, and compose output filename accordingly. This parameter allows user force MERIC to extend the output files name about parameters that are not tuned. Zero will represent the non-tuned parameter. This parameter does not influence parameters order. Each supported parameter is represent by one of the following strings:
	  - `CF` = CPU core frequency
	  - `UnCF` = CPU uncore frequency
	  - `thrds` = number of threads
	  - `CPUPWR` = CPU power limit
	  - `CPUPWRtw` = CPU power limit time window
	  - `GPUF` = GPU streaming multiprocessors frequency
	  - `GPUMEMF` = GPU memory frequency
	  - `GPUPWR` = GPU power limit

* `export MERIC_OUTPUT_FILENAME="log"`
	- Name of the output file is set automatically according specified values of the tuned system parameters, however this is a way how to add filename suffix.

## Thread affinity management ##
* `export MERIC_PIN_MODE=<thread pinning mode>`
	- MERIC is capable of performing simple MPI-aware thread (OpenMP) to core pinning. The aim is to pin each thread to its own dedicated core (as long as "thread count" <= "core count") to allow per-core frequency tuning and per-thread/core measurement in some cases. Following pinning algorithms are available:
	 - `default` = Maintains thread placement specified by the system (OpenMP, MPI, etc.), but pins each thread to a single core (when possible)
	 - `close` = Pins threads sequentially to available cores
	 - `spread` = Pins threads with step equal to CORE_COUNT / (THREAD_COUNT*RANK_COUNT), which can be used to spread threads between sockets etc.
	 - `none` = Disables the thread pinning mechanism in MERIC (OpenMP/MPI/... settings are honored)

--------------------------------------------------------------------------------
#     8] Content of test folder and example application run                    #
--------------------------------------------------------------------------------
* source codes
	* plain_test.cpp
		* Simple test with no MERIC regions.
	* test.cpp
		* One region, with two another regions inside.
	* test_mpi.cpp
		* Same test as test.c, only extended with MPI.
	* samples_test.cpp
		* Test with sleep (minimum energy consumption) and compute (much higher energy consumption) regions in a loop. This allows user to see in the list of samples how energy rise when compute region starts and check if MERIC detects this sample as a first one of this region.
		* Follow the instructions inside to set the test.
	* sleep_test.cpp
		* A test with RUN (maximum load) and SLEEP (minimum load) regions, both with the same runtime.
		* Originally made to control real CPU frequency of the machine.
	* overhead_test.cpp
		* Test to measure MERIC overhead and overhead of libraries to change environment parameters.
	* fort_test.f90
		* Fortran version of test.cpp to show the MERIC Fortran interface.
		* It si compiled using `make fortTest`.
	* blas_test.cpp
		* Test compares DGEMM and DGEMV. There are two possible sizes of matrices (large and small (not) to fit in L3 cache). In both cases, sizes of matrices were set to take approximately the same time for both DGEMM and DGEMV region, when using all available resources.
		* This test requires [Intel MKL](https://www.intel.com/content/www/us/en/developer/tools/oneapi/onemkl.html) library.
		* It is compiled using `make blasTest`.
	* cuda_test.cu
		* Executes a memory bound and a compute bound kernels in all available GPUs.
		* It is compiled using `make cudaTest`.
* Makefile
	* Command `make` compiles all test codes except the ones which explicitly lists its compilation command.
* environment_default.source
	* Basic script that sets chosen MERIC environment variables and informs you which variables are set.
	* When run with argument `-t`, the script just prints list of set variables.
	* Make a copy of this script and edit it to suits your needs.
* config directory
	* region.options
		* File that sets exact settings for regions inside your code.
		* In default it is set for blas_test.
	* region.options.extra
		* Configuration file, that shows advanced MERIC settings.
* test-power-monitoring.sh
	* Script that tests available power monitoring systems using `mericStatic` tool in conjunction with plain_test.cpp
* run.sh and run-mpi.sh
	* Scripts that runs MERIC analysis of test.cpp or test_mpi.cpp on Taurus machine.
* run-jetson.sh
	* Template script to submit a job on BSC ARM Jetson platform.
	

Specify the mandatory MERIC's parameters and run you instrumented application or one from the test directory. Good starting point is running `test` application from test directory. To understand well MERIC's output, explore the source file **test.cpp** to see that this test contains regions A, B and C (inside A are two regions B and one region C). For each region you can set CPU core and uncore frequencies and number of threads.
```
export MERIC_FREQUENCY=0         # no CPU core frequency tuning
export MERIC_UNCORE_FREQUENCY=0  # no CPU uncore frequency tuning
export MERIC_NUM_THREADS=0       # non-OpenMP application
export MERIC_PWRCAP_POWER=0      # no RAPL power capping
export MERIC_PWRCAP_TIME=0       # no RAPL power capping
export MERIC_GPU_FREQUENCY=0     # no GPU streaming multiprocessors frequency tuning
export MERIC_GPU_MEM_FREQUENCY=0 # no GPU memory frequency tuning
export MERIC_PWRCAP_GPU=0        # no GPU power capping
export MERIC_MEASURE=TIME        # time measurement only

./test                           # run the application as usual
```

--------------------------------------------------------------------------------
#      9] Analysis of MERIC output files                                       #
--------------------------------------------------------------------------------
MERIC's output is stored in the directory in default named `mericMeasurement` that contains result files in folders with names of the regions. To change output folder name export MERIC_OUTPUT_DIR="NEW_NAME".
Each output csv file carry 3 types of information:
 *  CALLTREE - the first line of every csv file, it's a call stack, so we can see, the regions nesting
 *  Section label (e.g. '# Job info') - determines a "category" of data that follows
 *  Data - tuples of: key, value

To identify the optimal parameters configuration of each region of the instrumented application, user may execute the application in various configurations and analyze the output files. This can be done manually, or automatically using `MERICwrapper` tool, that provides several algorithms for state-space-search, and optimal configuration configuration identification. For more details see its [README](https://code.it4i.cz/vys0053/meric/-/blob/wrapper/src/tools/wrapper/README.md) file.

MERIC output presenting application dynamic behavior can be visualized using [RADAR](https://code.it4i.cz/vys0053/SGS18-READEX) tool.

## Manual state-space-search example ##
```
export MERIC_MEASURE=RAPL,HDEEM
export MERIC_PWRCAP_POWER=0
export MERIC_PWRCAP_TIME=0
export MERIC_GPU_FREQUENCY=0
export MERIC_GPU_MEM_FREQUENCY=0
export MERIC_PWRCAP_GPU=0
export MERIC_OUTPUT_FORMAT="CF_UnCF_thrds" # optional parameter

for thread in {24..1..1}
do
	for cpu_freq in {25..12..1}
	do
		for uncore_freq in {30..12..1} # or {30..12..2}
		do
			for iter in {0..3}
			do
				export MERIC_NUM_THREADS=$thread
				export MERIC_FREQUENCY=${cpu_freq}00MHz
				export MERIC_UNCORE_FREQUENCY=${uncore_freq}00HMz
				./a.out
			done
		done
	done
done
```

--------------------------------------------------------------------------------
#      10] MERIC with a Fortran code                                           #
--------------------------------------------------------------------------------
<!---
### Fortran binding
-->
There are Fortran module and interface in the include directory. To instrument a Fortran application the Dyninst tool for static binary instrumentation can be used, also manual instrumentation is available.

To allow MERIC manual instrumentation in your Fortran application, add `use meric` command to your program. For the MERIC functions user should use keyword `call` as usual for Fortran functions. Since in Fortran is a problem with C `const *char` all the region names must be ended with `//char(0)` (e.g. `call MERIC_MeasureStart("RegionName"//char(0))`). MERIC repository contains a Fortran code example `test/fort_test.f90` to show how the API can be used.

--------------------------------------------------------------------------------
#      11] MERIC with a Python code                                           #
--------------------------------------------------------------------------------

### Python binding

There is Python module in the `lib` directory named libmericpy.so and libmericmpipy.so.

To allow MERIC manual instrumentation in your Python application, `include libmericpy` to your code. Usual `MERIC_*` calls replaced to camelCase version `libmeric.*`, i.e `MERIC_MeasureStart(..)` to `libmeric.measureStart(..)`. Provided API functions given below. MERIC repository contains a code example `test/sleep_test.py` to show how the API can be used.

```
Help on module libmericpy:

NAME
    libmericpy - Python bindings for MERIC API functions

FUNCTIONS
    close(...) method of builtins.PyCapsule instance
        close() -> None
        
        Close MERIC
    
    ignoreStart(...) method of builtins.PyCapsule instance
        ignoreStart() -> None
        
        Start ignoring measurements
    
    ignoreStop(...) method of builtins.PyCapsule instance
        ignoreStop() -> None
        
        Stop ignoring measurements
    
    init(...) method of builtins.PyCapsule instance
        init() -> None
        
        Initialize MERIC
    
    measureStart(...) method of builtins.PyCapsule instance
        measureStart(region: str) -> None
        
        Start measuring a region
    
    measureStop(...) method of builtins.PyCapsule instance
        measureStop(region: str) -> float
        
        Stop measuring a region and return elapsed time
    
    measureStopStart(...) method of builtins.PyCapsule instance
        measureStopStart(stop_region: str, start_region: str) -> float
        
        Stop one region and start another, return stop duration

```


--------------------------------------------------------------------------------
#     12] Command line utilities                                               #
--------------------------------------------------------------------------------
## systemInfo ##
 - Provides basic information about the CPU and GPUs (if available).
 - List of available power management knobs.
 - Range of possible configurations and the default value for each knob.
 - Identifies whether the MERIC installation is capable to manage the knob in the executed platform.
 - Provides list of available power monitoring systems that MERIC can use to measure energy consumption.

## E4RemoteMonitoring ##
 - Master (Monitoring System) - Slave (target system) implementation
 - After `master --init` command, slave.sh script created
 - Alter  the slave.sh to execute your commands then execute on the target system.
 - It will trigger a session creation on Master node and starts monitoring the target node.
 - Exit master program to terminate its session.

## mericStatic ##
 - Command line energy measurement and hardware power management.

##### Usage #####

The basic usage of the `mericStatic` tool is as follows:

```
mericStatic [mericStatic parameters] -- ./application [application parameters]
```

##### Application #####

The application can be any user-provided program; however, if the user provides the following special commands, the behavior of `mericStatic` will change accordingly.
> Users can override start, stop, or eval command behavior by using `./<start | stop | eval>`, provided they have a program with the same name. In other words, "`-- ./start`" and "`-- start`" are two different things.

- `start`: Starts measuring energy until the `mericStatic -- stop` command is issued.
- `stop`: Sends the SIGTERM signal to stop the `mericStatic` commands.
- `eval`: Reads all dump files and aggregates total energy consumption.

> **Warning:** The `start`, `stop`, and `eval` commands must be executed **only once per node**, regardless of how many tasks are requested per node. Please see the example use-cases related to multi-host multi-task execution.


<!-- 
### mericStatic Parameters

- `-i`: ID of the execution. Used to separate measurements. If not specified, defaults to *Slurm, PBS or Flux job id if exists else 0*.
- `-e`: One of the supported energy measurement systems: `RAPL`, `NVML`, `HWMON`, or `A64FX`.
- `-c`: CPU core frequency in Hz (default: Hz, can specify units like GHz, MHz, or kHz).
- `-u`: CPU uncore frequency in Hz (default: Hz, can specify units like GHz, MHz, or kHz).
- `-t`: Number of OpenMP threads.
- `-p`: CPU power capping limit in mW.
- `-w`: CPU power capping time window in ms.
- `-s`: GPU SM frequency in Hz.
- `-r`: GPU memory frequency in Hz.
- `-g`: GPU power capping limit in mW.
- `-b`: Single node baseline (static) power consumption in W.
- `-q`: Print the overall consumed energy only in Joules (J).
- `-o`: Output directory, `./mericStaticMeasurements` by default. -->

##### Output Format #####

By default, `mericStatic` creates a folder named `mericStaticMeasurements` and saves the measurement files there, unless a different name is specified using the `-o` option. The measurement file is named using the format `mericStatic_<ID>_$(hostname).data`, where `ID` corresponds to one of the environment variables `SLURM_JOB_ID`, `PBS_JOB_ID`, or `FLUX_JOB_ID`. If none of these are defined, the ID defaults to `0`, unless explicitly provided by the user using the `-i` option.

##### Example Usecase #####

`mericStatic` can be used in different scenarios including *program measurement*, *general measurement*, and *multi-host measurement*. Results are saved per node and the `mericStatic` Eval command can be used to calculate the total sum.

##### Program Measurement #####

In this setup, we measure RAPL and NVML energy consumption for `sleep 5` for a single node.

```
mericStatic -e RAPL,NVML -- sleep 5
```

```
$ ./mericStatic -e RAPL,NVML -- sleep 5
job_id :1289568

$ cat mericStaticMeasurements/mericStatic_1289568_cn195.barbora.it4i.cz.data 
Runtime [s] = 5.00656
RAM_0 [J] = 35.4023
RAM_1 [J] = 20.939
PCKG_0 [J] = 239.969
PCKG_1 [J] = 239.376

RAPL Energy consumption [J] = 535.686
Watt-hour (Energy[J] / 3600) = 0.1488016


Runtime [s] = 5.00656
GPU_0 [J] = 207.016
GPU_1 [J] = 209.552
GPU_2 [J] = 199.66
GPU_3 [J] = 204.812

NVML Energy consumption [J] = 821.04
Watt-hour (Energy[J] / 3600) = 0.228066
```

##### General Measurement #####

Similar to *program execution*, user can start measuring until further notice.

```
mericStatic -e RAPL,NVML -- start 
mericStatic -e RAPL,NVML -- start &  # optionally.
```

This command will start collecting energy measurements every 60 seconds. It handles counter-overflow internally. User may terminate the measurement with SIGTERM or SIGINT, but it is advised to use `-- stop`.

```
mericStatic -e RAPL,NVML -- stop
```

`-- stop` can be used in conjunction with `-- start` or independently to end long-running energy measurements. It sends a kill signal to the `mericStatic` program run by the current user, which then forwards the signal to the program it is monitoring, and subsequently dumps the energy measurements.

##### Eval #####

`eval` assumes that all result files are accessible to the user. It filters files using the pattern `mericStatic_<job_id>*`. While it sums the total energy fields, reports the maximum execution time along with watt-hours. For the earlier example:

```
$ ./mericStatic -- eval
Max Runtime [s] = 5.00656
A64FX Energy consumption [J] = 0
HWMON Energy consumption [J] = 0
NVML Energy consumption [J] = 821.04
RAPL Energy consumption [J] = 535.686
ROCM Energy consumption [J] = 0
Total Energy = 1356.73
Total Watt-hour (Total Energy[J] / 3600) = 0.376868


```

##### Multi-Host Measurement #####

So far we have used a single host, but `mericStatic` can also be used by the job scheduler. The example below is for a Slurm environment.
> `mericStatic -- eval` assumes a NAS system exists in the cluster. Otherwise, user must ensure access to the results (e.g., by copying them).

**Executing for a program and evaluating the result**

```
$ srun ./mericStatic -e RAPL,NVML -- ./application [application parameters]
$ ./mericStatic -- eval
```

**Executing for a program, early termination and result evaluation**

```
$ srun --overlap ./mericStatic -e RAPL,NVML -- ./application [application parameters] &
$ srun --overlap ./mericStatic -- stop
$ ./mericStatic -- eval
```
Note that `--overlap` is Slurm specific, allowing to execute other srun commands simultaneously, with this we could issue `stop`

**Long-run start, stop, and evaluation**

```
$ srun --overlap ./mericStatic -e RAPL,NVML -- start &
------------------ after some time ------------------
$ srun --overlap ./mericStatic -- stop
$ ./mericStatic -- eval
```

**Real Use-Case for Multi-Host Multi-Task Execution**

Following is an sbatch script for a dummy benchmark. Note that in the benchmark we allocate 16 nodes with 8 GPUs where each to create 8 instances of the program and each to have 16 CPU cores.

Below example is written both for Slurm and srun. Based on the Slurm configurations, sometimes it is not possible to request single instance per node with `--overlap`. In such a cases, one can use mpirun for that.
```
#!/bin/bash
#SBATCH --account OPEN-XX-YY
#SBATCH --job-name namd_gpu_1N_test
#SBATCH --partition qgpu
#SBATCH --nodes 16
#SBATCH --ntasks-per-node 8
#SBATCH --cpus-per-task 16
#SBATCH --gpus-per-node 8
#SBATCH --time 00:20:00
#SBATCH --output=out-%x.%j.txt

#SBATCH --mail-user=john_doe@gmail.com
#SBATCH --mail-type=BEGIN,END,FAIL

nnodes=16


mkdir mericStaticMeasurements

# srun --overlap --ntasks-per-node 1 --nodes $nnodes /path-to/mericStatic -e RAPL,NVML -o ./mericStaticMeasurements/$nnodes -- start &
mpirun -np $nnodes --map-by ppr:1:node /path-to/mericStatic -e RAPL,NVML -o ./mericStaticMeasurements/$nnodes -- start &

srun /path-to/benchmark [params]

mpirun -np $nnodes --map-by ppr:1:node /path-to/mericStatic -o ./mericStaticMeasurements/$nnodes -- stop
# srun  --ntasks-per-node 1 --nodes 4 /path-to/mericStatic -o ./mericStaticMeasurements/$nnodes -- stop

/path-to/mericStatic -o ./mericStaticMeasurements/$nnodes -- eval

out_file="./out-${SLURM_JOB_NAME}.${SLURM_JOB_ID}.txt"
mv "$out_file" "mericStaticMeasurements/$nnodes/$out_file"

```

--------------------------------------------------------------------------------
#     13] Acknowledgment                                                       #
--------------------------------------------------------------------------------
MERIC is being developed at [IT4Innovations National Supercomputing Center](https://www.it4i.cz/) under [BSD-3 license](https://code.it4i.cz/vys0053/meric/blob/master/LICENSE).
Please, open an issue, if you meet any problem. 

	
For referencing MERIC, please, cite: **[MERIC and RADAR Generator: Tools for Energy Evaluation and Runtime Tuning of HPC Applications](https://link.springer.com/chapter/10.1007/978-3-319-97136-0_11)**.





