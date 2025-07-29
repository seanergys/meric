#!/bin/bash

HLINE='---------------------------------------------'

if [ -z $PATH_TO_BINARIES ]
then
	PATH_TO_BINARIES='bin'
fi
echo "INFO: PATH_TO_BINARIES variable is set to: $PATH_TO_BINARIES"

if [ -z $PATH_TO_TESTS ]
then
	PATH_TO_TESTS='test'
fi
echo "INFO: PATH_TO_TESTS variable is set to: $PATH_TO_TESTS"

echo $HLINE
echo 'Obtaining available power monitoring systems:'

echo 'Running command: systemInfo...'
SYSTEM_INFO_OUTPUT=$(${PATH_TO_BINARIES}/systemInfo 2> system_info.err)
RETURN_CODE=$?
if [ ! -e system_info.err ]
then
	echo 'ERROR: Failed to redirect standart error to system_info.err - file is missing'
	RETURN_CODE=1
elif [ $RETURN_CODE -gt 0 ] || [ -s system_info.err ]
then
	if [ $RETURN_CODE -eq 0 ]
	then
		echo 'ERROR: Command finished with error(s)'
		RETURN_CODE=1
	else
		echo 'ERROR: Command finished with non-zero exit code'
		echo "Exit code: $RETURN_CODE"
	fi

	if [ -s system_info.err ]
	then
		echo -e "Standart error:\n$(cat system_info.err)"
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
echo "Running command: make -C $PATH_TO_TESTS plainTest..."
make -C $PATH_TO_TESTS plainTest
RETURN_CODE=$?; if [ $RETURN_CODE -gt 0 ]; then exit $RETURN_CODE; fi
echo 'Done'

for POWER_MONITORING_SYSTEM in $AVAILABLE_POWER_MONITORING_SYSTEMS
do
	echo $HLINE
	echo "Testing $POWER_MONITORING_SYSTEM power monitoring system:"

	echo "Running command: mericStatic -e $POWER_MONITORING_SYSTEM -- ${PATH_TO_TESTS}/plainTest..."
	MERIC_STATIC_OUTPUT=$(${PATH_TO_BINARIES}/mericStatic -e $POWER_MONITORING_SYSTEM -- ${PATH_TO_TESTS}/plainTest 2> meric_static.err)
	RETURN_CODE=$?
	if [ ! -e meric_static.err ]
	then
		echo 'ERROR: Failed to redirect standart error to meric_static.err - file is missing'
		RETURN_CODE=1
	elif [ $RETURN_CODE -gt 0 ] || [ -s meric_static.err ]
	then
		if [ $RETURN_CODE -eq 0 ]
		then
			echo 'ERROR: Command finished with error(s)'
			RETURN_CODE=1
		else
			echo 'ERROR: Command finished with non-zero exit code'
			echo "Exit code: $RETURN_CODE"
		fi

		if [ -s meric_static.err ]
		then
			echo -e "Standart error:\n$(cat meric_static.err)"
		fi
	fi
	if [ -e meric_static.err ]
	then
		rm -f meric_static.err
	fi

	if [ $RETURN_CODE -gt 0 ]
	then
		rm -f ${PATH_TO_TESTS}/plainTest
		if [ -d mericStaticMeasurements ]
		then
			rm -rf mericStaticMeasurements
		fi

		exit $RETURN_CODE
	fi

	JOB_ID=$(echo $MERIC_STATIC_OUTPUT | tr -d ' ' | cut -d ':' -f 2)
	NODE_NAME=$(hostname)

	echo "Checking measured values in mericStaticMeasurements/mericStatic_${JOB_ID}_${NODE_NAME}.data..."
	if [ ! -e mericStaticMeasurements/mericStatic_${JOB_ID}_${NODE_NAME}.data ]
	then
		rm -f ${PATH_TO_TESTS}/plainTest

		echo "ERROR: File mericStaticMeasurements/mericStatic_${JOB_ID}_${NODE_NAME}.data is missing"
		exit 1
	fi

	MEASURED_VALUES=$(cat mericStaticMeasurements/mericStatic_${JOB_ID}_${NODE_NAME}.data | grep -o '[0-9]\+\.[0-9]\+')
	for MEASURED_VALUE in $MEASURED_VALUES
	do
		if [ $(echo "${MEASURED_VALUE}==0" | bc) -eq 1 ]
		then
			rm -f ${PATH_TO_TESTS}/plainTest
			rm -rf mericStaticMeasurements

			echo "ERROR: It has been detected that $POWER_MONITORING_SYSTEM power monitoring system is available, but one or more of the measured values are equal to zero"
			exit 1
		fi
	done
	rm -rf mericStaticMeasurements

	echo 'Done'
done

rm -f ${PATH_TO_TESTS}/plainTest
