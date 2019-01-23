################################################################################
# MERIC test applications Makefile                                             #
# reads $CXX, $FC and $MPICXX environment variables                            #
# if env variables not set, using GNU compiler as default                      #
# in the second section specify which libraries you use and their paths        #
################################################################################

ifeq ($(CXX),)
	CXX=g++
endif

ifeq ($(FC),)
	FC=gfortran
	ifeq (, $(shell which ${FC}))
		FC=\#
	endif
endif

ifeq ($(MPICXX),)
	MPICXX=mpicxx
	ifeq (, $(shell which ${MPICXX}))
		MPICXX=\#
	endif
endif

ifeq ($(shell echo ${CXX} | awk '{n=split($$1,A,"/"); print A[n]}'),g++)
	OPENMP=-fopenmp
else
	OPENMP=-qopenmp
endif

################################################################################
#HDEEM=-L../hdeem -lhdeem -lfreeipmi
#X86ADAPT=-L/usr/local/lib -lx86_adapt
#CPUFREQ=-L$(HOME)/cpufrequtils-008/ -lcpufreq
#CPUFREQ=-L/usr/lib64/ /lib64/libcpupower.so.0
#CPUFREQ=-L/lib64/ -lcpupower
#PAPI=-lpapi
#PAPI=-L/sw/taurus/libraries/papi/5.5.1/lib/ -lpapi
#PAPI=-L/apps/papi/5.4.3/lib/ -lpapi # PAPI on BSC ARM machines
#RESTCLIENT=-L$(HOME)/rest-client/libs/ -lrestclient-cpp
################################################################################
#if x86_adapt is missing, numa library is required
ifeq ($(X86ADAPT),)
	X86ADAPT=-lnuma
endif
MERIC=-L../lib -I../include 
LIBS=-lrt $(MERIC) $(OPENMP) $(HDEEM) $(X86ADAPT) $(CPUFREQ) $(PAPI) $(RESTCLIENT)
BINARIES=test test_mpi blasTest samples sleepTest fortTest overHead
################################################################################

default: all

all: samplesTest mpiTest basicTest sleepTest overhead blasTestInfo fortTest

 ######
sleepTest: sleep_test.cpp
	$(CXX) -O0 -g sleep_test.cpp -lmeric $(LIBS) -o sleepTest -DUSE_MERIC
	
mpiTest: test_mpi.cpp
	$(MPICXX) -O0 -g test_mpi.cpp -lmericmpi $(LIBS) -o test_mpi -DUSE_MERIC

basicTest: test.cpp
	$(CXX)    -O0 -g test.cpp -o test -DUSE_MERIC -lmeric $(LIBS)

scorepTest: test.cpp
	scorep --online-access --user --thread=none $(CXX) -O0 -g test.cpp $(MERIC) -lrt -o test -DUSE_SCOREP_MANUAL

overhead: overhead_test.cpp
	$(CXX)   -g overhead_test.cpp -std=c++11 -DUSE_MERIC -lmeric $(LIBS) -o overHead

samplesTest: samples_test.cpp
	$(CXX)    -O0 -g samples_test.cpp -lmeric $(LIBS) -o samples

blasTest: blas_test.c
	$(CXX) -O3 -g blas_test.c -lmeric -mkl $(LIBS) -o blasTest

blasTestInfo:
	$(info )
	$(info INFO: to compile blas_test.c load mkl module and compile using "make blasTest" )
	$(info )

fortTest:
	$(FC) fort_test.f90 -cpp -lmeric $(LIBS) -o fortTest -DUSE_MERIC


 ######
MeasurementDIRS=hdeemMeasurement* mericMeasurement* TEST* TESTMPI*

clean: 
	rm -rf ${BINARIES} *.csv *.o *~ *.out *.qsub

cleanAll:
	make clean
	rm -rf $(MeasurementDIRS) *.tmp


