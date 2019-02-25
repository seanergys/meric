# MERIC #

Lightweight C/C++ library with Fortran interface for HPC applications dynamic behavior detection with a goal in energy consumption reduction - applying [READEX](https://www.readex.eu/) approach.

The library originally developed for x86 systems (tested on HSW, BDW and KNL) but additionally supports OpenPOWER8+ CINECA DAVIDE and selected BSC ARM systems.

--------------------------------------------------------------------------------
#      README Content                                                          #
--------------------------------------------------------------------------------
<!--> master branch links 
 1. [Content of src folder](https://code.it4i.cz/vys0053/meric#1-content-of-src-folder)
 2. [MERIC and TIMEPROF interface and Shared Score-P/MERIC API](https://code.it4i.cz/vys0053/meric#2-meric-and-timeprof-interface-and-shared-score-pmeric-api)
 3. [MERIC binary instrumentation](https://code.it4i.cz/vys0053/meric#3-meric-binary-instrumentation)
 4. [Compilation](https://code.it4i.cz/vys0053/meric#4-compilation)
 5. [MERIC input parameters](https://code.it4i.cz/vys0053/meric#5-meric-input-parameters)
 6. [Content of test folder and example application run](https://code.it4i.cz/vys0053/meric#6-content-of-test-folder-and-example-application-run)
 7. [Code dynamism investigation](https://code.it4i.cz/vys0053/meric#7-code-dynamism-investigation)
 8. [MERIC with a Fortran code](https://code.it4i.cz/vys0053/meric#8-meric-with-a-fortran-code)
 9. [Using MERIC on BSC ARM systems](https://code.it4i.cz/vys0053/meric#9-using-meric-on-bsc-arm-systems)
10. [Using MERIC on D.A.V.I.D.E. system](https://code.it4i.cz/vys0053/meric#10-using-meric-on-cineca-davide-system)
11. [Tool for static tuning](https://code.it4i.cz/vys0053/meric#11-tool-for-static-tuning)
12. [Acknowledgement](https://code.it4i.cz/vys0053/meric#12-acknowledgement)
<!-->
<!--> dev branch links <!-->
 1. [Content of src folder](https://code.it4i.cz/vys0053/meric/tree/dev#1-content-of-src-folder)
 2. [MERIC and TIMEPROF interface and Shared Score-P/MERIC API](https://code.it4i.cz/vys0053/meric/tree/dev#2-meric-and-timeprof-interface-and-shared-score-pmeric-api)
 3. [MERIC binary instrumentation](https://code.it4i.cz/vys0053/meric/tree/dev#3-meric-binary-instrumentation)
 4. [Compilation](https://code.it4i.cz/vys0053/meric/tree/dev#4-compilation)
 5. [MERIC input parameters](https://code.it4i.cz/vys0053/meric/tree/dev#5-meric-input-parameters)
 6. [Content of test folder and example application run](https://code.it4i.cz/vys0053/meric/tree/dev#6-content-of-test-folder-and-example-application-run)
 7. [Code dynamism investigation](https://code.it4i.cz/vys0053/meric/tree/dev#7-code-dynamism-investigation)
 8. [MERIC with a Fortran code](https://code.it4i.cz/vys0053/meric/tree/dev#8-meric-with-a-fortran-code)
 9. [Using MERIC on BSC ARM systems](https://code.it4i.cz/vys0053/meric/tree/dev#9-using-meric-on-bsc-arm-systems)
10. [Using MERIC on D.A.V.I.D.E. system](https://code.it4i.cz/vys0053/meric/tree/dev#10-using-meric-on-cineca-davide-system)
11. [Tool for static tuning](https://code.it4i.cz/vys0053/meric/tree/dev#11-tool-for-static-tuning)
12. [Acknowledgement](https://code.it4i.cz/vys0053/meric/tree/dev#12-acknowledgement)


--------------------------------------------------------------------------------
#     1] Content of src folder                                                 #
--------------------------------------------------------------------------------
* basis    - Input parser.
* meric    - Base classes of the library.
* store    - Different types of the store class. To store new type of the data, 
             new class inherited from class StoreBase (store.h) is required.
* timeprof - Lightweight time-profiling library desined to identify functions 
             to instrument with MERIC.
* wrapper
	- environmentwrapper
		= Thread switching, CPU core and uncore frequencies settings using [x86_adapt](https://github.com/tud-zih-energy/x86_adapt) or [cpufreq](http://www.thinkwiki.org/wiki/How_to_use_cpufrequtils) or [libmsr](https://github.com/LLNL/libmsr) + [msr-safe](https://github.com/LLNL/msr-safe).

	- hdeemwrapper
		= Energy measurement using [HDEEM](https://doc.zih.tu-dresden.de/hpc-wiki/bin/view/Compendium/EnergyMeasurement).

	- perfeventwrapper
		= Hardware counters provided by [perf_event](http://man7.org/linux/man-pages/man2/perf_event_open.2.html).
	
	- papiwrapper
		= Hardware counters provided by [PAPI](http://icl.cs.utk.edu/papi/).
		
	- raplwrapper
		= Intel RAPL counters read by [x86_adapt](https://github.com/tud-zih-energy/x86_adapt).

	- davidewrapper
		= Support for OpenPOWER system [DAVIDE](http://www.hpc.cineca.it/content/davide).
	
	- jetsonwrapper and thunderwrapper
		= Support for [ARM machines](http://montblanc-project.eu/prototypes).
	
	- counters
		= List of supported HW counters (PAPI, perfevent, RAPL).

--------------------------------------------------------------------------------
#     2] MERIC and TIMEPROF interface and Shared Score-P/MERIC API             #
--------------------------------------------------------------------------------
## MERIC interface ##
If you want to use MERIC with a parallel application, keep in mind, that all processes and all running threads must call every inserted MERIC function, otherwise MERIC behavior is undefined. It is not possible to set runtime environment for each process separately, because MERIC does environment changes that effects whole node (or socket). To guarantee that selected settings is applied for selected region, each measurement start and stop begins with a MPI and OpenMP barrier. MERIC interface is defined in `include/meric.h`.

	void MERIC_Init()
		Initialization of the library. Insert directly after MPI_Init().
		MERIC automatically starts a region called same as the analyzed application binary with suffix "_static".
	void MERIC_Close()
		Finalization of the library run to store the measurement results. Insert directly before MPI_Finalize().
		Ends the region that started at the MERIC_Init() time.
	void MERIC_MeasureStart(const char * regionName)
		Starting measurement of a reagion.
		Please, do not use **#** and **@** in names of regions.
	double MERIC_MeasureStop()
		End of the measurement of the last started region.
		Returns runtime od the stopped region in seconds. Only single MPI process per node returns the runtime, the others returns 0.0.
	double MERIC_MeasureStopStart(const char * regionName)
		End of the measurement of the last started region and start of a new region.
		Removes environment switching to configuration of the region the application is nested in.
		Returns runtime od the stopped region in seconds. Only single MPI process per node returns the runtime, the others returns 0.0.
	void MERIC_CaptureScope(const char * regionName)
		C++ and C (no support for Fortran) function to start measurement which will be stopped automatically at the end of the scope. Useful to capture a function that has several return statements.
		This functionality is based on [RAII](https://en.cppreference.com/w/cpp/language/raii) technique and if it should be used for instrumentation of a C application, the application may require compilation with `-fno-exceptions` or `-lstdc++` flag.
	void MERIC_IgnoreStart()
		From this point MERIC doesn't store resources consumption of the following regions but the requested settings of the nested regions is set.
		It is not possible to nest ignore sections of the code.
	void MERIC_IgnoreStop()
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

## TIMEPROF interface ##
The time measurement provided by TIMEPROF is done by master thread of the process MPI_WORLD_COMM rank 0. The interface is defined in `include/timeprof.h`. 

	void MERIC_Init()
		Initialization of the library. Insert directly after MPI_Init().
	void MERIC_Close()
		Finalization of the library run to store the measurement results. Insert directly before MPI_Finalize().
	void TIMEPROF_regionStart(const char * regionName);
		Start time measurement of region called regionName
	double TIMEPROF_regionStop(const char * regionName);
		End of the time measurement, returns region duration in seconds.
	void TIMEPROF_evaluate (unsigned int timeThreshold = 0, const char * fileName = "");
		At the end of the application run might be called this function 
		to evaluate the measurements. It will produce a list of the regions 
		with duration longer than timeThreshold [ms] and store it to the fileName.
		If no threshold provided, it will print complete list of measured functions with its minimum runtime.
		If no output file specified, the list of functions will be printed to stdout.
	void TIMEPROF_captureScope(const char * regionName);
		[RAII](https://en.cppreference.com/w/cpp/language/raii) time measurement of the scope, where specified.
		In case of C applications , the application may require compilation with `-fno-exceptions` or `-lstdc++` flag if you want to use this function.
	double TIMEPROF_getLastRegionDuration();
		Since scope time measurement does not return the time measured, it can be obtained using this function.


## Shared MERIC/Score-P interface ##
Into include folder the C header file `readex.h` and Fortran `readex.inc` were added. Instead of previously presented MERIC API you may use following functions, that provides shared instrumentation for MERIC and Score-P. To use selected library for measurement, compile your code with -DUSE_MERIC or -DUSE_SCOREP for compiler annotated code (phase region only) or -DUSE_SCOREP_MANUAL for manually annotated code. It is not possible to use MERIC and Score-P simultaneously. As an example of the API use, test.cpp and fort_test.f90 example codes use this interface.

TIMEPROF is not currently supported in readex header file.

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
#     3] MERIC binary instrumentation                                          #
--------------------------------------------------------------------------------
Manual application instrumentation is straightforward, however it requires at least some basic knowledge about the target application, access to the source files and also time to instrument the code. All these steps can be overcome when using static binary instrumentation (SBI). MERIC repository contains several tools for binary analysis placed in `tools/DBI` directory, all of them are compiled separately from the MERIC compilation, using Makefile located ibidem. For SBI there are two tools based on [Dyninst library](https://dyninst.org/) `dinst_profile.cpp` (make dinst) and `dins_instrument.cpp` (make sbi). The `dinst_profile` tool provides list of all functions that are defined in the binary (shared libraries are not take into account) and provides information whether the function can or cannot be instrumented.

The second tool `dinst_instrument` provides binary instrumentation with MERIC or TIMEPROF based on list of functions that should be instrumented. In case of TIMEPROF, the list of functions does not have to be provided, in that case all the instrumentable functions are selected. Besides the instrumentation tools also adds all the necessary dependencies, so it is not necessary to recompile the application and link it with them. 
In case of MPI applications it is not only the application that is instrumented, but also the MPI library itself too. For this purpose shared MPI library that is used for the application must be provided. Please, specify full path to the MPI library to omit any possible mistake. Use `ldd` command to detect which MPI library is used for the analyzed application. When running instrumented MPI application `LD_PRELOAD` must be specified to replace default MPI library with the instrumented one. 

See tools' help message for more information.

## Dyninst installation ##
The tools has been developed using Dyninst-10.0.0, any newer version of the library should work too, however following information might be out-of-date with newer Dyninst versions. Dyninst installation is described in its [repository README](https://github.com/dyninst/dyninst) and it is quite simple, however Dyninst compilation may fail on `make install` due to missing sudo rights. Due to that all the tools' compilation paths are set, please, export following environment variables before you compile and use the Dyninst tool.
```
export DYNINST_HOME=/PATH/TO/DYNINST/DIRECTORY
export DYNINSTAPI_RT_LIB=$DYNINST_HOME/build/dyninstAPI_RT/libdyninstAPI_RT.so
export LD_LIBRARY_PATH+=:$DYNINST_HOME/build/dyninstAPI_RT/
```

### known issues ###
 * Instrumented function with the return command only may lost the return value (might be related to previous issue), you may increase `instlimit` TIMEPROF parameter for skipping such function.
 * If instrumented application is compiled with the same list of modules as the dininst tools, sometimes the new binary is corrupted. Can be tested using `ldd` command. If it happens, we suggest to use different set of modules for the application or the Dyninst tool.

--------------------------------------------------------------------------------
#    4] Compilation                                                            #
--------------------------------------------------------------------------------
MERIC is compiled using [waf build system](https://waf.io/), since the system is not well known, a Makefile in the repository root folder is provided. Please, modify libs and include paths according your system paths:

### MERIC used libraries ###
* mandatory  - rt (high precision time measurement)
* optionally - OpenMP (used in default)
* optionally - MPI
* optionally - [PAPI](http://icl.cs.utk.edu/papi/)
* optionally - [perf_event](http://man7.org/linux/man-pages/man2/perf_event_open.2.html)
* optionally - [libmsr](https://github.com/LLNL/libmsr)+[msr-safe](https://github.com/LLNL/msr-safe)
* optionally - cpufreq/[cpupower](https://github.com/torvalds/linux/tree/master/tools/power/cpupower)
* optionally - [x86_adapt](https://github.com/tud-zih-energy/x86_adapt)
* optionally - [numa](https://github.com/numactl/numactl) (mandatory if x86_adapt is missing)
* optionally - [REST-client](https://github.com/mrtazz/restclient-cpp) (mandatory for DiG energy measurement system)
* integrated - [sheredom json parser](https://github.com/sheredom/json.h)

### TIMEPROF used libraries ###
* mandatory  - rt
* optionally - OpenMP
* optionally - MPI

Beside these libraries waf requires Python.

Default compilation expects Intel compiler, if you want to compile using GCC use `make gcc` instead of `make`. Together with MERIC also TIMEPROF is being compiled. If MPI compiler is available, than compilation will produce both MPI and non-MPI versions of the libraries, both using OpenMP. If a MPI application without OpenMP should be analyzed, compilation with `--noopenmp` must be used to compile such version of MERIC. Please, link your application with `-lmeric`/`-ltimeprof` or `-lmericmpi`/`-ltimeprofmpi` for your OpenMP+MPI application or`-lmericmpionly`/`-ltimeprofmpionly` for pure MPI application.

--------------------------------------------------------------------------------
#     5] MERIC input parameters                                                #
--------------------------------------------------------------------------------

## SET MERIC STATIC PARAMETERS - mandatory parameters ##
	export MERIC_FREQUENCY=2400MHz
	export MERIC_UNCORE_FREQUENCY=2GHz
		- both frequencies can be specified as integer in Hz (default), KHz, MHz or GHz
	export MERIC_NUM_THREADS=24
		- To run a code in the default settings, without MERIC influence, set these three environment variables to zero.

## SET MERIC WORKING MODE ##
	export MERIC_MODE=0
		0 = hdeem - uses hdeem to measure energy consumption
		1 = rapl - uses rapl counters to measure energy consumption
		2 = hdeem & rapl - uses hdeem and rapl at the same time
		3 = none - doesn't measure energy consumption, but provides you the option to set configuration for inserted regions
		4 = jetson - energy measurement on BSC Jetson TX1 system
		5 = thunder - energy measurement on BSC ThunderX system
		6 = davide - energy measurement on CINECA D.A.V.I.D.E. system
		7 = time - storing runtime of the regions only

	export MERIC_ITERATION=0
		- if runing an application several times with the same configuration MERIC_ITERATION=$iteration must be exported
		- always start with 0

	export MERIC_BARRIERS=all
		all  = all barriers are applied (default)
		mpi  = use MPI barriers only
		omp  = use OpenMP barriers only
		none = do not use barriers

## SET ONE OF MERIC OUTPUT FORMAT ##
	export MERIC_CONTINUAL=1
		- Single samples are stored in HDEEM internal memory and read at the end of the runtime
		  (with frequency 1000 samples per 1 second for blade and 100 samples in detailed mode for VRs).
		- Minimal overhead - only times of the beginning and the end of measurement
		  are stored (samples are processed after the measurement).
		- in noncontinual mode (MERIC_CONTINUAL=0) energy consumption measured directly 
		  (with HDEEM internal delay) at each region start and end.

	export MERIC_DETAILED=1
		- HDEEM gives us not only data from blade, but data from Voltage Regulators (VR-CPU1,
		  VR-CPU0, VR-DIMMGH, VR-DIMMEF, VR-DIMMCD, VR-DIMMAB) are stored too.
		- In detailed mode RAPL returns values also for each CPU, not only energy consumption for a node.

	export MERIC_DEBUG=1
		- Data are taken both from samples and Stats structure, so we can compare them.
		- Data are taken from blade and Voltage Regulators too.
		- Only for measurement check - there can be larger overhead because of two
		  types of data processing performed simultaneously.

	export MERIC_SAMPLES=1
		- When using HDEEM samples to read the energy consumption, MERIC prints 
		  each sample to the output file, if MERIC_SAMPLES is set.
		- Files can become very big when measured regions run for a long time.

	export MERIC_AGGREGATE=0
		- When running an MPI application, MERIC aggregate the data from all the processes and stores the aggregated results. There are both average values and summary included in the output files.
		- Exporting MERIC_AGGREGATE=0 turns off this behavior and MERIC will store the results for each node separately.

	export MERIC_COUNTERS=papi or perfevent
		- If set you can read HW counters using PAPI or perfevent.
		- When using counters, there is not only counter value but also an information.
		  about average CPU core frequency during the region runtime, computational
		  and arithmetic intensity (if possible to measure).
		- To add a counter you want to measure it is necessary to follow 
		  the instructions in wrapper/counters.h.
	
## SET OUTPUT FILES/FOLDERS NAME ##
	export MERIC_OUTPUT_DIR="hdeemMeasurement"
		- default name is mericMeasurement
	export MERIC_OUTPUT_FILENAME="log"
		- name of the output file is set automatically according specified values of core and uncore CPU frequencies and number of active OpenMP threads, but this is a way how to add filename suffix

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
				"FREQUENCY" : 1300MHz,
				"UNCORE_FREQUENCY" : 1400MHz
			}
		}
	}
	
To define ignore settings, write a object with the keyword "@IGNORE", and value is another object, that might contain "@REGIONS" with an array of regions' names to ignore, and "@CHANGE" with key object that contains settings with values, that specify how large the change might be to ignore it.

	"@IGNORE" : {
		"@REGIONS" : ["A", "B", "C"],
		"@CHANGE" : {
			"FREQUENCY" : 2500MHz,
			"UNCORE_FREQUENCY" : 2GHz,
			"NUM_THREADS" : 2
		}
	}

Examples of region.options files are in test/config directory. The region.options.extra contain all supported settings.

--------------------------------------------------------------------------------
#     6] Content of test folder and example application run                    #
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
export MERIC_MODE=7              # time measurement only

./test                           # run the application as usual
```

--------------------------------------------------------------------------------
#      7] Code dynamism investigation                                          #
--------------------------------------------------------------------------------
MERIC's output is stored in the directory in default named `mericMeasurement` that contains result files in folders with names of the regions. To change output folder name export MERIC_OUTPUT_DIR="NEW_NAME".
Each csv file carry 3 types of data:
 *  CALLTREE - the first line of every CSV file, it's a call stack, so we can see, where is the measured region nested
 *  Section label (e.g. '# Job info') - determines a "category" of following data in a file7
 *  Data - tuples (mostly pairs) structured like a hash map: key, value

To find the best settings for each region, you should run your code with several possible settings. The content of the MERIC's output directories, can be analysed using our RADAR tool, that generates a MERIC configuration file for production runs of the application and LaTeX report describing the application behavior.

	# Run an application in several configurations you want to test - in test directory are provided example scripts `run.sh` or `run-mpi.sh`
		```
		export MERIC_MODE=1
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
						export MERIC_ITERATION=$iter
						./test
					done
				done
			done
		done
		```
	
	# Edit description file `measurementInfo.json` in your output data folder. This step is not compulsory but this file helps you keep information what you have measured.
		```
		{
			"Timestamp" : "Thu Dec  6 15:37:23 2018",
			"System"    : "IT4I Salomon",
			"DataFormat": "node_CF_UnCF_thrds",
			"Note"      : ""
		}
		```

	# Process the results using RADAR tool
		Repository URL: https://code.it4i.cz/bes0030/readex-radar.git
		1) Set variables in config.py file (description is included in the file)
		2) Launch python3 ./printFullReport.py -configFile path/to/config.py


--------------------------------------------------------------------------------
#      8] MERIC with a Fortran code                                            #
--------------------------------------------------------------------------------
There are Fortran module and interface in the include directory. The module is being compiled separately from MERIC, use `make fortran` command. To instrument a Fortran application the Dyninst tool for static binary instrumentation can be used, also manual instrumentation is available.

To allow MERIC manual instrumentation in your Fortran application, add `use meric` command to your program. For the MERIC functions user should use keyword `call` as usual for Fortran functions. Since in Fortran is a problem with C `const *char` all the region names must be ended with `//char(0)` (e.g. `call MERIC_MeasureStart("RegionName"//char(0))`). MERIC repository contains a Fortran code example `test/fort_test.f90` to show how the API can be used.
	
--------------------------------------------------------------------------------
#      9] Using MERIC on BSC ARM systems                                       #
--------------------------------------------------------------------------------
To compile MERIC on Jetson, use gcc and PAPI modules and Makefile option arm. The only difference when running a test on Jetson is an energy measurement. There is python energy measurement script that runs on node background and effects CPU. 10 samples per second was selected in the measurement script as a compromise - this samples rate takes ~2% of the CPU load. It is possible to change the rate in tools/getJTX1measurements.py. 

Export MERIC_MODE=4 to activate MERIC on Jetson - otherwise it isn't possible to change frequencies and measure energy consumption. Since Jetson doesn't have non-continual energy measurement interface exporting MERIC_CONTINUAL=0 turns off the energy consumption measurement. To start energy measurement one must export MERIC_CONTINUAL=1.

ARM core and uncore frequencies are much lower than Haswell's. To easily set these frequencies, input values are in kHz. Default frequencies are 518400 kHz core and 408000 kHz uncore. It is recommended to set frequencies from a list made by administrators, see:

	core: /sys/devices/system/cpu/cpu0/cpufreq/scaling_available_frequencies [kHz]
	102000 204000 307200 403200 518400 614400 710400 825600 921600 1036800 1132800 1224000 1326000

	uncore:	/sys/kernel/debug/clock/emc/possible_rates [kHz]
	40800 68000 102000 204000 408000 665600 800000 1065600 1331200 1600000

Another supported ARM system is ThunderX. This system is much more powerfull in compare to Jetson/TX1 and it has energy measurement system that doesn't effects the CPUs. Its measurement system measure the energy consumed by all available nodes (one must allocate all four nodes), and its energy measurement samples frequency is approximately 4 samples per second. Unfortunately, the frequency scaling is not supported. To run MERIC on the ThunderX export MERIC_CONTINUAL=1, MERIC_MODE=5.

At BSC ARM systems it is possible to load modules at login node only - it is necessary to load them before running a job. See run-jetson.sh script in the test directory, that shows how to run a test on Jetson.

--------------------------------------------------------------------------------
#     10] Using MERIC on CINECA D.A.V.I.D.E. system                            #
--------------------------------------------------------------------------------
To activate posibility of energy measurement provided by DiG system in the MERIC, [REST-client library](https://github.com/mrtazz/restclient-cpp) must be available on the target system. MERIC and the tuned applications must be compiled with the library.

Available CPU core frequencies available on IBM Power8+ are: 4.02, 3.99, 3.96, 3.92, 3.89, 3.86, 3.82, 3.79, 3.76, 3.72, 3.69, 3.66, 3.62, 3.59, 3.56, 3.52, 3.49, 3.46, 3.42, 3.39, 3.36, 3.33, 3.29, 3.26, 3.23, 3.19, 3.16, 3.13, 3.09, 3.06, 3.03, 2.99, 2.96, 2.93, 2.89, 2.86, 2.83, 2.79, 2.76, 2.73, 2.69, 2.66, 2.63, 2.59, 2.56, 2.53, 2.49, 2.46, 2.43, 2.39, 2.36, 2.33, 2.29, 2.26, 2.23, 2.19, 2.16, 2.13, 2.09, 2.06 GHz. For the frequency tuning no extra library is necessary.

--------------------------------------------------------------------------------
#     11] Tool for static tuning                                               #
--------------------------------------------------------------------------------
MERIC repository also contain a tool, based on MERIC source code, for static energy measurement and CPU frequencies setting. It is located in the tools/staticMERICtool/ and is compiled separately from the MERIC library.

Binaries energyMeasureStart and energyMeasureStop provides RAPL energy measurement for a single node (similar to HDEEM commandline tools startHdeem and stopHdeem), if one wants to do measurement on several nodes multiNodeStaticMeasureStart/Stop.sh scripts are located in the same directory. Since there is no multi-node HDEEM measurement tool, this script provides the option for both energy measurement interfaces. To select which one should be used, the script takes one argument `--rapl` or `--hdeem`.

For a static analysis of a selected application, the directory with the tool contain `staticAnalysis.sh` bash script too. It not only runs the application in variety of available HW settings, but also stores the results in format similar to MERIC, so the results can be analysed using RADAR library.

--------------------------------------------------------------------------------
#     12] Acknowledgement                                                      #
--------------------------------------------------------------------------------
MERIC is being developed at [IT4Innovations National Supercomputing Center](https://www.it4i.cz/) under [BSD-3 license](https://code.it4i.cz/vys0053/meric/blob/master/LICENSE).
Please, open an issue, if you meet any problem. 

	
For referencing MERIC, please, cite: **[MERIC and RADAR Generator: Tools for Energy Evaluation and Runtime Tuning of HPC Applications](https://link.springer.com/chapter/10.1007/978-3-319-97136-0_11)**.





