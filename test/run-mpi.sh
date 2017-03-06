#!/bin/bash
# #SBATCH -w taurusi4168 # RUN ONE TEST AT THE SAME NODES
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
module load intel/2016.1.150
module load mpirt/5.1.2.150
module load hdeem/2.2.20ms
module load papi/5.4.1

# COMPILE ######################################################################
make

# RUN FIRST TEST TO AVOID HDEEM ERROR ##########################################
export MERIC_FREQUENCY=25
export MERIC_UNCORE_FREQUENCY=25
export MERIC_OUTPUT_DIR="DELETEME"
mpirun -n 24 ./test_mpi

# SET MERIC OUTPUT #############################################################
export MERIC_OUTPUT_DIR="TEST"
export MERIC_COUNTERS=perfevent
export MERIC_MODE=2
export MERIC_DETAILED=1
export MERIC_CONTINUAL=1
#export MERIC_AGGREGATE=0


# REMOVE LAST RESULT FROM THE PREVIOUS RUN #####################################
# useful if test did not finished and you want to continue where it ended
# do not use this part if you want more tests with the same settings, as well as IF in RUN section of this script (marked with <<<)
if [ "$( ls $MERIC_OUTPUT_DIR'Counters/' )" ]
then
	LAST=$( ls -t $MERIC_OUTPUT_DIR"Counters/startCounter_"* | head -1 | sed 's/[^_]*_\(.*\).tmp/\1/' )
	rm $MERIC_OUTPUT_DIR"Counters/startCounter_"$LAST.tmp
	for dir in $MERIC_OUTPUT_DIR/*
	do
		rm -f $dir/$LAST.csv
	done
fi

# UNSET SLURM MPI PINNING ######################################################
# works for mpirun, check the behavior using htop
unset I_MPI_PMI_LIBRARY
export SLURM_CPU_BIND=none

# RUN THE CODE FOR EACH SETTINGS ###############################################
# export KMP_AFFINITY=compact
# export KMP_AFFINITY="granularity=core,scatter"

echo "RUNNING"
for proc in 24
do
	for thread in 1
	do
		for cpu_freq in {25..12..1} # or 25 {24..12..2} or {33..12..2}
		do
			for uncore_freq in {30..12..1} # or {30..12..2}
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

					# DO NOT USE THIS if, if you want more test with same settings <<<
				if [ ! -f $MERIC_OUTPUT_DIR"Counters/startCounter_"$MERIC_OUTPUT_FILENAME".tmp" ]
				then
					mpirun -n $proc ./test_mpi
				else
					echo $MERIC_OUTPUT_FILENAME skipped
				fi
			done
		done
	done
done


