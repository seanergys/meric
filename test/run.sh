#!/bin/bash
#SBATCH -N 1
#SBATCH --exclusive
#SBATCH -p haswell
#SBATCH -A p_readex	#to account your compute time on the readex project
#SBATCH --mem 62000	#to the memory available on the node. Some nodes have more memory. Please see the documentation.
	##SBATCH --mem=126000
#SBATCH -t 0-1:00
#SBATCH --comment="cpufreqchown"
#SBATCH --cpu-freq=Low
	##SBATCH --mail-type=ALL
	##SBATCH --mail-user=ondrej.vysocky@vsb.cz

	## openMP	
#SBATCH --tasks-per-node=1
#SBATCH --cpus-per-task=24

	## MPI
##SBATCH --tasks-per-node=24
##SBATCH --cpus-per-task=1

# sbatch run.sh
# squeue -u $USER
################################################################################

# LOAD MODULES #################################################################
module purge
module load intel/2016.1.150
module load mpirt/5.1.2.150
module load hdeem/2.2.20ms
module load papi/5.4.1

# COMPILE ######################################################################
make

# RUN FIRST TEST TO AVOID HDEEM ERROR ##########################################
export MERIC_FREQUENCY=25
export MERIC_UNCORE_FREQUENCY=25
mpirun -n 24 ./test_mpi
make cleanAll
make

# SET MERIC OUTPUT #############################################################
export MERIC_OUTPUT_DIR="TESTMPI"
export MERIC_COUNTERS=perfevent
export MERIC_MODE=2
# export MERIC_DEBUG=1
export MERIC_AGGREGATE=0
export MERIC_DETAILED=1
export MERIC_CONTINUAL=1

# RUN THE CODE FOR EACH SETTINGS ###############################################
echo "RUNNING"
for proc in 24
do
	for thread in 1  #{24..1..1}
	do
		for cpu_freq in 16 14 12 # {25..12..1}
		do
			for uncore_freq in 30 28 26 #{30..12..1}
			do
				# OUTPUT FILE NAME
				export MERIC_OUTPUT_FILENAME=$thread"_"$cpu_freq"_"$uncore_freq

				# TEST SETTINGS
				export MERIC_NUM_THREADS=$thread
				export MERIC_FREQUENCY=$cpu_freq
				export MERIC_UNCORE_FREQUENCY=$uncore_freq

				# RUN THE TEST
				echo
				echo TEST $MERIC_OUTPUT_FILENAME 
				mpirun -n $proc ./test_mpi
				mpirun -n $proc ./test_mpi

			done
		done
	done
done


