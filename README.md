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
	* test_mpi.cpp
		* Same test as test.c, only extended with mpi.
	* blas_test.cpp
		* Test compares DGEMM and DGEMV. There are two possible sizes of matrices (large and small (not) to fit in L3 cache). In both cases, sizes of matrices were set to take approximately the same time for both DGEMM and DGEMV region, when using all resources.
		* WARNING: This test requires mkl module. When make is called, module is loaded.
* region.options
	* File that sets exact settings for regions inside your code.
	* In default it is set for blas test.
* Makefile
* run.sh
	* Script that runs the code with all possible settings.
	* Ready to do the basic test with test_mpi.cpp on Taurus.
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

## TO SET FREQUENCY AND UNCORE_FREQUENCY FOR EACH REGION ##
	If you find a settings that you want to apply for your regions, you can write 
	a file **region.options**, where you set them. This file must be in JSON format,
	where each region has an object with parameters. Parametrs names are the same 
	as exported environment variables, but witout the prefix "MERIC_".
	
	If any region of your code or any parameter is missing, than this missing 
	parameter will be set to the same value, as it was before the region started.
	
	To run your code with a region settings, set name of the file into environment variable MERIC_REGION_OPTIONS.
	
	# EXAMPLE OF **region.options** FILE FOR REGIONS **A** AND **B**
	{
	"A" : {
	  "FREQUENCY" : 15,
	  "UNCORE_FREQUENCY" : 15,
	  "NUM_THREADS" : 15
	}
	,
	"B":{ 
	  "FREQUENCY": 23,
	  "UNCORE_FREQUENCY" : 23
	}
	}

## SET ONE OF MERIC OUTPUT FORMAT ##
	export MERIC_CONTINUAL=1
		- Energy consumption is computed from single samples taken by HDEEM (when 
		  MERIC_CONTINUAL=0, then data are taken from Stats structure)
		- Single samples are stored (with frequency 1 samples per 1 ms)
		- Minimal overhead - only times of the beginning and the end of measurement
		  are stored (samples are processed after the measurement)
		- Files can become very big when measured regions run for a long time!

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
		- when using HDEEM samples to read the energy consumtion, MERIC prints 
		  each sample to the output file, if MERIC_SAMPLES is set

	export MERIC_AGGREGATE=1
		- ONLY for distributed applications!
		- Aggregates data from all nodes - there are both average values and sums
		included in the output 

	export MERIC_COUNTERS=papi or perfevent
		- If set you can read HW counters using papi or perfevent
		- When using counters, there is not only counter value but also an information about flop/s (if possible to measure)
		- To add a counter you want to measure it is necessary to follow 
		  the instructions in wrapper/counters.h.
	
## SET OUTPUT FILES/FOLDERS NAME ##
	export MERIC_OUTPUT_DIR="hdeemMeasurement"
	export MERIC_OUTPUT_FILENAME="log"

--------------------------------------------------------------------------------
#      7] Measure energy consumption of your code at Taurus                    #
--------------------------------------------------------------------------------
## BEFORE WE START ##
If you want to use Meric with a parallel application, keep in mind, that all Meric regions must call all processes and all running threads, otherwise Meric behavior is undefined. It is not possible to set runtime environment for each process separately, because Meric settings impress whole node and each region settings allows not to follow this restriction. Because of this, each MeasureStart and MeasureStop starts with a MPI and openmp barrier, that guarantee, that the environment settings is set for its region.

If you want to insert Meric regions into code you don't know, instead of Meric probes insert into the code MPI and openMP barriers and make sure, that the code works fine. After that replace barriers with Meric_MeasureStart/Stop.

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
	# install freeipmi
		# https://www.gnu.org/software/freeipmi/download.html
	# optionaly install other libraries
		# x86_adapt, cpufreq, papi
	# set library paths in Meric Makefile
	# add path to Meric in $LD_LIBRARAY_PATH
	# load modules
		# intel compiler
		# python
		# papi, x86_adapt or cpufreq if available
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

