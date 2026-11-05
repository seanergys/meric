#!/bin/bash

HLINE='---------------------------------------------'

if [ -z "$MERIC_SANITY_PATH_TO_BINARIES" ]; then
	MERIC_SANITY_PATH_TO_BINARIES='bin'
fi
echo "INFO: MERIC_SANITY_PATH_TO_BINARIES variable is set to: $MERIC_SANITY_PATH_TO_BINARIES"

if [ -z "$MERIC_SANITY_PATH_TO_TESTS" ]; then
	MERIC_SANITY_PATH_TO_TESTS='test'
fi
echo "INFO: MERIC_SANITY_PATH_TO_TESTS variable is set to: $MERIC_SANITY_PATH_TO_TESTS"

echo $HLINE
echo 'Obtaining available power monitoring systems:'

echo 'Running command: systemInfo...'
SYSTEM_INFO_OUTPUT=$("$MERIC_SANITY_PATH_TO_BINARIES"/systemInfo 2>system_info.err)
RETURN_CODE=$?
if [ ! -e system_info.err ]; then
	echo 'ERROR: Failed to redirect standard error to system_info.err - file is missing'
	RETURN_CODE=1
elif [ $RETURN_CODE -gt 0 ] || [ -s system_info.err ]; then
	if [ $RETURN_CODE -eq 0 ]; then
		echo 'ERROR: Command finished with error(s)'
		RETURN_CODE=1
	else
		echo 'ERROR: Command finished with non-zero exit code'
		echo "Exit code: $RETURN_CODE"
	fi

	if [ -s system_info.err ]
	then
		echo -e "Standard error:\n$(cat system_info.err)"
	fi
fi
if [ -e system_info.err ]
then
	rm -f system_info.err
fi

if [ $RETURN_CODE -gt 0 ]
then
	exit $RETURN_CODE
fi

echo 'Done'

AVAILABLE_POWER_MONITORING_SYSTEMS=$(echo "$SYSTEM_INFO_OUTPUT" | sed '1,/^# AVAILABLE POWER MONITORING SYSTEMS$/d' | tr -d '\t')
echo $HLINE
echo -e "Available power monitoring systems:\n$AVAILABLE_POWER_MONITORING_SYSTEMS"

if [ "$AVAILABLE_POWER_MONITORING_SYSTEMS" = 'NONE' ]
then
	exit 0
fi

echo $HLINE
echo 'Compiling plain test:'
echo "Running command: make -C $MERIC_SANITY_PATH_TO_TESTS plainTest..."
make -C "$MERIC_SANITY_PATH_TO_TESTS" plainTest
RETURN_CODE=$?
if [ $RETURN_CODE -gt 0 ]; then exit $RETURN_CODE; fi
echo 'Done'

for POWER_MONITORING_SYSTEM in $AVAILABLE_POWER_MONITORING_SYSTEMS; do
	echo $HLINE
	echo "Testing $POWER_MONITORING_SYSTEM power monitoring system:"

	echo "Running command: mericStatic -e $POWER_MONITORING_SYSTEM -- ${MERIC_SANITY_PATH_TO_TESTS}/plainTest..."
	"$MERIC_SANITY_PATH_TO_BINARIES"/mericStatic -e "$POWER_MONITORING_SYSTEM" -- "$MERIC_SANITY_PATH_TO_TESTS"/plainTest >/dev/null 2>meric_static.err
	RETURN_CODE=$?
	if [ ! -e meric_static.err ]; then
		echo 'ERROR: Failed to redirect standard error to meric_static.err - file is missing'
		RETURN_CODE=1
	elif [ $RETURN_CODE -gt 0 ] || [ -s meric_static.err ]; then
		if [ $RETURN_CODE -eq 0 ]; then
			echo 'ERROR: Command finished with error(s)'
			RETURN_CODE=1
		else
			echo 'ERROR: Command finished with non-zero exit code'
			echo "Exit code: $RETURN_CODE"
		fi

		if [ -s meric_static.err ]
		then
			echo -e "Standard error:\n$(cat meric_static.err)"
		fi
	fi
	if [ -e meric_static.err ]
	then
		rm -f meric_static.err
	fi

	if [ $RETURN_CODE -gt 0 ]
	then
		rm -f "$MERIC_SANITY_PATH_TO_TESTS"/plainTest
		if [ -d mericStaticMeasurements ]
		then
			rm -rf mericStaticMeasurements
		fi

		exit $RETURN_CODE
	fi

	echo 'Checking for output data (running command: ls mericStaticMeasurements/mericStatic_*.data)...'
	FOUND_OUTPUT_DATA=$(ls mericStaticMeasurements/mericStatic_*.data)
	RETURN_CODE=$?
	if [ $RETURN_CODE -gt 0 ]; then
		rm -f "$MERIC_SANITY_PATH_TO_TESTS"/plainTest
		if [ -d mericStaticMeasurements ]; then
			rm -rf mericStaticMeasurements
		fi

		exit $RETURN_CODE
	fi
	FOUND_OUTPUT_DATA=$(echo "$FOUND_OUTPUT_DATA" | head -n 1)

	echo "Checking measured values in ${FOUND_OUTPUT_DATA}..."
	MEASURED_VALUES=$(<"$FOUND_OUTPUT_DATA" grep -o '[0-9]\+\.[0-9]\+')
	for MEASURED_VALUE in $MEASURED_VALUES; do
		if [ "$(echo "${MEASURED_VALUE}==0" | bc)" -eq 1 ]; then
			rm -f "$MERIC_SANITY_PATH_TO_TESTS"/plainTest
			rm -rf mericStaticMeasurements

			echo "ERROR: It has been detected that $POWER_MONITORING_SYSTEM power monitoring system is available, but one or more of the measured values are equal to zero"
			exit 1
		fi
	done
	rm -rf mericStaticMeasurements

	echo 'Done'
done

rm -f "$MERIC_SANITY_PATH_TO_TESTS"/plainTest
