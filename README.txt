# README #

--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
#     Content of src folder                                                    #
--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
* basis - Input parser.
* meric - Base class of the library, that differs in case of use of MPI.
* store - Different types of the store class. To store new type of the data, 
          new class inherited from class StoreBase (store.h) is required.
* wrapper
	- environmentwrapper
		= CPU frequency and uncore frequency settings
	- hdeemwrapper
		= energy measurement using hdeem
	- perfeventwrapper
		= hardware counters provided by perf_event 
		  (http://man7.org/linux/man-pages/man2/perf_event_open.2.html)
		- to add a counter it is necessary to
			1) add counter description to perfeventwrapper.h
			2) in void PERFEVENT::init() call setCounter with corresponding 
			   counter type and configuration
			3) in void PERFEVENT::initMap(std::map<std::string, long> & dict) 
			   add new dictionary element key=CounterDesription value=0 

--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
#     This is a tutorial how to insert measurement regions into your code      #
#     and compile it with necessary libraries                                  #
--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

## SET THIS VARIABLES ACCORDING TO YOUR CASE ##
	FUNCLIST=data/functionListEspreso
	OUTPUT_DIR=~/espresoWithSondas
	INPUT_CODE=~/espreso
	PATH_TO_MERIC=~/meric

## CLONE REPOSITORY ##
	git clone git@code.it4i.cz:mec059/meric.git
	# OR
	git clone git@code.it4i.cz:xvysoc01/meric.git

## CONFIGURE ENVIRONMENT ##
	export LD_LIBRARY_PATH+=:$PATH_TO_MERIC/lib:/usr/local/lib
	. set_env_taurus	# LOAD MODULES FOR YOUR CODE
		# IN CASE OF ESPRESO
		module load intel/2016.1.150
		module load mpirt/5.1.2.150
		module load hdeem/2.2.2

## COMPILE MERIC ##
	./waf clean
	./waf configure --cpp=icpc --flag="-L/projects/p_readex/cpufrequtils-icc 
		-I/projects/p_readex/cpufrequtils-icc/lib/"
	./waf install

## INSERT MEASUREMENT REGIONS ##
	cd /projects/p_readex/amt/readex-repository/FooTools
	. ./env 
	./fooInsert.py -o $OUTPUT_DIR -funcFile $FUNCLIST $INPUT_CODE

## COMPILE YOUR CODE ##
	cd $OUTPUT_DIR

	# EDIT MAIN
		#include <meric.h>
		...
		MPI_Init(&argc, &argv);
		MERIC_Init();
		...
		MERIC_Close();
		MPI_Finalize();

	# EDIT MAKEFILE
		-I$(PATH_TO_MERIC)/include
		-L$(PATH_TO_MERIC)/lib
		-lmeric -lmericmpi -lhdeem -lcpufreq -lx86_adapt
	# OR IN CASE OF ESPRESO
		# * build.config
			INCLUDES = $PATH_TO_MERIC/include
			LIBPATH =  $PATH_TO_MERIC/lib
		# * src/app/wscript
			def configure(ctx):
			    ctx.env.append_unique("LIB", "meric")
			    ctx.env.append_unique("LIB", "mericmpi")
			    ctx.env.append_unique("LIB", "hdeem")
			    ctx.env.append_unique("LIB", "cpufreq")
			    ctx.env.append_unique("LIB", "x86_adapt")
	./waf configure
	./waf install

## SET MERIC PARAMETERS ##
	export MERIC_FREQUNECY=2500000
	export MERIC_UNCORE_FREQUENCY=25

## SET ONE OF MERIC OUTPUT FORMAT
	#0. - no env variable needed
	#1.
		MERIC_CONTINUAL=1
	#2.
		MERIC_DETAILED=1
	#3.
		MERIC_CONTINUAL=1
		MERIC_DETAILED=1
	#4.
		MERIC_DEBUG=1
	#5. - for distributed only
		MERIC_AGGREGATE=1
		MERIC_DEBUG=1
	#6. - for distributed only
		MERIC_AGGREGATE=1
		MERIC_CONTINUAL=1
		MERIC_DETAILED=1
	#7. - for distributed only
		MERIC_AGGREGATE=1
		MERIC_CONTINUAL=1
	#8. - for distributed only
		MERIC_AGGREGATE=1
		MERIC_DETAILED=1
	#9. - for distributed only
		MERIC_AGGREGATE=1
	#10.- under development
		MERIC_COUNTERS=1

## TO SET FREQUENCY AND UNCORE_FREQUENCY FOR EACH REGION WRITE A FILE ##
## **region.options** WITH VALUES FOR EACH REGION ##
	# EXAMPLE FOR REGIONS A and B
	A {
	  FREQUENCY = 1500000,
	  UNCORE_FREQUENCY = 15
	}

	B { 
	  FREQUENCY = 2300000, 
	  UNCORE_FREQUENCY = 23
	}

## READY TO USE ##
	# run your code as usual
	# CODE OUTPUT:
	 hdeemMeasurement
	 	# contains result files in folders with names of the regions
	 	# to change output folder name export MERIC_OUTPUT_DIR="NEW_NAME"
	 startcounters_log.tmp
		# initial counter value


