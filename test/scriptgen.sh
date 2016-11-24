#!/bin/bash
# this bash script generates SLURM scripts to run a code with different settings 
# ./scriptgen.sh run slurm pbs
# Ready to do BLAS test on Taurus.


EMAIL=ondrej.vysocky@vsb.cz
#WORKDIR=~/pool/intensity
#THREADS_PER_MPI=1
#MPI_PER_NODE=24


if [ "$1" = "run" ]; then
  qsub_command_0="" #"#!/bin/bash;"
  
 ##### EXPORT RUN INDEPENDENT VARIABLES #####   
#	qsub_command_0+="export VAR=1;"

 ##### MERIC MODULES ##### 
  qsub_command_0+="module purge;"
  qsub_command_0+="module load intel/2016.1.150;"
  qsub_command_0+="module load mpirt/5.1.2.150;"
  qsub_command_0+="module load hdeem/2.2.2;"
  qsub_command_0+="module load papi/5.4.1;"

 ##### YOUR APP MODULES ##### 
  qsub_command_0+="module load mkl;"
  
  qsub_command_0+="module list;"

 ##### FOR EACH RUN ##### 
  walltime=0-0:05
  queue=haswell
  memory=62000
#  account=OPEN-7-46

 ##### LOOPS ##### 
 #XXX - for each LOOP must be added "done" in another XXX section
  for FREQ in {12..25..2} #{12..25..1}
  do
  for UN_FREQ in 12 30 # {12..30..1}
  do
  for THRD in 1 #{1..24..1}
  do

 ##### RUN CONNECTED VARIABLES ##### 
    jobname=$FREQ"intensity"
    
    qsub_command=$qsub_command_0

 ##### SET ENVIRONMENT #####     
    
    qsub_command+="export MERIC_FREQUENCY="$FREQ";"
    qsub_command+="export MERIC_UNCORE_FREQUENCY="$UN_FREQ";"
    qsub_command+="export MERIC_OUTPUT_FILENAME="$THRD"_"$FREQ"_"$UN_FREQ";"
    qsub_command+="export MERIC_NUM_THREADS="$THRD";"
    qsub_command+="export MERIC_DEBUG=1;"
#    qsub_command+="export MERIC_COUNTERS=perfevent;"


 ##### QSUB ##### 

	if [ "$3" = "pbs" ]
	then
#		if [ "$2" = "intel" ]
#		then
#			actualTime=$( date +%y%m%d_%H:%M:%S )
#			log_file=LOG-$X:$Y:$Z-$x:$y:$z-$d:$d:$d-$c:$c:$c-$actualTime.log
#
#			qsub_command+=$time"mpirun      -n $(( X * Y * Z ))                 ./espreso -p $EXAMPLEDIR/$EXAMPLE ${el_type[0]} ${X} ${Y} ${Z} ${x} ${y} ${z} ${d} ${d} ${d} -vvv -mmm | tee -a $log_file;"    
#			echo $qsub_command | tr ";" "\n" | tr -d "¡" | tee $WORKDIR/$out_dir/job.qsub
#			echo "qsub -q "$queue" -A $account -l select=$(( ((X * Y * Z)/MPI_PER_NODE) + 1 )):ncpus=24:mpiprocs=$MPI_PER_NODE:ompthreads=$THREADS_PER_MPI -l walltime=00:30:00 -N $d$jobname" | tee -a $WORKDIR/$out_dir/job.qsub
#			echo $qsub_command | tr ";" "\n" | tr -d "¡" | \      
#			qsub -q qmpp -A $account -l select=$(( ((X * Y * Z)/MPI_PER_NODE) + 1 )):ncpus=24:mpiprocs=$MPI_PER_NODE:ompthreads=$THREADS_PER_MPI:perrin=true -l walltime=00:30:00 -N $d$jobname
#		fi

		if [ "$2" = "slurm" ]
		then
			PBS="#SBATCH -p "$queue" ;"
			PBS+="#SBATCH -t "$walltime" ;"
			PBS+="#SBATCH -N 4 ;"			#TODO count required number of nodes
			PBS+="#SBATCH --mem "$memory" ;"
			PBS+="#SBATCH --exclusive ;"
			PBS+="#SBATCH --ntasks-per-node=1 ;"	#one MPI proc per node
#			PBS+="#SBATCH --mail-type=ALL ;"
#			PBS+="#SBATCH --mail-user="$EMAIL" ;"

			##### YOUR SCRIPT #####
#			qsub_command+="./blasTest;"
                        qsub_command+="make;"
                        qsub_command+="export MERIC_OUTPUT_DIR=TEST;"
                        qsub_command+="./test;"
                        qsub_command+="export MERIC_OUTPUT_DIR=TESTMPI;"
                        qsub_command+="mpirun -n 4 ./test_mpi;"

			echo "#!/bin/bash;"$PBS$qsub_command | tr ";" "\n" | tr -d "¡" | tee $THRD"_"$FREQ"_"$UN_FREQ".qsub"
			sbatch $THRD"_"$FREQ"_"$UN_FREQ".qsub"
		fi
	fi
#XXX	#<<<
  done	
  done
  done
fi
