#!/bin/bash

# SET THIS SECTION #############################################################
MERIC_OUTPUT_DIR=MEASUREMENT #name it as you wish
MAIN_REGION=Main #name it as you wish
NUM_PROCS=$[ 32*24 ]
APP=simpleFoam -parallel
PATH_TO_MERIC=/scratch/work/user/vys0053/SOFTWARE/tmp/meric

# LOAD MODULES #################################################################
module load x86_adapt
export LD_LIBRARY_PATH+=:/usr/local/lib

# RUN THE CODE FOR EACH SETTINGS ###############################################
mkdir -p $MERIC_OUTPUT_DIR
mkdir -p $MERIC_OUTPUT_DIR"Counters"
mkdir -p $MERIC_OUTPUT_DIR/$MAIN_REGION
echo "RUNNING"

#WARM-UP
mpirun -n $NUM_PROCS $APP

# RUN THE TEST
for cpu_freq in 25 {24..12..2}
do
	for uncore_freq in {30..12..2}
	do

		MERIC_OUTPUT_FILENAME=$cpu_freq"_"$uncore_freq".csv"
		echo
		echo "TEST output:" $MERIC_OUTPUT_FILENAME 

		for loop in {0..19}
		do
			echo "# CALLTREE;init_$loop" >> $MERIC_OUTPUT_DIR/$MAIN_REGION/$MERIC_OUTPUT_FILENAME
			echo "# Job info - rapl" >> $MERIC_OUTPUT_DIR/$MAIN_REGION/$MERIC_OUTPUT_FILENAME
			$PATH_TO_MERIC/tools/staticMERICtool/multiNodeStaticMeasureStart.sh --rapl -c $cpu_freq -u $uncore_freq
			mpirun -n $NUM_PROCS $APP > log.txt
			result=$($PATH_TO_MERIC/tools/staticMERICtool/multiNodeStaticMeasureStop.sh --rapl)
			echo "${result//: /,}"  >> $MERIC_OUTPUT_DIR/$MAIN_REGION/$MERIC_OUTPUT_FILENAME
		done
		echo $loop > $MERIC_OUTPUT_DIR"Counters"/"startCounter_"$cpu_freq"_"$uncore_freq".tmp"
	done
done



