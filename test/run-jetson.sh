#!/bin/sh
#SBATCH -N 1
#SBATCH --partition=jetson-tx
#SBATCH --tasks-per-node=1
#SBATCH --cpus-per-task=24
#SBATCH -t 5:00

## INTERACTIVE
## srun -t 5:00 -N 1 --tasks-per-node=1 --cpus-per-task=1 --partition=jetson-tx --pty bash

PATH_TO_MERIC=$HOME/meric
export LD_LIBRARY_PATH+=:$PATH_TO_MERIC/lib

module load gcc/6.1.0
module load papi

#start energy measurement
$PATH_TO_MERIC/tools/getJTX1measurements.py &
sleep 5s

#run your code
./test

#finish energy measurement
pkill -f -INT getJTX1measurements.py
