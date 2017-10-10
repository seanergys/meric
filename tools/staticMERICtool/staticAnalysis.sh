#!/bin/bash


# loops for setting different configuration
# CPU core and uncore frequencies
# number of OpenMP threads, threads placement
# application parameters

for UNCORE in {30..12..2}
do
	for CORE in 25 {24..12..2}
	do
		./multiNodeStaticMeasurementStart.sh --rapl -c $CORE -u $UNCORE
		
		sleep 5 # << run your application instead
		
		ENERGY=$(./multiNodeStaticMeasurementStop.sh --rapl -c 25 -u 30) # default frequencies
		
		echo -e "Configuration\n" \
				"\t core freq:" $CORE "\n"\
				"\t uncore freq:" $UNCORE "\n"\
				"Consumed energy:" $ENERGY "\n"
	done
done
