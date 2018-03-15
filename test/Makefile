COMPILER=INTEL
#COMPILER=GCC

ifeq ($(COMPILER),INTEL)
	CC=icpc
	MPICC=mpiicpc
	FCC=ifort
	OPENMP=-qopenmp
else #ifeq($(COMPILER),GCC)
	CC=g++
	MPICC=mpic++
	FCC=gfortran
	OPENMP=-fopenmp
endif

################################################################################
MERIC=-L../lib -I../include
HDEEM=-L../hdeem -lhdeem
FREEIPMI=-lfreeipmi
X86ADAPT=-L/usr/local/lib -lx86_adapt
CPUFREQ=-lcpufreq
PAPI=-L/sw/taurus/libraries/papi/5.5.1/lib/ -lpapi
#PAPI=-L/apps/papi/5.4.3/lib/ -lpapi # PAPI on BSC ARM machines

#if x86_adapt is missing, numa library is required
ifeq ($(X86ADAPT),)
	X86ADAPT=-lnuma
endif

BINARIES=test test_mpi blasTest samples sleepTest fortTest
################################################################################

default: all

all: samplesTest mpiTest basicTest sleepTest fortTest blasTestInfo overheadTest

 ######
sleepTest: sleeptest.cpp
	$(CC) -O0 -g sleeptest.cpp $(MERIC) -lmeric -lrt $(OPENMP) $(HDEEM) $(FREEIPMI) $(X86ADAPT) $(CPUFREQ) $(PAPI) -o sleepTest -DUSE_MERIC
	
mpiTest: test_mpi.cpp
	$(MPICC) -O0 -g test_mpi.cpp $(MERIC) -lmericmpi -lrt $(OPENMP) $(HDEEM) $(FREEIPMI) $(X86ADAPT) $(CPUFREQ) $(PAPI) -o test_mpi

basicTest: test.cpp
	$(CC)    -O0 -g test.cpp -o test -DUSE_MERIC $(MERIC) -lmeric -lrt $(OPENMP) $(HDEEM) $(FREEIPMI) $(X86ADAPT) $(CPUFREQ) $(PAPI)

scorepTest: test.cpp
	scorep --online-access --user --thread=none g++ -O0 -g test.cpp $(MERIC) -lrt -o test -DUSE_SCOREP_MANUAL

overhead: overheadTest.cpp
	$(CC)   -g overheadTest.cpp -DUSE_MERIC $(MERIC) -lmeric -lrt $(OPENMP) $(HDEEM) $(FREEIPMI) $(X86ADAPT) $(CPUFREQ) $(PAPI) -o overHead -DLOAD_MAX

samplesTest: samples_test.cpp
	$(CC)    -O0 -g samples_test.cpp $(MERIC) -lmeric -lrt $(OPENMP) $(HDEEM) $(FREEIPMI) $(X86ADAPT) $(CPUFREQ) $(PAPI) -o samples

blasTest: blas_test.c
	$(CC) -O3 -g blas_test.c $(MERIC) -lmeric -mkl -lrt $(OPENMP) $(HDEEM) $(FREEIPMI) $(X86ADAPT) $(CPUFREQ) $(PAPI) -o blasTest

blasTestInfo:
	$(info )
	$(info INFO: to compile blas_test.c load mkl module and compile using "make blasTest" )
	$(info )

fortTest:
	$(FCC) fort_test.f90 -cpp $(MERIC) -lmeric -lrt $(OPENMP) $(HDEEM) $(FREEIPMI) $(X86ADAPT) $(CPUFREQ) $(PAPI) -o fortTest -DUSE_MERIC


 ######
MeasurementDIRS=hdeemMeasurement* mericMeasurement* TEST* TESTMPI*

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
