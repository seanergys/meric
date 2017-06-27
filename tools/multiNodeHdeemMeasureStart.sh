#!/bin/bash

# get list of nodes to run the script
	# squeue -u $USER
# copy the list of nodes and run tsis script using pdsh
	# pdsh -w taurusi6605,taurusi6606 $HOME/meric/tools/script.sh
# run your application afterwards
# when it is finished, run the stop script
file=$HOME/meric/tools/$(hostname)_measurement
hostname >> $file

clearHdeem
startHdeem

