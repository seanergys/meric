#!/bin/bash
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
if $RAPL
then
	pdsh -w $NODE_LIST "source /etc/profile.d/10_modules.sh; module load x86_adapt 2> /dev/null; module load intel 2> /dev/null; export LD_LIBRARY_PATH+=:/usr/local/lib; $HOME/bin/energyMeasureStart"
else #HDEEM
	pdsh -w $NODE_LIST "source /etc/profile.d/10_modules.sh; module load hdeem 2> /dev/null; clearHdeem; startHdeem"
fi






