# MERIC #

Lightweight C/C++ library with Fortran interface for HPC applications dynamic behavior detection with a goal in energy consumption reduction - applying [READEX](https://www.readex.eu/) approach.

The library was originally developed for x86 systems (tested on HSW, BDW, KNL, SKL, and CSL) but additionally supports OpenPOWER8+ CINECA DAVIDE and selected BSC ARM systems.

--------------------------------------------------------------------------------
#      README Content                                                          #
--------------------------------------------------------------------------------
<!--> master branch links 
 1. [System parameters tuning](https://code.it4i.cz/vys0053/meric#1-system-parameters-tuning)
 2. [Supported energy measurement systems](https://code.it4i.cz/vys0053/meric#2-supported-energy-measurement-systems)
 3. [MERIC interface and Shared Score-P/MERIC API](https://code.it4i.cz/vys0053/meric#3-meric-interface-and-shared-score-pmeric-api)
 4. [TIMEPROF library](https://code.it4i.cz/vys0053/meric#4-timeprof-library)
 5. [MERIC binary instrumentation](https://code.it4i.cz/vys0053/meric#5-meric-binary-instrumentation)
 6. [Compilation](https://code.it4i.cz/vys0053/meric#6-compilation)
 7. [MERIC input parameters](https://code.it4i.cz/vys0053/meric#7-meric-input-parameters)
 8. [Content of test folder and example application run](https://code.it4i.cz/vys0053/meric#8-content-of-test-folder-and-example-application-run)
 9. [Code dynamism investigation](https://code.it4i.cz/vys0053/meric#9-code-dynamism-investigation)
10. [MERIC with a Fortran code](https://code.it4i.cz/vys0053/meric#10-meric-with-a-fortran-code)
11. [Tool for static tuning](https://code.it4i.cz/vys0053/meric#11-tool-for-static-tuning)
12. [Acknowledgement](https://code.it4i.cz/vys0053/meric#12-acknowledgement)
<!-->

<!--> dev branch links <!-->
 1. [System parameters tuning](https://code.it4i.cz/vys0053/meric/tree/dev#1-system-parameters-tuning)
 2. [Supported energy measurement systems](https://code.it4i.cz/vys0053/meric/tree/dev#2-supported-energy-measurement-systems)
 3. [MERIC interface and Shared Score-P/MERIC API](https://code.it4i.cz/vys0053/meric/tree/dev#3-meric-interface-and-shared-score-pmeric-api)
 4. [TIMEPROF library](https://code.it4i.cz/vys0053/meric/tree/dev#4-timeprof-library)
 5. [MERIC binary instrumentation](https://code.it4i.cz/vys0053/meric/tree/dev#5-meric-binary-instrumentation)
 6. [Compilation](https://code.it4i.cz/vys0053/meric/tree/dev#6-compilation)
 7. [MERIC input parameters](https://code.it4i.cz/vys0053/meric/tree/dev#7-meric-input-parameters)
 8. [Content of test folder and example application run](https://code.it4i.cz/vys0053/meric/tree/dev#8-content-of-test-folder-and-example-application-run)
 9. [Code dynamism investigation](https://code.it4i.cz/vys0053/meric/tree/dev#9-code-dynamism-investigation)
10. [MERIC with a Fortran code](https://code.it4i.cz/vys0053/meric/tree/dev#10-meric-with-a-fortran-code)
11. [Tool for static tuning](https://code.it4i.cz/vys0053/meric/tree/dev#11-tool-for-static-tuning)
12. [Acknowledgement](https://code.it4i.cz/vys0053/meric/tree/dev#12-acknowledgement)


--------------------------------------------------------------------------------
#     1] System parameters tuning                                              #
--------------------------------------------------------------------------------
MERIC allows to set selected system parameters for the whole application run or for each instrumented part of the analyzed application. Following list of the system parameters can be tuned using MERIC. Default and valid values of the following parameters can be read using `systemInfo` utility (part of the MERIC repository, located in the tools/ directory).


### CPU core frequency ###
MERIC provides several ways how to change the CPU core frequency:
 *  Intel processor P-State tuning via MSR - require [msr-safe](https://github.com/LLNL/msr-safe), ideally together with [libmsr](https://github.com/LLNL/libmsr). Under `intel_pstate` scaling driver requires additional write acess to `/sys/devices/system/cpu/intel_pstate/no_turbo` and `/sys/devices/system/cpu/cpu*/cpufreq/scaling_max_freq`.
 *  Using [x86_adapt](https://github.com/tud-zih-energy/x86_adapt).
 *  Using cpufreq/[cpupower](https://github.com/torvalds/linux/tree/master/tools/power/cpupower).
 *  Using direct write to the `/sys/devices/system/cpu/cpu*/cpufreq/scaling_setspeed`.


### Intel CPU uncore frequency ###
Intel uncore frequency refers to frequency of subsystems in the physical processor package that are shared by multiple processor cores. E.g., L3 cache or on-chip ring interconnect.

MERIC requires [msr-safe](https://github.com/LLNL/msr-safe), ideally together with [libmsr](https://github.com/LLNL/libmsr) or [x86_adapt](https://github.com/tud-zih-energy/x86_adapt) or [GEOPM](https://geopm.github.io/) to be able to change the system uncore frequency. 


### Intel RAPL power capping ###
Intel processors from Sandy Bridge architecture provides Runnnig Average Power Limit (RAPL), that keeps the power consumption of the available domains bellow the specified power limit in a specific time window (long-term and short-term windows). MERIC allows to specify power limit of the Package power domain within its long-term time window, which can be also adjusted.

MERIC requires [msr-safe](https://github.com/LLNL/msr-safe), ideally together with [libmsr](https://github.com/LLNL/libmsr) or [x86_adapt](https://github.com/tud-zih-energy/x86_adapt) or [GEOPM](https://geopm.github.io/) to be able change the power limits. Currently only PKG power cap control is supported.
	
Please, note that RAPL power capping system uses equation to specify the window size for PKG: `PKGTimeLimit = 2^Y * (1.0 + Z/4.0) * TimeUnit`, where Y=<0, 31>, Z=<0,3> and TimeUnit is usually a fraction of microsecond. <!--> and window size for DRAM: `DRAMTimeLimit = 2^Y*F * TimeUnit`, where F={1.0, 1.1, 1.2, 1.3} and Y=<0, 31>. <!--> Due to that it might be impossible to set exactly the window size you have specified.


### Number of active OMP threads ###
Using OpenMP API.


### Nvidia GPU streaming multiprocessors frequency ###
Some Nvidia GPUs provide possibility to tune frequency of streaming multiprocessors. The frequencies are much more finegrain then frequencies of server processors, while only specific values are possible to set. Also, the available SM frequencies depend on frequency of the GPU memory, if the memory frequency is possible to tune. User may specify any frequency from the minimum - maximum frequency range, while MERIC will find the closest available frequency and will set it.
To tune the SM frequency, MERIC requires [Nvidia Management Library (NVML)](https://developer.nvidia.com/nvidia-management-library-nvml). To tune GPU frequencies without sudo rights, superuser must execute command `sudo nvidia-smi -acp UNRESTRICTED` (feature removed from driver 465.xx).


### Nvidia GPU memory frequency ###
Nvidia GPUs with GDDR memory provide possibility to tune its frequency. As well as in case of the SMs, MERIC sets the closest available frequency, which is possible to set. Requires [Nvidia Management Library (NVML)](https://developer.nvidia.com/nvidia-management-library-nvml). To tune GPU frequencies without sudo rights, superuser must execute command `sudo nvidia-smi -acp UNRESTRICTED` (feature removed from driver 465.xx).


### Nvidia GPU power capping ###
Using [Nvidia Management Library (NVML)](https://developer.nvidia.com/nvidia-management-library-nvml) to set power limit of Nvidia GPUs. Requires sudo rights.


### ARM Jetson memory frequency ###
To simplify MERIC's interface, the memory frequency is set the same way as Intel CPU uncore frequency using MERIC_UNCORE_FREQUENCY environment variable.


--------------------------------------------------------------------------------
#     2] Supported energy measurement systems                                  #
--------------------------------------------------------------------------------
Energy measurement system can be selected using `MERIC_MEASURE` environment variable.

### RAPL ###
From the Sandy Bridge microarchitecture all Intel processors provides software-based energy meter Runnnig Average Power Limit (RAPL) with sampling frequency 1 kHz of the Package and DRAM. The RAPL does not cover energy consumption of the node itself. Contemporary AMD CPUs have the same energy consumption meter, in contrast to Intel implementation it is not used for power capping, exposing energy consumption of the Package and each CPU core separatelly.

Since we are speaking about performance counters (non-continual mode only), overflow may happen during the application run. MERIC implements a overflow detection and recovery system, however it cannot fix the values if two or more overflows happen during a region that does not have any nested region. Please, keep it in mind when specifying a region size.

Based on these counters Intel processors can limit its energy consumption, using RAPL power capping system. User may specify average PKG or DRAM power consumption for a specific time window and the system itself will reduce its resources to fulfill this constraint.
	
In MERIC [msr-safe](https://github.com/LLNL/msr-safe) + ideally [libmsr](https://github.com/LLNL/libmsr) or [x86_adapt](https://github.com/tud-zih-energy/x86_adapt) or [GEOPM](https://geopm.github.io/) is required to access the RAPL counters at Intel architecture. AMD architectures require [msr-safe](https://github.com/LLNL/msr-safe) to be installed. In the detailed mode MERIC stores energy consumption for each power domain, otherwise it aggregates the values.


### HDEEM ###
[High Definition Energy Efficiency Monitoring (HDEEM)](https://tu-dresden.de/zih/forschung/projekte/hdeem) is Bull's stand alone energy measurement system located on the node itself, with a power sampling frequency 1 kHz (blade), 100 Hz (on node components) respectively. HDEEM stores power samples in its internal memory and MERIC can read the energy consumption when a region starts and ends with significant overhead of the HDEEM system, or when the application run is over, without any impact on the application run. Both continual and non-continual modeas are available, however considering these features continual mode is prefered.


### DiG ###
Dwarf in Giant (DiG) is a hardware stand-alone power sampling system that stores the samples, that can be accessed through the [EXAMON](http://projects.eees.dei.unibo.it/monitoring/wordpress/) framework. Continual mode available only.

To activate possibility of energy measurement provided by DiG system in the MERIC, [REST-client library](https://github.com/mrtazz/restclient-cpp) must be available on the target system. MERIC and the tuned applications must be compiled with the library.


### NVML ###
[Nvidia Managemenet Library (NVML)](https://developer.nvidia.com/nvidia-management-library-nvml) allows to measure energy consumption of Nvidia GPUs Volta or newer architectures. Nvidia Volta GPUs samples the power consumption once every 20 ms, regions should be at least a second long to retrieve reliable energy measurment. Currently only non-continual mode is supported.


### BSC ARM Jetson ###
Part of the MERIC repository is a python power measurement script that runs on node background and effects CPU. The script stores the power samples into a file, and MERIC evaluates energy consumption of the instrumented regions when the application run is over. Ten samples per second was selected in the measurement script as a compromise - this sampling rate takes ~2% of the CPU load. It is possible to change the rate in tools/getJTX1measurements.py. Continual mode available only.


### BSC ARM ThunderX ###
BSC's ThunderX energy measurement system doesn't effects the CPUs, however it measures energy consumed by all available nodes (one must allocate all four nodes), and its energy measurement samples frequency is approximately 4 samples per second. Continual mode available only.

--------------------------------------------------------------------------------
#     3] MERIC interface and Shared Score-P/MERIC API                          #
--------------------------------------------------------------------------------
## MERIC interface ##
If you want to use MERIC with a parallel application, keep in mind, that all processes and all running threads must call every inserted MERIC function, otherwise MERIC behavior is undefined. It is not possible to set runtime environment for each process separately, because MERIC does environment changes that effects whole node (or socket). To guarantee that selected settings is applied for selected region, each measurement start and stop begins with an MPI and OpenMP barrier. MERIC interface is defined in `include/meric.h`.

 *	`void MERIC_Init()`

	Initialization of the library. Insert directly after MPI_Init().
	MERIC automatically starts a region called same as the analyzed application binary with suffix "_static".

 *	`void MERIC_Close()`

	Finalization of the library run to store the measurement results. Insert directly before MPI_Finalize().
	Ends the region that started at the MERIC_Init() time.

 *	`void MERIC_MeasureStart(const char * regionName)`

	Starting measurement of a reagion.
	Please, do not use `#` and `@` in names of regions.

 *	`double MERIC_MeasureStop()`

	End of the measurement of the last started region.
	Returns runtime od the stopped region in seconds. Only single MPI process per node returns the runtime, the others returns 0.0.

 *	`double MERIC_MeasureStopStart(const char * regionName)`

	End of the measurement of the last started region and start of a new region.
	Removes environment switching to configuration of the region the application is nested in.
	Returns runtime od the stopped region in seconds. Only single MPI process per node returns the runtime, the others returns 0.0.

 *	`void MERIC_CaptureScope(const char * regionName)`

	C++ and C (no support for Fortran) function to start measurement which will be stopped automatically at the end of the scope. Useful to capture a function that has several return statements.
	This functionality is based on [RAII](https://en.cppreference.com/w/cpp/language/raii) technique and if it should be used for instrumentation of a C application, the application may require compilation with `-fno-exceptions` or `-lstdc++` flag.

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
	MERIC_MeasureStop()

	MERIC_MeasureStart("B")
		MERIC_MeasureStart("A")	//region A called at higher nested level
		MERIC_MeasureStop()
	MERIC_MeasureStop()

	MERIC_MeasureStart("A")	//next region A call at the same nested level as the first one
		MERIC_MeasureStart("C")	//the problem is here, when region A has another nested region
		MERIC_MeasureStop()	//region C will cause defect in its call tree
	MERIC_MeasureStop()
MERIC_MeasureStop()
```

## Shared MERIC/Score-P interface ##
Into include folder the C header file `readex.h` and Fortran `readex.inc` were added. Instead of previously presented MERIC API you may use following functions, that provides shared instrumentation for MERIC and Score-P. To use selected library for measurement, compile your code with -DUSE_MERIC or -DUSE_SCOREP for compiler annotated code (phase region only) or -DUSE_SCOREP_MANUAL for manually annotated code. It is not possible to use MERIC and Score-P simultaneously. As an example of the API use, test.cpp and fort_test.f90 example codes use this interface.

Used parameters has the same datatype as Score-P functions: struct SCOREP_User_Region* handle, const char* name, uint32_t type.

| Shared API                                | MERIC function           | Score-P function                               |
| ----------------------------------------- | ------------------------ | ---------------------------------------------- |
| READEX_INIT()                             | MERIC_Init()             |                                                |
| READEX_CLOSE()                            | MERIC_Close()            |                                                |
| READEX_PHASE_REGION_DEFINE(handle)        |                          | SCOREP_USER_REGION_DEFINE(handle)              |
| READEX_SIGNIFICANT_REGION_DEFINE(handle)* |                          | SCOREP_USER_REGION_DEFINE(handle)              |
| READEX_REGION_START(handle, name, type)   | MERIC_MeasureStart(name) | SCOREP_USER_REGION_BEGIN(handle, name, type)   |
| READEX_REGION_STOP(handle)                | MERIC_MeasureStop()      | SCOREP_USER_REGION_END(handle)                 |
| READEX_REGION_STOP_START(stop_handle, start_handle, start_name, start_type) | MERIC_MeasureStopStart (start_name) | SCOREP_USER_REGION_END(stop_handle) SCOREP_USER_REGION_BEGIN (start_handle, start_name, start_type) |
| READEX_PHASE_START(handle, name, type)    | MERIC_MeasureStart(name) | SCOREP_USER_OA_PHASE_BEGIN(handle, name, type) |
| READEX_PHASE_STOP(handle)                 | MERIC_MeasureStop()      | SCOREP_USER_OA_PHASE_END(handle)               |
| READEX_IGNORE_START()                     | MERIC_IgnoreStart()      | SCOREP_RECORDING_OFF()                         |
| READEX_IGNORE_STOP()                      | MERIC_IgnoreStop()       | SCOREP_RECORDING_ON()                          |
    *  Defines region handle except of phase region

READEX interface doesn't contain all Score-P API functions, because there is no support for these in MERIC. For the rest functionality user may use usual Score-P API, the functions will be ignored if the code will be compiled without Score-P.


--------------------------------------------------------------------------------
#     4] TIMEPROF library                                                      #
--------------------------------------------------------------------------------

The TIMEPROF library provides light-weight runtime measurement of the instrumented regions in the code. The library is part of the MERIC repository since its main task is to identify regions for the future instrumentation with MERIC.

The library produces a list of the regions, which duration is longer than time threshold (export environment variable `TIMEPROF_TIME` [ms]). When no threshold provided, the TIMEPROF will print complete list of measured regions with its minimum runtime. If environment variable `TIMEPROF_OUTPUT` exported, the list of regions is stored to the file, otherwise it is printed to the stdout.

The time measurement provided by TIMEPROF is done by master thread of the process MPI_WORLD_COMM rank 0.


## TIMEPROF interface ##
The interface is defined in `include/timeprof.h`. 

 *	`void TIMEPROF_regionStart(const char * regionName)`

	Start time measurement of region called regionName.

 *	`double TIMEPROF_regionStop(const char * regionName)`
 
	End of the time measurement, returns region duration in seconds.

 *	`void TIMEPROF_evaluate()`

	At the end of the application run should be called this function to evaluate the measurements.

 *	`void TIMEPROF_captureScope(const char * regionName)`

	[RAII](https://en.cppreference.com/w/cpp/language/raii) time measurement of the scope, where specified.
	In case of C applications, the application may require compilation with `-fno-exceptions` or `-lstdc++` flag if you want to use this function.

 *	`double TIMEPROF_getLastRegionDuration()`

	Since scope time measurement does not return the time measured, it can be obtained using this function.

--------------------------------------------------------------------------------
#     5] MERIC binary instrumentation                                          #
--------------------------------------------------------------------------------
Manual application instrumentation is straightforward, however it requires at least some basic knowledge about the target application, access to the source files and also time to instrument the code. All these steps can be overcome when using static binary instrumentation (SBI). We provide two tools based on [Dyninst library](https://dyninst.org/):
 * `dinst_profile` - Provides list of all functions that are defined in the binary (shared libraries are not taken into account) and provides information whether the function can or cannot be instrumented.
 * `dinst_instrument` - Provides binary instrumentation with MERIC or TIMEPROF based on list of functions that should be instrumented. In case of TIMEPROF, the list of functions does not have to be provided, in that case all the instrumentable functions are selected. Besides the instrumentation tools also adds all the necessary dependencies, so it is not necessary to recompile the application and link it with them. In case of MPI applications it is not only the application that is instrumented, but also the MPI library itself too. For this purpose shared MPI library that is used for the application must be provided. Please, specify full path to the MPI library to omit any possible mistake. Use `ldd` command to detect which MPI library is used for the analyzed application. When running instrumented MPI application `LD_PRELOAD` must be specified to replace default MPI library with the instrumented one. 

See tools' help message for more information.

To run the SBI tools it is not only necessary to update LD_LIBRARY_PATH, but also export `DYNINSTAPI_RT_LIB` variable to specify `libdyninstAPI_RT.so` location. This shared library, which does not have a compiler dependency, is the only added dependency to generated binary in compare to manually instrumented application.

### known issues ###
 * Instrumented function with the return command only may lost the return value, you may increase `instlimit` TIMEPROF parameter for skipping such function.
 * If instrumented application is compiled with the same list of modules as the Dyninst tools, sometimes the new binary is corrupted. Can be tested using `ldd` command. If it happens, we suggest to use different set of modules for the application or the Dyninst tool.

--------------------------------------------------------------------------------
#    6] Compilation                                                            #
--------------------------------------------------------------------------------
MERIC is compiled using [waf build system](https://waf.io/), since the system is not well known, a Makefile in the repository root folder is provided. Please, modify libs and include paths according your system paths:

### MERIC used libraries ###
* mandatory  - [rt](https://man.cx/librt(3))
* optionally - OpenMP (used in default)
* optionally - MPI
* optionally - [PAPI](http://icl.cs.utk.edu/papi/)
* optionally - [perf_event](http://man7.org/linux/man-pages/man2/perf_event_open.2.html)
* optionally - [msr-safe](https://github.com/LLNL/msr-safe) - prefered interface for Intel and AMD CPUs parameters tuning
* optionally - [libmsr](https://github.com/LLNL/libmsr) - prefered to be used for Intel CPUs parameters tuning
* optionally - [HDEEM](https://tu-dresden.de/zih/forschung/projekte/hdeem?set_language=en)
* optionally - [FreeIPMI](http://www.gnu.org/software/freeipmi/) - mandatory for the HDEEM power monitoring
* optionally - cpufreq/[cpupower](https://github.com/torvalds/linux/tree/master/tools/power/cpupower)
* optionally - [x86_adapt](https://github.com/tud-zih-energy/x86_adapt)
* optionally - [GEOPM](https://geopm.github.io/)
* optionally - [NVML](https://developer.nvidia.com/nvidia-management-library-nvml) - mandatory for the Nvidia GPUs monitoring and tuning
* optionally - [numa](https://github.com/numactl/numactl)
* optionally - [REST-client](https://github.com/mrtazz/restclient-cpp) - mandatory for the DiG power monitoring
* integrated - [sheredom json parser](https://github.com/sheredom/json.h)

### TIMEPROF used libraries ###
* mandatory  - [rt](https://man.cx/librt(3))
* optionally - OpenMP
* optionally - MPI

### SBI tools ###
The Static Binary Instrumentation tools use Dyninst library for binary analysis and following modification. The Dyninst can be compiled using GNU compiler only, despite it supports also binary instrumentation generated by others compilers. Due to this limitation the SBI tools are compiled together with MERIC only when using GNU compiler. It is possible to compile SBI tools only when using `make sbi`.
* mandatory - [Dyninst](https://dyninst.org/)
* mandatory - [Boost](https://www.boost.org/) - date_time, system, and thread

Beside all the mentioned libraries waf build system requires Python.

The compilation mechanism detects the compiler type, however if the detection fails, the default is GNU C++ compiler. To compile MERIC using any other compiler, you may specify `--cpp=` and `--mpi=` flags for `./waf configure` command, or use `make intel` for compilation using Intel compiler. The compilation compiles MERIC, TIMEPROF and all the tools, except the SBI tools when GNU compiler is not used, or any of the dependecies is missing. If MPI compiler is available, than compilation will produce both MPI and non-MPI versions of the libraries, both using OpenMP. If an MPI application without OpenMP should be analyzed, compilation with `--noopenmp` waf flag must be used to compile such version of MERIC. Please, link your application with `-lmeric`/`-ltimeprof` or `-lmericmpi`/`-ltimeprofmpi` for your OpenMP+MPI application or`-lmericmpionly`/`-ltimeprofmpionly` for pure MPI application.

--------------------------------------------------------------------------------
#     7] MERIC input parameters                                                #
--------------------------------------------------------------------------------

## SET MERIC STATIC PARAMETERS - mandatory parameters ##
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

## SET MERIC MEASUREMENT MODE ##
* `export MERIC_MEASUREMENT=RAPL,HDEEM-S`
	- NONE    = Default option, no time or energy measurement.
	- HDEEM-S = Bull|Atos HDEEM energy and power measurement, evaluating energy consumption from power samples ("continual mode").
	- HDEEM-C = Bull|Atos HDEEM energy and power measurement, the system provides energy consumption directly at the beginning and end of each region. This mode may cause extra overhead in compare to evaluating from power samples.
	- HDEEM   = Equals to HDEEM-S option.
	- RAPL    = Intel RAPL energy measurement.
	- NVML    = Nvidia GPUs energy measurement.
	- DAVIDE  = DiG energy and power measurement at CINECA D.A.V.I.D.E. system.
	- JETSON  = BSC Jetson TX1 energy and power measurement.
	- THUNDER = BSC ThunderX energy and power measurement.
	- TIME    = Runtime measuement, default for other modes.

* `export MERIC_BARRIERS=all`
	- all  = all barriers are applied (default)
	- mpi  = use MPI barriers only
	- omp  = use OpenMP barriers only
	- none = do not use barriers

## SET MERIC OUTPUT FORMAT ##
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
	- Available only in non-aggregate (MERIC_AGGREGATE=0) continual mode (such as HDEEM-S).
	- WARNING: Output may become very big when measured regions run for a long time.

* `export MERIC_DEBUG=1`
	- Uses HDEEM-S and HDEEM-C energy measurement at the same time.
	- Ignores MERIC_DETAILED input parameter and evaluates all the voltage regulators.
	- Only for measurement check - there can be larger overhead because of two types of data processing performed simultaneously.

## SET OUTPUT FILES/FOLDERS NAME ##
* `export MERIC_OUTPUT_DIR="hdeemMeasurement"`
	- Default name is mericMeasurement.

* `export MERIC_OUTPUT_FORMAT="CF_UnCF"`
	- Underscore separated list of parameters. MERIC automatically determines which parameters are tuned, and compose output filename accordingly. This parameter allows user force MERIC to extend the output files name about parameters that are not tuned. Zero will represent the non-tuned parameter. This parameter does not influence parameters order. Each supported parameter is represent by one of the following strings:
	  -	`CF` = CPU core frequency
	  - `UnCF` = CPU uncore frequency
	  - `thrds` = number of threads
	  - `CPUPWR` = CPU power limit
	  - `CPUPWRtw` = CPU power limit time window
	  - `GPUF` = GPU streaming multiprocessors frequency
	  - `GPUMEMF` = GPU memory frequency
	  - `GPUPWR` = GPU power limit

* `export MERIC_OUTPUT_FILENAME="log"`
	- Name of the output file is set automatically according specified values of the tuned system parameters, however this is a way how to add filename suffix.

## ADVANCED SETTINGS ##
Settings through the exported environment variable should fulfill your needs when manually searching for the optimal settings. To set more complex settings the configuration file must be define.
In the configuration file one can specify settings for each region separately, different settings can be applied for each node and also socket. It is also possible to provide list of regions to ignore (the settings for these regions are applied but no consumptions are measured), and size of change in settings, that should be ignored, because it is too small to apply.
	
To use the extended options, configuration file must be written in JSON format as follows and `export MERIC_REGION_OPTIONS=/path/to/regionoptions.json`.

The basic settings via configuration file is HW settings for regions. In this case each region has an object with parameters. Parameters names are the same as exported environment variables, but without the "MERIC_" prefix.
	
In case of per node or per socket settings specification, the objects of region settings are wrapped in another object. Per node settings starts the JSON object with keyword "@NODE" (as well as per socket settings use "@SOCKET"), where the value is an object, that has as a keys ids of the nodes (or sockets), and the value of this object specify the settings for each region.

It is also possible to specify the settings for a socket on a specific node, in this case into "@NODE" object insert "@SOCKET" object, that contains required region settings. If the "@NODE" and "@SOCKET" settings are set in separated objects, the settings for a node has higher priority than settings for a socket. If any region of your code or any parameter is missing, the default setting is set.

	"@SOCKET" : {
		"0" : {
			"A" : {
				"FREQUENCY" : "1300MHz",
				"UNCORE_FREQUENCY" : "1400MHz"
			}
		}
	}
	
To define ignore settings, write a object with the keyword "@IGNORE", and value is another object, that might contain "@REGIONS" with an array of regions' names to ignore, and "@CHANGE" with key object that contains settings with values, that specify how large the change might be to ignore it.

	"@IGNORE" : {
		"@REGIONS" : ["A", "B", "C"],
		"@CHANGE" : {
			"PWRCAP_POWER" : 10000,
			"NUM_THREADS" : 2
		}
	}

Examples of region.options files are in test/config directory. The region.options.extra contain all supported settings.

--------------------------------------------------------------------------------
#     8] Content of test folder and example application run                    #
--------------------------------------------------------------------------------
* source codes
	* test.cpp
		* One region, with two another regions inside.
		* This test uses shared Score-P/MERIC API. See section 3 of this README.
	* test_mpi.cpp
		* Same test as test.c, only extended with MPI.
	* fort_test.f90
		* Fortran version of test.cpp to show the MERIC and READEX Fortran interface.
	* samples_test.cpp
		* Test with sleep (minimum energy consumption) and compute (much higher energy consumption) regions in a loop. This allows user to see in the list of samples how energy rise when compute region starts and check if MERIC detects this sample as a first one of this region.
		* Follow the instructions inside to set the test.
	* sleep_test.cpp
		* A test with RUN (maximum load) and SLEEP (minimum load) regions, both with the same runtime.
		* Originally made to control real CPU frequency of the machine.
	* overhead_test.cpp
		* Test to measure MERIC overhead and overhead of libraries to change environment parameters.
	* blas_test.cpp
		* Test compares DGEMM and DGEMV. There are two possible sizes of matrices (large and small (not) to fit in L3 cache). In both cases, sizes of matrices were set to take approximately the same time for both DGEMM and DGEMV region, when using all available resources.
		* This test requires mkl library, due to that it is compiled using `make blasTest` alongside to other tests.
* Makefile
	* Command `make` compiles all test codes except blas_test.cpp.
	* To compile blas_test.cpp use `make blasTest` command.
* environment_default.source
	* Basic script that sets chosen MERIC environment variables and informs you which varibles are set.
	* When run with argument `-t`, the script just prints list of set variables.
	* Make a copy of this script and edit it to suits your needs.
* config direcory
	* region.options
		* File that sets exact settings for regions inside your code.
		* In default it is set for blas_test.
	* region.options.extra
		* Configuration file, that shows all available ways how to specify MERIC settings.
* run.sh and run-mpi.sh
	* Scripts that runs MERIC anslysis of test.cpp or test_mpi.cpp on Taurus machine.
* run-jetson.sh
	* Template script to submit a job on BSC ARM Jetson platform.
	

Specify the mandatory MERIC's parameters and run you instrumented application or one from the test directory. Good starting point is runnig `test` application from test directory. To understand well MERIC's output, explore the source file **test.cpp** to see that this test contains regions A, B and C (inside A are two regions B and one region C). For each region you can set CPU core and uncore frequencies and number of threads.
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
#      9] Code dynamism investigation                                          #
--------------------------------------------------------------------------------
MERIC's output is stored in the directory in default named `mericMeasurement` that contains result files in folders with names of the regions. To change output folder name export MERIC_OUTPUT_DIR="NEW_NAME".
Each csv file carry 3 types of data:
 *  CALLTREE - the first line of every CSV file, it's a call stack, so we can see, where is the measured region nested
 *  Section label (e.g. '# Job info') - determines a "category" of following data in a file7
 *  Data - tuples (mostly pairs) structured like a hash map: key, value

To find the best settings for each region, you should run your code with several possible settings. The content of the MERIC's output directories, can be analysed using our RADAR tool, that generates a MERIC configuration file for production runs of the application and LaTeX report describing the application behavior.

## Run an application in several configurations you want to test ##
In test directory are provided example scripts `run.sh` or `run-mpi.sh`
```
export MERIC_MEASURE=RAPL,HDEEM
export MERIC_PWRCAP_POWER=0
export MERIC_PWRCAP_TIME=0
export MERIC_GPU_FREQUENCY=0
export MERIC_GPU_MEM_FREQUENCY=0
export MERIC_PWRCAP_GPU=0

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
				./test
			done
		done
	done
done
```
	
Edit description file `measurementInfo.json` in your output data folder. This step is not compulsory but this file helps you keep information about the measurement.
```
{
	"Timestamp" : "Thu Dec  6 15:37:23 2018",
	"System"    : "IT4I Salomon",
	"DataFormat": "node_CF_UnCF_thrds",
	"Note"      : ""
}
```

## Process the results using RADAR tools ##
We have implemented a python tools for the MERIC's output data analysis. All the details are descibed in its repository: [RADAR GUI](https://code.it4i.cz/vys0053/SGS18-READEX).

--------------------------------------------------------------------------------
#      10] MERIC with a Fortran code                                           #
--------------------------------------------------------------------------------
There are Fortran module and interface in the include directory. The module is being compiled separately from MERIC, use `make fortran` command. To instrument a Fortran application the Dyninst tool for static binary instrumentation can be used, also manual instrumentation is available.

To allow MERIC manual instrumentation in your Fortran application, add `use meric` command to your program. For the MERIC functions user should use keyword `call` as usual for Fortran functions. Since in Fortran is a problem with C `const *char` all the region names must be ended with `//char(0)` (e.g. `call MERIC_MeasureStart("RegionName"//char(0))`). MERIC repository contains a Fortran code example `test/fort_test.f90` to show how the API can be used.
	
--------------------------------------------------------------------------------
#     11] Tool for static tuning                                               #
--------------------------------------------------------------------------------
MERIC repository also contain a tool, based on MERIC source code, for static energy measurement and system parameters tuning. It is located in the tools/ directory.

Binaries `energyMeasureStart` and `energyMeasureStop` provides RAPL energy measurement for a single node (similar to HDEEM commandline tools startHdeem and stopHdeem), if one wants to do measurement on several nodes multiNodeStaticMeasureStart/Stop.sh scripts are located in the same directory. Since there is no multi-node HDEEM measurement tool, this script provides the option for both energy measurement interfaces. To select which one should be used, the script takes one argument `--rapl` or `--hdeem`.

For a static analysis of a selected application, the directory with the tool contains `staticAnalysis.sh` bash script too. It not only runs the application in a variety of available hardware settings, but also stores the results in format similar to MERIC, so the results can be analysed using RADAR library.

--------------------------------------------------------------------------------
#     12] Acknowledgement                                                      #
--------------------------------------------------------------------------------
MERIC is being developed at [IT4Innovations National Supercomputing Center](https://www.it4i.cz/) under [BSD-3 license](https://code.it4i.cz/vys0053/meric/blob/master/LICENSE).
Please, open an issue, if you meet any problem. 

	
For referencing MERIC, please, cite: **[MERIC and RADAR Generator: Tools for Energy Evaluation and Runtime Tuning of HPC Applications](https://link.springer.com/chapter/10.1007/978-3-319-97136-0_11)**.





