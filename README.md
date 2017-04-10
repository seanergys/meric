# README #

--------------------------------------------------------------------------------
#     1] Content of src folder                                                 #
--------------------------------------------------------------------------------
* basis - Input parser
* meric - Base class of the library, that differs in case of use of MPI.
* store - Different types of the store class. To store new type of the data, 
          new class inherited from class StoreBase (store.h) is required.
* wrapper
	- environmentwrapper
		= CPU core frequency and uncore frequency settings.

		+ http://www.thinkwiki.org/wiki/How_to_use_cpufrequtils

		+ https://doc.zih.tu-dresden.de/hpc-wiki/bin/view/Compendium/X86Adapt

	- hdeemwrapper
		= Energy measurement using hdeem.

		+ https://doc.zih.tu-dresden.de/hpc-wiki/bin/view/Compendium/EnergyMeasurement

	- perfeventwrapper
		= Hardware counters provided by perf_event.

		+ http://man7.org/linux/man-pages/man2/perf_event_open.2.html
	
	- papiwrapper
		= Hardware counters provided by papi.
		- NOT SUITABLE FOR THREADED APPLICATIONS!
		
		+ http://icl.cs.utk.edu/papi/
		
	- raplwrapper
		= Intel RAPL counters read by x86_adapt

--------------------------------------------------------------------------------
#     2] Content of test folder                                                #
--------------------------------------------------------------------------------
* source codes
	* test.cpp
		* One region, with two another regions inside.
		* This test has shared Score-P/MERIC api. See section 10 of this README.
	* test_mpi.cpp
		* Same test as test.c, only extended with mpi.
	* blas_test.cpp
		* Test compares DGEMM and DGEMV. There are two possible sizes of matrices (large and small (not) to fit in L3 cache). In both cases, sizes of matrices were set to take approximately the same time for both DGEMM and DGEMV region, when using all resources.
		* WARNING: This test requires mkl module. When make is called, module is loaded.
	* samples_test.cpp
		* Test with sleep (minimum energy consumption) and compute (much higher energy consumption) regions in a loop. This allows user to see in the list of samples how energy rise when compute region starts and check if Meric detects this sample as a first one of this region.
		* Follow the instructions inside to set the test.
* config direcory
	* region.options
		* File that sets exact settings for regions inside your code.
		* In default it is set for blas test.
	* region.options.extra
		* Configuration file, that shows what all possible ways how to specify the settings.
* Makefile
* run.sh and run-mpi.sh
	* Scripts that runs the code with all possible settings.
	* Ready to do the basic test with test and test_mpi.cpp on Taurus.
* environment_taurus
	* Basic script that sets chosen MERIC environment variables and informs you about values of set varibles
	* When runed with argument -t, just prints list of set variables
	* Make a copy of this script and edit it to suit your needs!

--------------------------------------------------------------------------------
#    3] Compile and run basic example                                          #
--------------------------------------------------------------------------------
	# in the interactive job load necessary modules
		module load intel/2016.1.150
		module load mpirt/5.1.2.150
		module load hdeem/2.2.20ms
		module load papi/5.4.1

	# compile MERIC
		export LD_LIBRARY_PATH+=:$PATH_TO_MERIC/lib:/usr/local/lib
		make
	
	# compile test code
		cd test
		make

	# set the default frequencies outside of the regions, or inside regions where are not set
		export MERIC_FREQUENCY=25
		export MERIC_UNCORE_FREQUENCY=25

	# run the example
		./test

To understand this example, explore the source file **test.cpp** to see that this test contains regions A, B and C (inside A are two regions B and one C). For each region you can set cpu core, uncore frequency and number of threads. How to do so, you will find in section MERIC input parameters.

