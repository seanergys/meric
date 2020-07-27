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
HELP=false

while true
do
  case "$1" in
    --hdeem 	) HDEEM=true; shift ;;
    --rapl   	) RAPL=true; shift ;;
    -h | --help	) HELP=true; shift ;;
    -- ) shift; break ;;
    * ) break ;;
  esac
done

if $HELP
then
	echo "Ends the energy measurement executed at allocated nodes and restores the hardware settings"
	echo "select --hdeem or --rapl to choose energy measurement interface"
	exit 0
fi

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

		for id in $(echo "${sufix%?}" | tr ',' ' ')
		do
			if grep -q '-' <<< $id
			then
				first=$(cut -d "-" -f 1 <<< $id)
				last=$(cut -d "-" -f 2 <<< $id)
				while [ $first != $last ]
				do
					NODE_LIST+=$prefix$first','
					first=$[ first+1]
				done
				NODE_LIST+=$prefix$first','
			else
				NODE_LIST+=$prefix$id','
			fi
		done
		NODE_LIST="${NODE_LIST%?}"
	else
		NODE_LIST=$SLURM_JOB_NODELIST
	fi

else	# PBS
	NODE_LIST=$(cut -d "." -f 1 $PBS_NODEFILE | uniq | tr '\n' ',')
	NODE_LIST="${NODE_LIST%?}"
fi

############################# get the results of the measurement for each node #
LOCATION=$(pwd)'/'$( dirname "${BASH_SOURCE[0]}" )

if $RAPL	
then
	# source /etc/profile.d/10_modules.sh necessary for Taurus slurm
	# loading modules from set_env file, located in MERIC root directory
	OUTPUT=$(pdsh -w $NODE_LIST -N "source /etc/profile.d/10_modules.sh 2>/dev/null; source $LOCATION/../set_env 2> /dev/null; $LOCATION/energyMeasureStop")
	TIME=$(grep -oP "(?<=Runtime \[s\] = )\d+\.\d+"  <<< $OUTPUT | head -1)
	NODES=$(grep -oP "(?<=Energy consumption \[mJ\] = )\d+"  <<< $OUTPUT )
	echo "Runtime [s] =" $TIME
else #HDEEM
	OUTPUT=$(pdsh -w $NODE_LIST -N "source /etc/profile.d/10_modules.sh 2>/dev/null; file=$HOME/hostname; stopHdeem; checkHdeem | grep -o -E 'BLADE\s+[0-9]+\.[0-9]+\s+[0-9]+\.[0-9]+\s++[0-9]+\.[0-9]+\s+[0-9]+\.[0-9]+' | sed 's/  */ /g' | cut -d ' ' -f 5")
fi

#echo $OUTPUT

############################################## get the overall consumed energy #
SUM=0.0
for VAL in $NODES
do
	SUM=$(echo $SUM $VAL | awk '{print $1 + $2}')
done
echo "Overall nodes energy consumption [mJ] =" $SUM

exit 0



