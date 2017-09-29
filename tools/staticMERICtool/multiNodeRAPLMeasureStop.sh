#!/bin/sh
# gives support for PBS and SLURM too

################################################################ RAPL or HDEEM #

OPT=`getopt -o h --long hdeem,rapl,help -n 'energyMeasure' -- "$@"`
if [ $? != 0 ]
then
	exit 1
fi

eval set -- "$OPT"
HDEEM=false
RAPL=false

while true
do
  case "$1" in
    --hdeem 	) HDEEM=true; shift ;;
    --rapl   	) RAPL=true; shift ;;
    -h | --help	) MEMORY="$2"; shift ;;
    -- ) shift; break ;;
    * ) break ;;
  esac
done

if ( $RAPL && $HDEEM ) || ( ! $RAPL && ! $HDEEM )
then
	echo "select only HDEEM (--hdeem) or only RAPL (--rapl)"
	exit 1
fi

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
if $RAPL
then
	OUTPUT=$(pdsh -w $NODE_LIST 'module load x86_adapt; module load intel; $HOME/bin/energyMeasureStop | tail -1 | awk '\''{ print $NF }'\'' ') 
else #HDEEM
	pdsh -w $NODE_LIST "$file=$HOME/hostname; stopHdeem; checkHdeem >> $file"
fi



########################################### get the overall energy consumption #
#echo $OUTPUT
if $RAPL
then
	for VAL in $(grep -o -E ': [0-9]+' <<< $OUTPUT | awk '{ print $NF }' )
	do 
		SUM=$[ SUM + VAL ]
	done
fi
#SUM=0
#for node in $( echo $NODE_LIST | tr ',' ' ')
#do
#	SUM=$[ SUM + $(cat "/scratch/temp/.MERICmeasure-"$node".data") ]
#done
#
echo
echo "Overall energy consumption is" $SUM "J"





