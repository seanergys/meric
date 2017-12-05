COMPILER=INTEL
#COMPILER=GCC

ifeq ($(COMPILER),INTEL)
	CC=icpc
	MPICC=mpiicpc
	OPENMP=-qopenmp
else #ifeq($(COMPILER),GCC)
	CC=g++
	MPICC=mpic++
	OPENMP=-fopenmp
endif

################################################################################
MERIC=-L../lib -I../include
HDEEM=-L../hdeem -lhdeem
FREEIPMI=-lfreeipmi
X86ADAPT=-L/usr/local/lib -lx86_adapt
CPUFREQ=-lcpufreq
PAPI=-L/sw/taurus/libraries/papi/5.5.1/lib/ -lpapi
#PAPI=-L/apps/papi/5.5.1/lib/ -lpapi

BINARIES=test test_mpi blasTest samples
################################################################################

default: all

all: samplesTest mpiTest basicTest blasTest sleepTest

 ######
sleepTest: sleeptest.cpp
	$(CC) -O0 -g sleeptest.cpp $(MERIC) -lmeric -lrt $(OPENMP) $(HDEEM) $(FREEIPMI) $(X86ADAPT) $(CPUFREQ) $(PAPI) -o sleeptest -DUSE_MERIC
#	$(CC) -O0 -g sleeptest.cpp $(MERIC) -lmeric -lrt $(OPENMP) $(HDEEM) $(FREEIPMI) $(X86ADAPT) $(PAPI) -o sleeptest -DUSE_MERIC

	
mpiTest: test_mpi.cpp
	$(MPICC) -O0 -g test_mpi.cpp $(MERIC) -lmericmpi -lrt $(OPENMP) $(HDEEM) $(FREEIPMI) $(X86ADAPT) $(CPUFREQ) $(PAPI) -o test_mpi

basicTest: test.cpp
	$(CC)    -O0 -g test.cpp $(MERIC) -lmeric -lrt $(OPENMP) $(HDEEM) $(FREEIPMI) $(X86ADAPT) $(CPUFREQ) $(PAPI) -o test -DUSE_MERIC

scorepTest: test.cpp
	scorep --online-access --user --thread=none g++ -O0 -g test.cpp $(MERIC) -lrt -o test -DUSE_SCOREP_MANUAL

samplesTest: samples_test.cpp
	$(CC)    -O0 -g samples_test.cpp $(MERIC) -lmeric -lrt $(OPENMP) $(HDEEM) $(FREEIPMI) $(X86ADAPT) $(CPUFREQ) $(PAPI) -o samples

blasTest: blas_test.c
	$(info )
	$(info WARNING: Makefile loads mkl module that is necessary for BLAS test )
	$(info )
	module load mkl;
	$(CC) -O0 -g blas_test.c $(MERIC) -lmeric -mkl -lrt $(OPENMP) $(HDEEM) $(FREEIPMI) $(X86ADAPT) $(CPUFREQ) $(PAPI) -o blasTest


 ######
MeasurementDIRS=hdeemMeasurement* TEST* TESTMPI*

clean: 
	rm -rf ${BINARIES} *.csv *.o *~ *.out *.qsub

cleanAll:
	make clean
	rm -rf $(MeasurementDIRS) *.tmp

 ######
run:
	./test
	mpirun -n 4 -genv I_MPI_DEBUG=5 ./test_mpi

srun:
	srun -N 1 -c 24 --tasks-per-node=1 --exclusive -p haswell --x11 --pty bash
