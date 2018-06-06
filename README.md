# MERIC #

Lightweight C/C++ library with Fortran interface for HPC applications dynamic behavior detection with a goal in energy consumption reduction - applying [READEX](https://www.readex.eu/) approach.

The library originally developed for x86 systems (tested on HSW-EP and KNL) but additionally supports selected BSC ARM systems.

--------------------------------------------------------------------------------
#     1] Content of src folder                                                 #
--------------------------------------------------------------------------------
* basis - Input parser
* meric - Base class of the library, that differs in case of use of MPI.
* store - Different types of the store class. To store new type of the data, 
          new class inherited from class StoreBase (store.h) is required.
* wrapper
	- environmentwrapper
		= Thread switching, CPU core and uncore frequencies settings.
		+ http://www.thinkwiki.org/wiki/How_to_use_cpufrequtils
		+ https://doc.zih.tu-dresden.de/hpc-wiki/bin/view/Compendium/X86Adapt

	- hdeemwrapper
		= Energy measurement using hdeem.
		+ https://doc.zih.tu-dresden.de/hpc-wiki/bin/view/Compendium/EnergyMeasurement

	- perfeventwrapper
		= Hardware counters provided by perf_event.
		+ http://man7.org/linux/man-pages/man2/perf_event_open.2.html
	
	- papiwrapper
		= Hardware counters provided by PAPI.
		+ http://icl.cs.utk.edu/papi/
		
	- raplwrapper
		= Intel RAPL counters read by x86_adapt.
	
	- jetsonwrapper and thunderwrapper
		= Support for ARM machines.
	
	- counters
		= List of supported HW counters (PAPI, perfevent, RAPL).

--------------------------------------------------------------------------------
#     2] Content of test folder                                                #
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
	* Command `make` compiles all test codes except blas_test.cpp
	* To compile blas_test.cpp use `make blasTest` command
* environment_taurus
	* Basic script that sets chosen MERIC environment variables and informs you which varibles are set
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

--------------------------------------------------------------------------------
#     3] MERIC interface and Shared Score-P/MERIC API                          #
--------------------------------------------------------------------------------
## MERIC interface ##
If you want to use MERIC with a parallel application, keep in mind, that all processes and all running threads must call every inserted MERIC function, otherwise MERIC behavior is undefined. It is not possible to set runtime environment for each process separately, because MERIC does environment changes that effects whole node (or socket). To guarantee that selected settings is applied for selected region, each measurement start and stop begins with a MPI and OpenMP barrier. MERIC interface is defined in `include/meric.h`.

	MERIC_Init()
		Initialization of the library. Insert directly after MPI_Init().
	MERIC_Close()
		Finalization of the library run to store the measurement results. Insert directly before MPI_Finalize().
	MERIC_MeasureStart("A")
		Starting measurement of a reagion A.
		Please, do not use **#** and **@** in names of regions.
	MERIC_MeasureStop()
		End of the measurement of the last started region.
	MERIC_MeasureStopStart("B")
		End of the measurement of the last started region and start of region B.
		Removes environment switching to configuration of the region we are nested in.
	MERIC_IgnoreStart()
		From this point MERIC doesn't store resources consumption of the following regions but the requested settings of the nested regions is set.
		It is not possible to nest ignore sections of the code.
	MERIC_IgnoreStop()
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
#     4] MERIC input parameters                                                #
--------------------------------------------------------------------------------

## SET MERIC STATIC PARAMETERS - mandatory parameters ##
	export MERIC_FREQUENCY=24        # [0.1 GHz]
	export MERIC_UNCORE_FREQUENCY=24 # [0.1 GHz]
	export MERIC_NUM_THREADS=24      # [0 for applications that doesn't use threads]
		- To run a code in the default settings, without MERIC influence, set this three environment variables to zero.

## SET MERIC WORKING MODE ##
	export MERIC_MODE=0
		0 = hdeem - uses hdeem to measure energy consumption
		1 = rapl - uses rapl counters to measure energy consumption
		2 = hdeem & rapl - uses hdeem and rapl at the same time
		3 = none - doesn't measure energy consumption, but provides you the option to set configuration for inserted regions
		4 = jetson - energy measurement on BSC Jetson TX1 system
		5 = thunder - energy measurement on BSC ThunderX system
	export MERIC_BARRIERS=all
		all  = all barriers are applied (default)
		mpi  = use MPI barriers only
		omp  = use OpenMP barriers only
		none = do not use barriers
		- This settings can by used in MERIC_MODE=3 only.

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
	export MERIC_OUTPUT_FILENAME="log"

## ADVANCED SETTINGS ##
Settings through the exported environment variable should fulfill your needs when manually searching for the optimal settings. To set more complex settings the configuration file must be define.
In the configuration file one can specify settings for each region separately, different settings can be applied for each node and also socket. It is also possible to provide list of regions to ignore (the settings for these regions are applied but no consumptions are measured), and size of change in settings, that should be ignored, because it is too small to apply.
	
To use the extended options, configuration file must be written in JSON format as follows and exported MERIC_REGION_OPTIONS=/path/to/regionoptions.json.

The basic settings via configuration file is HW settings for regions. In this case each region has an object with parameters. Parameters names are the same as exported environment variables, but without the "MERIC_" prefix.
	
In case of per node or per socket settings specification, the objects of region settings are wrapped in another object. Per node settings starts the JSON object with keyword "@NODE" (as well as per socket settings use "@SOCKET"), where the value is an object, that has as a keys ids of the nodes (or sockets), and the value of this object specify the settings for each region.

It is also possible to specify the settings for a socket on a specific node, in this case into "@NODE" object insert "@SOCKET" object, that contains required region settings. If the "@NODE" and "@SOCKET" settings are set in separated objects, the settings for a node has higher priority than settings for a socket. If any region of your code or any parameter is missing, the default setting is set.

	"@SOCKET" : {
		"0" : {
			"A" : {
				"FREQUENCY" : 13,
				"UNCORE_FREQUENCY" : 13
			}
		}
	}
	
To define ignore settings, write a object with the keyword "@IGNORE", and value is another object, that might contain "@REGIONS" with an array of regions' names to ignore, and "@CHANGE" with key object that contains settings with values, that specify how large the change might be to ignore it.

	"@IGNORE" : {
		"@REGIONS" : ["A", "B", "C"],
		"@CHANGE" : {
			"FREQUENCY" : 2,
			"UNCORE_FREQUENCY" : 2,
			"NUM_THREADS" : 2
		}
	}

Examples of region.options files are in test/config directory. The region.options.extra contain all supported settings.

--------------------------------------------------------------------------------
#    5] Compile MERIC and run an instrumented application                      #
--------------------------------------------------------------------------------
	# install/load libraries
		mandatory - intel compiler	#if GCC is used use "make gcc" instead of "make" to compile MERIC
		optionaly - MPI - mandatory for MPI version of the library
		optionaly - HDEEM + freeipmi
		optionaly - PAPI
		optionaly - cpufreq
		optionaly - x86_adapt
		optionaly - numa - mandatory if x86_adapt is missing
		
	# edit Makefile to specify correct paths and compile MERIC
		export LD_LIBRARY_PATH+=:$PATH_TO_MERIC/lib:/usr/local/lib
		make # or make gcc

	# instrument an application according a section 3 and link with the same libraries as meric
		use -lmeric OR FOR MPI APPS -lmericmpi
	
	# or edit test/Makefile and compile a test application
		cd test
		make
	
	# set static frequencies outside of the regions, or inside regions where are not set
		export MERIC_FREQUENCY=25
		export MERIC_UNCORE_FREQUENCY=25
		export MERIC_NUM_THREADS=0
		export MERIC_MODE=3 # no energy measurement

	# run the application as usual
		./test

To understand this example, explore the source file **test.cpp** to see that this test contains regions A, B and C (inside A are two regions B and one region C). For each region you can set CPU core and uncore frequencies and number of threads. How to do so, you will find in section MERIC input parameters.

--------------------------------------------------------------------------------
#     6] Measurement results                                                   #
--------------------------------------------------------------------------------
	# CODE OUTPUT:
	* mericMeasurement
	 	# contains result files in folders with names of the regions
	 	# to change output folder name export MERIC_OUTPUT_DIR="NEW_NAME"
		# 3 types of data in CSV files:
			# CALLTREE - the first line of every CSV file, it's a call stack, so we can see, where is the measured region nested
			# Section label (e.g. '# Job info') - determines a "category" of following data in a file7
			# Data - tuples (mostly pairs) structured like a hash map: key, value
	* mericMeasurementCounters
		# Program start counters, stored in .tmp files.
	
	# EXAMPLE of CSV file contained in mericMeasurement folder:
		# CALLTREE;init_0;Solve_RegCG_singular_dom_itersolver.cpp:109_0;Projector_l_inv_compG_itersolver.cpp:296_0
		# Job info
		JOB ID,8786406.0
		Nodelist,taurusi6125
		Runtime of function [s],0.005079696
		# Blade samples
		15635,135.75
		15636,135.5
		15637,136.125
		15638,136.5
		# Blade summary
		Energy consumption [J] (Stats structure),0.68
		Energy consumption [J] (Samples),0.69068
		Maximal power [W] (Samples),136.5
		Minimal power [W] (Samples),135.5
		Average power [W] (Samples),135.969
		ID of first blade sample,15635
		ID of last blade sample,15638
	
--------------------------------------------------------------------------------
#      7] Code dynamism investigation                                          #
--------------------------------------------------------------------------------
To find the best settings for each region, you should run your code with several possible settings. The measurement results are stored in two directories, that can be analysed using our RADAR tool, that generates a MERIC configuration file for production runs of the application and LaTeX report describing the application behavior.

	# Run an application in several configurations you want to test - in test directory are provided example scripts `run.sh` or `run-mpi.sh`
		```
		export MERIC_MODE=1
		for thread in {24..1..1}
		do
			for cpu_freq in {25..12..1}
			do
				for uncore_freq in {30..12..1} # or {30..12..2}
				do
					export MERIC_OUTPUT_FILENAME=$thread"_"$cpu_freq"_"$uncore_freq
					export MERIC_NUM_THREADS=$thread
					export MERIC_FREQUENCY=$cpu_freq
					export MERIC_UNCORE_FREQUENCY=$uncore_freq
					./test
				done
			done
		done
		```
	
	# Add a description file `measurementInfo.json` to your output data folder. This step is not compulsory but this file helps us for data analysis.
		```
		{
			"Timestamp" : "13.6. 2017 14:59",
			"System"	: "IT4I Salomon",
			"DataFormat": "node_CF_UnCF_config",
			"Note"  	: ""
		}
		```

	# Process the results using RADAR tool
		Repository URL: https://code.it4i.cz/bes0030/readex-radar.git
		1) Set variables in config.py file (description is included in the file)
		2) Launch python3 ./printFullReport.py -configFile path/to/config.py


