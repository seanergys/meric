#!/bin/bash
#SBATCH -N 2
#SBATCH --exclusive
#SBATCH --ntasks-per-node=1
#SBATCH -p haswell
#SBATCH --mem 62000
#SBATCH -t 0-1:00
# #SBATCH --mail-type=ALL
# #SBATCH --mail-user=ondrej.vysocky@vsb.cz

# sbatch run.sh
# squeue -u $USER
###############################################################################

# LOAD MODULES
module purge
module load intel/2016.1.150
module load mpirt/5.1.2.150
module load hdeem/2.2.2
module load papi/5.4.1

# COMPILE
make

# SET MERIC OUTPUT FORMAT
export MERIC_DEBUG=1

# FOR EACH SETTINGS
for proc in 2
do
	for thread in 1  #{1..24..1}
	do
		for cpu_freq in 12 14 # {12..25..1}
		do
			for uncore_freq in 28 30 #{12..30..1}
			do
				# OUTPUT NAMES
				export MERIC_OUTPUT_FILENAME=$thread"_"$cpu_freq"_"$uncore_freq
				export MERIC_OUTPUT_DIR="TESTMPI"

				# TEST SETTINGS
				export MERIC_NUM_THREADS=1
				export MERIC_FREQUENCY=$cpu_freq
				export MERIC_UNCORE_FREQUENCY=$uncore_freq
				#export MERIC_COUNTERS=perfevent

				# RUN THE TEST
				echo
				echo TEST $MERIC_OUTPUT_FILENAME
				mpirun -n 2 ./test_mpi

			done
		done
	done
done