--------------------------------------------------------------------------------
#     4] Measurement results                                                   #
--------------------------------------------------------------------------------
	# CODE OUTPUT:
	* hdeemMeasurement
	 	# contains result files in folders with names of the regions
	 	# to change output folder name export MERIC_OUTPUT_DIR="NEW_NAME"
		# 3 types of data in CSV files:
			# CALLTREE - the first line of every CSV file, it's a call stack, so we can see, where is the measured region nested
			# Section label (e.g. '# Job info') - determines a "category" of following data in a file7
			# Data - tuples (mostly pairs) structured like a hash map: key, value
	* hdeemMeasurementCounters
		# Program start counters, strored in .tmp files.
	
	# EXAMPLE of CSV file contained in hdeemMeasurement folder:
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
	
	# To process the results use RADAR tool
	# Repository URL: https://code.it4i.cz/bes0030/readex-radar.git
		1) Set variables in config.py file (description is included in the file)
		2) Launch python3 ./printFullReport.py	

--------------------------------------------------------------------------------
#      5] Code dynamism investigation                                          #
--------------------------------------------------------------------------------
To find the best settings for each region, you should run your code with all possible settings. To do so, there is a script run.sh in the test folder. In default the script is set to run a segment of all settings for test_mpi.cpp and visualise the result in the RADAR tool.
	
	# Erase previous results to avoid mixing them
		make cleanAll
	
	# Run the script
		sbatch run.sh 
	
	# Provide correct configuration file to RADAR
	# Check inside the cofiguration file if the path to your results is correct
		mv config.py.TEST config.py
	
	# Run RADAR tool
		./printFullReport.py
	
	# cd to directory with measurement results
	# Compile output LaTeX source file and investigate the results
		pdflatex --shell-escape results.tex
	
--------------------------------------------------------------------------------
#     6] MERIC input parameters                                                #
--------------------------------------------------------------------------------

## SET MERIC DEFAULT PARAMETERS ##
	export MERIC_FREQUENCY=24        # [0.1 GHz]
	export MERIC_UNCORE_FREQUENCY=24 # [0.1 GHz]
	export MERIC_NUM_THREADS=24

## SET MERIC WORKING MODE ##
	export MERIC_MODE=0
		0 = hdeem - uses hdeem to measure energy consumtion
		1 = rapl - uses rapl counters to measure energy consumtion
		2 = hdeem & rapl - uses hdeem and rapl at the same time
		3 = none - doesn't measure energy consumtion, but provides you the option to set parameters for regions

## SET ONE OF MERIC OUTPUT FORMAT ##
	export MERIC_CONTINUAL=1
		- Single samples are stored in HDEEM internal memory and read at the end of the runtime
		  (with frequency 1000 samples per 1 second for a blade and 100 samples in detailed mode for VRs)
		- Minimal overhead - only times of the beginning and the end of measurement
		  are stored (samples are processed after the measurement)
		- in noncontinual mode (MERIC_CONTINUAL=0) energy consumption measured directly 
		  (with HDEEM internal delay) at each region start and end

	export MERIC_DETAILED=1
		- Hdeem gives us not only data from blade, but data from Voltage Regulators (VR-CPU1,
		  VR-CPU0, VR-DIMMGH, VR-DIMMEF, VR-DIMMCD, VR-DIMMAB) are stored too
		- RAPL returns values for each cpu, but without detailed mode, it is summarized per node

	export MERIC_DEBUG=1
		- Data are taken both from samples and Stats structure, so we can compare them
		- Data are taken from blade and Voltage Regulators too
		- Only for measurement check - there can be larger overhead because of two
		  types of data processing performed simultaneously

	export MERIC_SAMPLES=1
		- When using HDEEM samples to read the energy consumtion, MERIC prints 
		  each sample to the output file, if MERIC_SAMPLES is set
		- Files can become very big when measured regions run for a long time

	export MERIC_AGGREGATE=1
		- ONLY for distributed applications!
		- Aggregates data from all nodes - there are both average values and sums
		included in the output 

	export MERIC_COUNTERS=papi or perfevent
		- If set you can read HW counters using papi or perfevent
		- When using counters, there is not only counter value but also an information 
		  about computional and arithmetic intensity (if possible to measure)
		- To add a counter you want to measure it is necessary to follow 
		  the instructions in wrapper/counters.h.
	
