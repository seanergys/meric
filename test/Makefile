# if env variables not set, using GNU compiler as default ######################

ifeq ($(CXX),)
	CXX=g++
endif
ifeq ($(FC),)
	FC=gfortran
endif
MPICXX=mpicxx

ifeq ($(CXX),g++)
	OPENMP=-fopenmp
else
	OPENMP=-qopenmp
endif


################################################################################
MERIC=-L../lib -I../include 
#DAVIDE=-L$(HOME)/rest-client/libs/ -lrestclient-cpp
#HDEEM=-L../hdeem -lhdeem -lfreeipmi
#X86ADAPT=-L/usr/local/lib -lx86_adapt
#CPUFREQ=-L$(HOME)/cpufrequtils-008/ -lcpufreq
#PAPI=-L/sw/taurus/libraries/papi/5.5.1/lib/ -lpapi
#PAPI=-L/apps/papi/5.4.3/lib/ -lpapi # PAPI on BSC ARM machines

#if x86_adapt is missing, numa library is required
ifeq ($(X86ADAPT),)
	X86ADAPT=-lnuma
endif

BINARIES=test test_mpi blasTest samples sleepTest fortTest overHead
################################################################################

default: all

all: samplesTest mpiTest basicTest sleepTest fortTest overhead blasTestInfo

 ######
sleepTest: sleep_test.cpp
	$(CXX) -O0 -g sleep_test.cpp $(MERIC) -lmeric -lrt $(OPENMP) $(HDEEM) $(X86ADAPT) $(CPUFREQ) $(PAPI) $(DAVIDE) -o sleepTest -DUSE_MERIC
	
mpiTest: test_mpi.cpp
	$(MPICXX) -O0 -g test_mpi.cpp $(MERIC) -lmericmpi -lrt $(OPENMP) $(HDEEM) $(X86ADAPT) $(CPUFREQ) $(PAPI) $(DAVIDE) -o test_mpi

basicTest: test.cpp
	$(CXX)    -O0 -g test.cpp -o test -DUSE_MERIC $(MERIC) -lmeric -lrt $(OPENMP) $(HDEEM) $(X86ADAPT) $(CPUFREQ) $(PAPI) $(DAVIDE)

scorepTest: test.cpp
	scorep --online-access --user --thread=none $(CXX) -O0 -g test.cpp $(MERIC) -lrt -o test -DUSE_SCOREP_MANUAL

overhead: overhead_test.cpp
	$(CXX)   -g overhead_test.cpp -DUSE_MERIC $(MERIC) -lmeric -lrt $(OPENMP) $(HDEEM) $(X86ADAPT) $(CPUFREQ) $(PAPI) $(DAVIDE) -o overHead

samplesTest: samples_test.cpp
	$(CXX)    -O0 -g samples_test.cpp $(MERIC) -lmeric -lrt $(OPENMP) $(HDEEM) $(X86ADAPT) $(CPUFREQ) $(PAPI) $(DAVIDE) -o samples

blasTest: blas_test.c
	$(CXX) -O3 -g blas_test.c $(MERIC) -lmeric -mkl -lrt $(OPENMP) $(HDEEM) $(X86ADAPT) $(CPUFREQ) $(PAPI) $(DAVIDE) -o blasTest

blasTestInfo:
	$(info )
	$(info INFO: to compile blas_test.c load mkl module and compile using "make blasTest" )
	$(info )

fortTest:
	$(FC) fort_test.f90 -cpp $(MERIC) -lmeric -lrt $(OPENMP) $(HDEEM) $(X86ADAPT) $(CPUFREQ) $(PAPI) $(DAVIDE) -o fortTest -DUSE_MERIC


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
