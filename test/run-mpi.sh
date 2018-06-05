#!/bin/bash
# #SBATCH -w taurusi4168 # RUN ONE TEST AT THE SAME LIST OF NODES
#SBATCH -N 1
#SBATCH --exclusive
#SBATCH -p haswell
#SBATCH -A p_readex	# project name
#SBATCH --mem 62000	
#SBATCH -t 0-1:00
#SBATCH --comment="cpufreqchown"
#SBATCH --cpu-freq=Low

	## openMP	
##SBATCH --tasks-per-node=1
##SBATCH --cpus-per-task=24

	## MPI
#SBATCH --tasks-per-node=24
#SBATCH --cpus-per-task=1

# sbatch run.sh
# squeue -u $USER
################################################################################
hostname	#get node name

# LOAD MODULES #################################################################
module purge
module load intel
module load mpirt
module load papi
module load cpufreq

# RUN FIRST TEST TO AVOID HDEEM ERROR ##########################################
# to prevent HDEEM error use static measurement tool to reset HDEEM
#../tools/staticMERICtool/multiNodeStaticMeasureStart.sh --hdeem
export MERIC_FREQUENCY=25
export MERIC_UNCORE_FREQUENCY=25
export MERIC_OUTPUT_DIR="DELETEME"
mpirun -n 24 ./test_mpi

# SET MERIC OUTPUT #############################################################
export MERIC_OUTPUT_DIR="TEST"
export MERIC_COUNTERS=papi
export MERIC_MODE=2
export MERIC_DETAILED=1
export MERIC_CONTINUAL=1

rm -rf $MERIC_OUTPUT_DIR*

# RUN THE CODE FOR EACH SETTINGS ###############################################
# export KMP_AFFINITY=compact
# export KMP_AFFINITY="granularity=core,scatter"

echo "RUNNING"
for proc in 24
do
	for thread in 1
	do
		for cpu_freq in {25..12..1}
		do
			for uncore_freq in {30..12..1}
			do
				# OUTPUT FILE NAME
				export MERIC_OUTPUT_FILENAME=$thread"_"$cpu_freq"_"$uncore_freq

				# TEST SETTINGS
				export MERIC_NUM_THREADS=$thread
				export MERIC_FREQUENCY=$cpu_freq
				export MERIC_UNCORE_FREQUENCY=$uncore_freq

				# RUN THE TEST
				echo
				echo "TEST output:" $MERIC_OUTPUT_FILENAME 
				mpirun -n $proc ./test_mpi
			done
		done
	done
done


