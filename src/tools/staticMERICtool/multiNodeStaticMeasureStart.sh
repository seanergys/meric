#!/bin/bash
# gives support for PBS and SLURM too


################################################################ RAPL or HDEEM #

OPT=`getopt -o hc:u:t:w:p: --long hdeem,rapl,help -n 'energyMeasure' -- "$@"`
if [ $? != 0 ]
then
	exit 1
fi

eval set -- "$OPT"
HDEEM=false
RAPL=false
HELP=false
Cfreq=""
Ufreq=""
Threads=""
PowerLimit=""
PowerWindow=""

while true
do
  case "$1" in
    --hdeem 	) HDEEM=true; shift ;;
    --rapl   	) RAPL=true; shift ;;
    -c          ) Cfreq="-c "$2; shift; shift;;
    -u          ) Ufreq="-u "$2; shift; shift;;
    -t          ) Threads="-t "$2; shift; shift;;
    -p          ) PowerLimit="-p "$2; shift; shift;;
    -w          ) PowerWindow="-w "$2; shift; shift;;
    -h | --help	) HELP=true; shift ;;
    -- ) shift; break ;;
    * ) break ;;
  esac
done

if $HELP
then
	echo "Select --hdeem or --rapl to choose energy measurement interface"
	echo "In case of RAPL it is also possible to tune hardware settings:"
	echo " -c = CPU core frequency [Hz]"
	echo " -u = CPU uncore frequency [Hz]"
	echo " -t = number of active OpenMP threads [#]"
	echo " -p = power capping power limit [mW]"
	echo " -w = power capping time window [ms]"
	exit 0
fi

if ( $RAPL && $HDEEM ) || ( ! $RAPL && ! $HDEEM )
then
	echo "select only HDEEM (--hdeem) or only RAPL (--rapl)"
	exit 1
fi

HWparamters=$Cfreq" "$Ufreq" "$Threads" "$PowerLimit" "$PowerWindow

############################################################ get list of nodes #
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

#################################################### run the measurement there #
LOCATION=$(pwd)'/'$( dirname "${BASH_SOURCE[0]}" )

if $RAPL
then
	# loading modules from set_env file, located in MERIC root directory
	pdsh -w $NODE_LIST "source /etc/profile.d/10_modules.sh 2>/dev/null; source $LOCATION/../set_env 2> /dev/null; $LOCATION/energyMeasureStart $HWparamters"
else #HDEEM
	pdsh -w $NODE_LIST "source /etc/profile.d/10_modules.sh 2>/dev/null; stopHdeem; sleep 2; clearHdeem; startHdeem"
fi