## SET OUTPUT FILES/FOLDERS NAME ##
	export MERIC_OUTPUT_DIR="hdeemMeasurement"
	export MERIC_OUTPUT_FILENAME="log"

## SET MORE SETTINGS ##
Settings throught the exprorted envoronment variable should fullfil your need when manualy searching the optimal settings. To set more complex settings the configuration file must be define.
In the configuration file one can specify settings for each region separately, different settings can be applied for each node and also socket. It is also possible to provide list of regions to ignore (the settings for these regions are not applied and no consumptions are measured), and size of change in settings, that should be ignored, beacuse it is too small to apply.
	
To use the extended options, configuration file must be written in JSON format as follows and exported MERIC_REGION_OPTIONS=/path/to/regionoptions.json.

The basic settings via configuration file is settings for regions. In this case each region has an object with parameters. Parameters names are the same as exported environment variables, but without the prefix "MERIC_".
	
In case of per node or per socket settings specification, the objects of region settings are wrapped in another object. Per node settings starts the JSON object with keyword "@NODE" (as well as per socket settings use "@SOCKET"), where the value is an object, that has as a keys ids of the nodes (or sockets), and the value of this object that specify the settings for each region.

It is also possible to specify the settings for a socket on a specific node, in this case into "@NODE" object insert "@SOCKET" object, that contains required region settings. If the "@NODE" and "@SOCKET" settings are set in separated objects, the settings for a node has higher priority than settings for a socket.
If any region of your code or any parameter is missing, the default setting is set.

	"@SOCKET" : {
		"0" : {
			"A" : {
				"FREQUENCY" : 13,
				"UNCORE_FREQUENCY" : 13
			}
		}
	}
	
To define ignore settings, write a object with the key "@IGNORE", and value is another object, that might contain "@REGIONS" with array of regions' names to ignore, and "@CHANGE" with key object that contains settings with values, that specify how large the change might be to ignore it.

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
#      7] Measure energy consumption of your code at Taurus                    #
--------------------------------------------------------------------------------
## BEFORE WE START ##
If you want to use Meric with a parallel application, keep in mind, that all processes and all running threads 
must call every Meric region, otherwise Meric behavior is undefined. It is not possible to set runtime environment for each process separately, because Meric settings impress whole node and each region settings allows not to follow this restriction. Because of this, each MeasureStart and MeasureStop starts with a MPI and openmp barrier, that guarantee, that the environment settings is set for its region.

If you want to insert Meric regions into code you don't know, instead of Meric probes insert into the code MPI and openMP barriers and make sure, that the code works fine. After that replace barriers with Meric_MeasureStart/Stop.

There is one more restriction in placing probes into the code. In current version Meric does not support recursively nested regions and regions with at least three starts where fist and third calls are at the same level, but second call is at higher level. This case is shown in example:

	MERIC_MeasureStart("X") //region X wraps others, this region is not necessary in thi example
		MERIC_MeasureStart("A")	//first call of region A
		MERIC_MeasureStop()

		MERIC_MeasureStart("B")
			MERIC_MeasureStart("A")	//region A called at higher nested level
			MERIC_MeasureStop()
		MERIC_MeasureStop()

		MERIC_MeasureStart("A")	//next region A call at the same nested level as the first one
			MERIC_MeasureStart("C")	//the problem is here, when region A has another nested region
			MERIC_MeasureStop()	//region C will has defect in call tree
		MERIC_MeasureStop()
	MERIC_MeasureStop()




## COMPILE MERIC ##
	export LD_LIBRARY_PATH+=:$PATH_TO_MERIC/lib:/usr/local/lib
	module load intel/2016.1.150
	module load mpirt/5.1.2.150
	module load hdeem/2.2.20ms
	module load papi/5.4.1
	make

