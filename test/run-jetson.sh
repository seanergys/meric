#!/bin/sh
#SBATCH -N 1
#SBATCH --partition=jetson-tx
#SBATCH --tasks-per-node=4
#SBATCH --cpus-per-task=1
#SBATCH -t 30:00

## INTERACTIVE
## srun -t 30:00 -N 1 --tasks-per-node=1 --cpus-per-task=1 --partition=jetson-tx --pty bash

PATH_TO_MERIC=$HOME/meric
export LD_LIBRARY_PATH+=:$PATH_TO_MERIC/lib
hostname

#module load gcc/6.1.0
#module load papi

# set test settings
export MERIC_MODE=4

export MERIC_FREQUENCY=1326000
export MERIC_UNCORE_FREQUENCY=1600000
export MERIC_NUM_THREADS=0

# one test run
mkdir delete
export MERIC_OUTPUT_DIR=delete
mpirun -n 4 ./test_mpi



# run the test
# please use bellow mentioned frequencies only. These frequencies are in kHz
# list of freqeuncies based on administrators recommendation, see:
	# /sys/devices/system/cpu/cpu0/cpufreq/scaling_available_frequencies
	# /sys/kernel/debug/clock/emc/possible_rates

export MERIC_OUTPUT_DIR=jetsonMeasurement

for core in 102000 204000 307200 403200 518400 614400 710400 825600 921600 1036800 1132800 1224000 1326000
do
	for uncore in 40800 68000 102000 204000 408000 665600 800000 1065600 1331200 1600000
	do
		export MERIC_FREQUENCY=$core
		export MERIC_UNCORE_FREQUENCY=$uncore
		#export MERIC_NUM_THREADS=0

		export MERIC_OUTPUT_FILENAME=$MERIC_FREQUENCY"_"$MERIC_UNCORE_FREQUENCY

		mpirun -n 4./test_mpi
		if [ $? -ne 0 ]
		then
			mpirun -n 4./test_mpi
		do
	done
done

