#!/bin/bash

# SET THIS SECTION #############################################################
module purge
source readex_env/set_env_meric.source
source environment.sh

NUM_PROCS=$[ 32*24 ]
APP=simpleFoam -parallel

# MERIC SETTINGS ###############################################################
export MERIC_MODE=3
export MERIC_FREQUENCY=0
export MERIC_UNCORE_FREQUENCY=0
export MERIC_NUM_THREADS=0

# SCRIPT-SPECIFIC PART #########################################################
Evaluate (){
	TIME=$(grep -oP "(?<=Runtime \[s\] = )\d+\.\d+"  <<< $OUTPUT | head -1)
	SUMTIME=$( awk "BEGIN {print $SUMTIME + $TIME}" )
	CPUs=$(grep -oP "(?<=Overall CPUs energy consumption [\[J\] = )\d+"  <<< $OUTPUT )
	NODEs=$(grep -oP "(?<=Overall nodes energy consumption \[J\] = )\d+"  <<< $OUTPUT )
	
	for VAL in $CPUs
	do
		SUMCPUs=$( awk "BEGIN {print $SUMCPUs + $VAL}" )
	done
	for VAL in $NODEs
	do
		SUMNODEs=$( awk "BEGIN {print $SUMNODEs + $VAL}" )
	done
}

# RUN PLAIN APP ################################################################
SUMTIME=0.0
SUMCPUs=0.0
SUMNODEs=0.0

#WARM-UP
mpirun -n $NUM_PROCS $APP

REPEAT_COUNT=5
while [ $i -le $REPEAT_COUNT ]; do
	$MERIC_ROOT/tools/staticMERICtool/multiNodeStaticMeasureStart.sh --rapl
	mpirun -n $NUM_PROCS $APP > log.txt
	$MERIC_ROOT/tools/staticMERICtool/multiNodeStaticMeasureStop.sh --rapl
	Evaluate
	i=$(echo "$i + 1" | bc)
done

echo "PLAIN time:" $( awk "BEGIN {print $SUMTIME / $REPEAT_COUNT}" )
echo "PLAIN CPUs energy:" $( awk "BEGIN {print $SUMCPUs / $REPEAT_COUNT}" )
echo "PLAIN NODEs energy:" $( awk "BEGIN {print $SUMNODEs / $REPEAT_COUNT}" )


# RUN TUNNED APP ###############################################################
SUMTIME=0.0
SUMCPUs=0.0
SUMNODEs=0.0

export MERIC_REGION_OPTIONS=region.opts

while [ $i -le $REPEAT_COUNT ]; do
	$MERIC_ROOT/tools/staticMERICtool/multiNodeStaticMeasureStart.sh --rapl
	mpirun -n $NUM_PROCS $APP > log.txt
	$MERIC_ROOT/tools/staticMERICtool/multiNodeStaticMeasureStop.sh --rapl
	Evaluate
	i=$(echo "$i + 1" | bc)
done

echo "MERIC time:" $( awk "BEGIN {print $SUMTIME / $REPEAT_COUNT}" )
echo "MERIC CPUs energy:" $( awk "BEGIN {print $SUMCPUs / $REPEAT_COUNT}" )
echo "MERIC NODEs energy:" $( awk "BEGIN {print $SUMNODEs / $REPEAT_COUNT}" )