## EDIT YOUR CODE TO INSERT MEASUREMENT REGIONS ##
	# edit main function
		#include <meric.h>
		...
		MPI_Init(&argc, &argv);
		MERIC_Init();
		...
		MERIC_Close();
		MPI_Finalize();

	# edit file with a region
	# insert Start probe at the beginning of the region with a unique name
	# intert Stop probe at the end of the region - no name needed, ends the last started region
	# for the visualisation with Radar tool, there must be one region, that wraps other regions
		#include <meric.h>
		...
		MERIC_MeasureStart("A");
		//region A
		MERIC_MeasureStop();
	# please, do not use **#** and **@** in names of regions

## COMPILE YOUR CODE ##
	# edit Makefile
		-I$(PATH_TO_MERIC)/include
		-L$(PATH_TO_MERIC)/lib
		-L../hdeem -lhdeem -lfreeipmi -lcpufreq -lx86_adapt -L/sw/taurus/libraries/papi/5.4.1/lib/ -lpapi
		-lmeric OR FOR MPI APPS -lmericmpi 

	# of if application is compiled using waf (such as ESPRESO Solver)
		# * build.config
			INCLUDES = $PATH_TO_MERIC/include
			LIBPATH =  $PATH_TO_MERIC/lib /sw/taurus/libraries/papi/5.4.1/lib/
		# * src/app/wscript
			def configure(ctx):
			    ctx.env.append_unique("LIB", "meric")  # OR "mericmpi"
			    ctx.env.append_unique("LIB", "hdeem")
			    ctx.env.append_unique("LIB", "freeipmi")
			    ctx.env.append_unique("LIB", "cpufreq")
			    ctx.env.append_unique("LIB", "x86_adapt")
			    ctx.env.append_unique("LIB", "papi")

	# compile and run your code as usual
		make
		make run

## TEST YOUR CODE ##
	# edit run.sh and run it
		sbatch run.sh
	
	# edit Radar config.py - follow the instructions inside
	# GENERATE RADAR OUTPUT 
		module load python
		./printFullReport.py

	# compile LaTeX source file that is inside the folder with measurement results
		pdflatex --shell-escape results.tex

--------------------------------------------------------------------------------
#     8] MERIC compilation on a machine without the hdeem                      #
--------------------------------------------------------------------------------
	# optionaly install libraries that Meric uses
		# x86_adapt, cpufreq, papi, freeipmi, hdeem
	# set library paths in Meric Makefile
	# add path to Meric in $LD_LIBRARAY_PATH
	# load modules or install libraries that Meric uses
		# compiler
		# optionaly papi, x86_adapt, cpufreq
	# compile Meric in dummy mode
		# all modules works the same as with hdeem, only hdeemwrapper class is replaced with basic access counter
		# if there is a problem with compiler, set --cpp and --mpi options in Makefile
		make dummy
	# remove unused libraries in test/Makefile
	# compile examples in test directory
	# export MERIC_COUNTERS variable to measure real data

--------------------------------------------------------------------------------
#     9] MERIC with a fortran code                                             #
--------------------------------------------------------------------------------
There are fortran module and interface in the include directory. 
When calling MERIC_MeasureStart keep in mind that string parameter must end with //char(0).

	CALL MERIC_MeasureStart("region_name"//char(0))

--------------------------------------------------------------------------------
#     10] Shared Score-P/MERIC api                                             #
--------------------------------------------------------------------------------
Into include folder the header readex.h was added. Instead of Meric functions call you may use following functions, that are shared with Score-P. To use selected library to measure, compile your code with -DUSE_MERIC or -DUSE_SCOREP. It is not possible to use Meric and Score-P simultaneously. As an example of the api use, test.cpp example code has this interface.

## LIST OF FUNCTIONS ##
Used parameters has the same datatype as Score-P functions: struct SCOREP_User_Region* handle, const char* name, uint32_t type.

	READEX_INIT()
		library init
	READEX_CLOSE()
		library finish the run
	READEX_REGION_DEFINE(handle)
		define region handle
	READEX_REGION_START(handle, name, type)
		region start
	READEX_REGION_STOP(handle)
		region stop
	READEX_PHASE_START(handle, name, type)
		phase region start
	READEX_PHASE_STOP(handle)
		phase region stop
	




