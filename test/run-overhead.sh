#!/bin/bash
#SBATCH -N 1
#SBATCH --tasks-per-node=1
#SBATCH --cpus-per-task=24
#SBATCH --exclusive
#SBATCH -p haswell
#SBATCH -A p_readex
#SBATCH --mem 62000
#SBATCH -t 0-0:15
#SBATCH --comment="cpufreqchown"
#SBATCH --mail-type=ALL
#SBATCH --mail-user=ondrej.vysocky@vsb.cz

#PBS -q qexp
#PBS -l walltime=00:15:00
#PBS -A OPEN-10-15
#PBS -l select=1
#PBS -m be
#PBS -N MericOverhead
#PBS -l x86_adapt=true
################################################################################

#cd $PATH_TO_MERIC
cd /home/vys0053/readex/meric

. set_env_*
cd test/

export MERIC_FREQUENCY=25
export MERIC_UNCORE_FREQUENCY=30
export MERIC_NUM_THREADS=0
export MERIC_CONTINUAL=1
export MERIC_MODE=3	# 3 = no energy measurement


export MERIC_REGION_OPTIONS=region.options-UnCF
./overHead > TRACK-UnCF.txt
grep "MERIC uncore " TRACK-UnCF.txt | awk 'NR % 2 == 0' | grep -oE "[0-9].[0-9]+" > DOWN_UnCF.txt
grep "MERIC uncore " TRACK-UnCF.txt | awk 'NR % 2 == 1' | grep -oE "[0-9].[0-9]+" > UP_UnCF.txt
grep "MERIC start "  TRACK-UnCF.txt | grep -oE "[0-9].[0-9]+" > MB_UnCF.txt
grep "MERIC stop "   TRACK-UnCF.txt | grep -oE "[0-9].[0-9]+" > MA_UnCF.txt


export MERIC_REGION_OPTIONS=region.options-CF
./overHead > TRACK-CF.txt
grep "MERIC core " TRACK-CF.txt | awk 'NR % 2 == 0' | grep -oE "[0-9].[0-9]+" > DOWN_CF.txt
grep "MERIC core " TRACK-CF.txt | awk 'NR % 2 == 1' | grep -oE "[0-9].[0-9]+" > UP_CF.txt
grep "MERIC start "  TRACK-UnCF.txt | grep -oE "[0-9].[0-9]+" > MB_CF.txt
grep "MERIC stop "   TRACK-UnCF.txt | grep -oE "[0-9].[0-9]+" > MA_CF.txt