--------------------------------------------------------------------------------
#     8] MERIC with a Fortran code                                             #
--------------------------------------------------------------------------------
There are Fortran module and interface in the include directory. One can use shared READEX interface `(#include "readex.inc"`) or just the MERIC version (`#include "meric.inc"`). If MERIC interface is used, user should use keyword `call` as usual for Fortran functions, however READEX interface (as well as Score-P Fortran interface) should be used without it. MERIC repository contains a Fortran code example `test/fort_test.f90` to show how the interfaces can be used.
	
--------------------------------------------------------------------------------
#     9] Using MERIC on BSC ARM systems                                        #
--------------------------------------------------------------------------------
To compile MERIC on Jetson, use gcc and PAPI modules and Makefile option arm. The only difference when running a test on Jetson is an energy measurement. There is python energy measurement script that runs on node background and effects CPU. 10 samples per second was selected in the measurement script as a compromise - this samples rate takes ~2% of the CPU load. It is possible to change the rate in tools/getJTX1measurements.py. 

Export MERIC_MODE=4 to activate MERIC on Jetson - otherwise it isn't possible to change frequencies and measure energy consumption. Since Jetson doesn't have non-continual energy measurement interface exporting MERIC_CONTINUAL=0 turns off the energy consumption measurement. To start energy measurement one must export MERIC_CONTINUAL=1.

