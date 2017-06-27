#!/bin/bash

# get list of nodes to run the script
	# squeue -u $USER
# copy the list of nodes and run this script using pdsh
	# pdsh -w taurusi6605,taurusi6606 $HOME/meric/tools/script.sh

file=$HOME/meric/tools/$(hostname)_measurement
hostname >> $file

stopHdeem
checkHdeem >> $file
