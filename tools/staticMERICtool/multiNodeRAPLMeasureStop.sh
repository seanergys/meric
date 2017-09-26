#!/bin/sh
# gives support for PBS and SLURM too

######################################################## get the list of nodes #
if $(printenv | grep -q "SLURM")
then	# SLURM
	# taurusi6605
	# taurusi[6605-6606]
	# taurusi[6531-6532,6536,6539]

	if $(grep -q "\[" <<< $SLURM_JOB_NODELIST)
	then
		prefix=$(cut -d "[" -f 1 <<< $SLURM_JOB_NODELIST)
		sufix=$(cut -d "[" -f 2 <<< $SLURM_JOB_NODELIST)

		for id in for i in $(echo "${sufix%?}" | tr ',' ' ')
		do
			if grep -q '-' <<< $id
			then
				first=$(cut -d "-" -f 1 <<< $id)
				last=$(cut -d "-" -f 2 <<< $id)
				while [ $first -neq $last ]
				do
					NODE_LIST+=$prefix$first','
					first=$[ first+1]
				done
			else
				NODE_LIST+=$prefix$id','
			fi
			NODE_LIST="${NODE_LIST%?}"
		done
	else
		NODE_LIST=$SLURM_JOB_NODELIST
	fi

else	# PBS
	NODE_LIST=$(cut -d "." -f 1 $PBS_NODEFILE | uniq | tr '\n' ',')
	NODE_LIST="${NODE_LIST%?}"
fi

############################# get the results of the measurement for each node #
pdsh -w $NODE_LIST "module load x86_adapt;$HOME/bin/energyMeasureStop > /dev/null"

########################################### get the overall energy consumption #
SUM=0
for node in $( echo $NODE_LIST | tr ',' ' ')
do
	SUM=$[ SUM + $(cat "/scratch/temp/.MERICmeasure-"$node".data") ]
done

echo
echo "Overall energy consumption is" $SUM "J"
