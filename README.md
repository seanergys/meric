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
		- To add a counter it is necessary to follow the instructions in wrapper/counters.h.

		+ http://man7.org/linux/man-pages/man2/perf_event_open.2.html
	
	- papiwrapper
		= Hardware counters provided by papi.
		- To add a counter it is necessary to follow the instructions in wrapper/counters.h.
		
		+ http://icl.cs.utk.edu/papi/


--------------------------------------------------------------------------------
#    2] Compile and run basic example                                          #
--------------------------------------------------------------------------------
	# load necessary modules
	module load intel/2016.1.150
	module load mpirt/5.1.2.150
	module load hdeem/2.2.2
	module load papi/5.4.1

	# compile MERIC
	export LD_LIBRARY_PATH+=:$PATH_TO_MERIC/lib:/usr/local/lib
	make
	
	# compile test code
	cd test
	make

	# set the default frequencies outside of the regions, or inside regions where are not set
	export MERIC_FREQUENCY=2500000
	export MERIC_UNCORE_FREQUENCY=25

	# run the example
	./test

To understand this example, explore the source file *shared.cpp* to see that this test contains regions A and B (inside A are two regions B). For each region you can set cpu core and uncore frequency. To do so, edit *region.options* file. To use default settings for the region, skip the region settings you don't want, or just rename this file.

--------------------------------------------------------------------------------
#     3] Measurement results                                                   #
--------------------------------------------------------------------------------
	# CODE OUTPUT:
	* hdeemMeasurement
	 	# contains result files in folders with names of the regions
	 	# to change output folder name export MERIC_OUTPUT_DIR="NEW_NAME"
		# 3 types of data in CSV files:
			# CALLTREE - the first line of every CSV file, it's a call stack, so we can see, where is the measured region nested
			# Section label (e.g. '# Job info') - determines a "category" of following data in a file7
			# Data - tuples (mostly pairs) structured like a hash map: key, value
	* startcounters_log.tmp
		# Program start counter
	
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
	# Repo URL: https://code.it4i.cz/bes0030/readex-radar.git
	1) Set variables in config.py file (description is included in the file)
	2) Launch python3 ./printFullReport.py	
		
--------------------------------------------------------------------------------
#     4] MERIC input parameters                                                #
--------------------------------------------------------------------------------

## SET MERIC DEFAULT PARAMETERS ##
	export MERIC_FREQUNECY=2500000
	export MERIC_UNCORE_FREQUENCY=25

## TO SET FREQUENCY AND UNCORE_FREQUENCY FOR EACH REGION WRITE A FILE **region.options** WITH VALUES FOR EACH REGION ##
	# EXAMPLE FOR REGIONS A and B
	A {
	  FREQUENCY = 1500000,
	  UNCORE_FREQUENCY = 15
	}

	B { 
	  FREQUENCY = 2300000, 
	  UNCORE_FREQUENCY = 23
	}

## SET ONE OF MERIC OUTPUT FORMAT ##
	MERIC_CONTINUAL=1
		- Energy consumption is computed from single samples taken by HDEEM (when 
		  MERIC_CONTINUAL=0, then data are taken from Stats structure)
		- Single samples are stored (with frequency 1 samples per 1ms)
		- Minimal overhead - only times of the beginning and the end of measurement
		  are stored (samples are processed after the measurement)
		- Files can become very big when measured regions run for long time!

	MERIC_DETAILED=1
		- Not only data from blade, but data from Voltage Regulators (VR-CPU1,
		  VR-CPU0, VR-DIMMGH, VR-DIMMEF, VR-DIMMCD, VR-DIMMAB) are stored too

	MERIC_DEBUG=1
		- Data are taken both from samples and Stats structure, so we can compare them
		- Data are taken from blade and Voltage Regulators too
		- Only for measurement check - there can be larger overhead because of two
		  types of data processing performed simultaneously

	MERIC_AGGREGATE=1
		- ONLY for distributed applications!
		- Aggregates data from all nodes - there are both average values and sums
		  included in the output 

	MERIC_COUNTERS=1
		- In development
	
## SET OUTPUT FILES/FOLDERS NAME ##
	export MERIC_OUTPUT_DIR="hdeemMeasurement"
	export MERIC_OUTPUT_FILENAME="log"

--------------------------------------------------------------------------------
#      5] Measure energy consumption of your code                              #
--------------------------------------------------------------------------------

## COMPILE MERIC ##
	export LD_LIBRARY_PATH+=:$PATH_TO_MERIC/lib:/usr/local/lib
	module load intel/2016.1.150
	module load mpirt/5.1.2.150
	module load hdeem/2.2.2
	module load papi/5.4.1
	make

## EDIT YOUR CODE TO INSERT MEASUREMENT REGIONS ##
	# EDIT MAIN
		#include <meric.h>
		...
		MPI_Init(&argc, &argv);
		MERIC_Init();
		...
		MERIC_Close();
		MPI_Finalize();

	# EDIT FILE WITH A REGION 
	# INSERT START PROBE AT THE BEGINNING OF THE REGION WITH A UNIQUE NAME
	# INSERT STOP PROBE AT THE END OF THE REGION - NO NAME NEEDED, ENDS THE LAST STARTED REGION
		#include <meric.h>
		...
		MERIC_MeasureStart("A");
		//region A
		MERIC_MeasureStop();


## COMPILE YOUR CODE ##
	# EDIT MAKEFILE
		-I$(PATH_TO_MERIC)/include
		-L$(PATH_TO_MERIC)/lib
		-lmeric -lmericmpi -L../hdeem -lhdeem -lcpufreq -lx86_adapt -L/sw/taurus/libraries/papi/5.4.1/lib/ -lpapi

	# OR IF APPLICATION IS COMPILED USING WAF (such as ESPRESO Solver)
		# * build.config
			INCLUDES = $PATH_TO_MERIC/include
			LIBPATH =  $PATH_TO_MERIC/lib /sw/taurus/libraries/papi/5.4.1/lib/
		# * src/app/wscript
			def configure(ctx):
			    ctx.env.append_unique("LIB", "meric")
			    ctx.env.append_unique("LIB", "mericmpi")
			    ctx.env.append_unique("LIB", "hdeem")
			    ctx.env.append_unique("LIB", "cpufreq")
			    ctx.env.append_unique("LIB", "x86_adapt")
			    ctx.env.append_unique("LIB", "papi")

	# COMPILE AND RUN YOUR CODE AS USUAL
	make
	make run

