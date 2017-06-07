#!/bin/sh
#SBATCH -N 1
#SBATCH --partition=jetson-tx
#SBATCH --tasks-per-node=1
#SBATCH --cpus-per-task=24
#SBATCH -t 30:00

## INTERACTIVE
## srun -t 30:00 -N 1 --tasks-per-node=1 --cpus-per-task=1 --partition=jetson-tx --pty bash

PATH_TO_MERIC=$HOME/meric
export LD_LIBRARY_PATH+=:$PATH_TO_MERIC/lib

module load gcc/6.1.0
module load papi

#start energy measurement
$PATH_TO_MERIC/tools/getJTX1measurements.py &
sleep 5s

#run your code

# please use bellow mentioned frequencies only. These frequencies are in kHz
# list of freqeuncies based on administrators recommendation, see:
	# /sys/devices/system/cpu/cpu0/cpufreq/scaling_available_frequencies
	# /sys/kernel/debug/clock/emc/possible_rates

export MERIC_MODE=4

for core in 102000 204000 307200 403200 518400 614400 710400 825600 921600 1036800 1132800 1224000 1326000
do
	for uncore in 40800 68000 102000 204000 408000 665600 800000 1065600 1331200 1600000
	do
		export MERIC_FREQUENCY=$core
		export MERIC_UNCORE_FREQUENCY=$uncore
		export MERIC_NUM_THREADS=0

		./test

	done
done

#finish energy measurement
pkill -f -INT getJTX1measurements.py