ARM core and uncore frequencies are much lower than Haswell's. To easily set these frequencies, input values are in kHz. Default frequencies are 518400 kHz core and 408000 kHz uncore. It is recommended to set frequencies from a list made by administrators, see:

	core: /sys/devices/system/cpu/cpu0/cpufreq/scaling_available_frequencies
	102000 204000 307200 403200 518400 614400 710400 825600 921600 1036800 1132800 1224000 1326000

	uncore:	/sys/kernel/debug/clock/emc/possible_rates
	40800 68000 102000 204000 408000 665600 800000 1065600 1331200 1600000

Another supported ARM system is ThunderX. This system is much more powerfull in compare to Jetson/TX1 and it has energy measurement system that doesn't effects the CPUs. Its measurement system measure the energy consumed by all available nodes (one must allocate all four nodes), and its energy measurement samples frequency is approximately 4 samples per second. Unfortunately, the frequency scaling is not supported. To run MERIC on the ThunderX export MERIC_CONTINUAL=1, MERIC_MODE=5.

At BSC ARM systems it is possible to load modules at login node only - it is necessary to load them before running a job. See run-jetson.sh script in the test directory, that shows how to run a test on Jetson.
 
--------------------------------------------------------------------------------
#     10] Tool for static tuning                                               #
--------------------------------------------------------------------------------
MERIC repository also contain a tool, based on MERIC source code, for static energy measurement and CPU frequencies setting. It is located in the tools/staticMERICtool/ and is compiled separately from the MERIC library.

Binaries energyMeasureStart and energyMeasureStop provides RAPL energy measurement for a single node (similar to HDEEM commandline tools startHdeem and stopHdeem), if one wants to do measurement on several nodes multiNodeStaticMeasureStart/Stop.sh scripts are located in the same directory. Since there is no multi-node HDEEM measurement tool, this script provides the option for both energy measurement interfaces. To select which one should be used, the script takes one argument --rapl or --hdeem.

For a static analysis of a selected application, the directory with the tool contain `staticAnalysis.sh` bash script too. It not only runs the application in variety of available HW settings, but also stores the results in format similar to MERIC, so the results can be analysed using RADAR library.








