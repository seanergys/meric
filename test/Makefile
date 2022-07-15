################################################################################
# MERIC test applications Makefile                                             #
# Reads $CXX, $FC, $MPICXX and $CXX_FLAGS environment variables.               #
# If env variables not set, using GNU compiler as default.                     #
# In the second section of the Makefile specify which libraries you use and    #
# their paths                                                                  #
#                                                                              #
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

ifeq ($(shell echo ${CXX} | awk '{n=split($$1,A,"/"); print A[n]}'),icpc)
	OPENMP=-qopenmp
else ifeq ($(shell echo ${CXX} | awk '{n=split($$1,A,"/"); print A[n]}'),FCC)
	OPENMP=-Kopenmp
else
	OPENMP=-fopenmp
endif


################################################################################
#HDEEM=-lhdeem -lfreeipmi
#X86ADAPT=-lx86_adapt
#CPUFREQ=--lcpufreq
#CPUFREQ=-L/lib64/ -lcpupower
#PAPI=-lpapi
#RESTCLIENT= -lrestclient-cpp
#NUMA=-lnuma
#NVML=-lnvidia-ml
#ESMI=-le_smi64
#LIBMSR=-lmsr
################################################################################
MERIC=-L../lib -I../include 
LIBS=-lrt $(MERIC) $(OPENMP) $(HDEEM) $(X86ADAPT) $(CPUFREQ) $(PAPI) $(RESTCLIENT) $(NUMA) $(NVML) $(ESMI) $(LIBMSR) $(CXX_FLAGS)
BINARIES=test test_mpi blasTest samples sleepTest fortTest overHead
################################################################################

default: all

all: samplesTest mpiTest basicTest sleepTest overhead blasTestInfo fortTestInfo

 ######
sleepTest: sleep_test.cpp
	$(CXX) -O0 -g sleep_test.cpp -lmeric $(LIBS) -o sleepTest -DUSE_MERIC
	
mpiTest: test_mpi.cpp
	$(MPICXX) -O0 -g test_mpi.cpp -lmericmpi $(LIBS) -o test_mpi -DUSE_MERIC

basicTest: test.cpp
	$(CXX)    -O0 -g test.cpp -o test -DUSE_MERIC -lmeric $(LIBS)

scorepTest: test.cpp
	scorep --online-access --user --thread=none $(CXX) -O0 -g test.cpp $(MERIC) $(LIBS) -o test -DUSE_SCOREP_MANUAL

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

fortTestInfo:
	$(info )
	$(info INFO: to compile fort_test.f90 compile MERIC fortran using "make fortran" and compile the test app using "make fortTest" )
	$(info )


 ######
MeasurementDIRS=hdeemMeasurement* mericMeasurement* TEST* TESTMPI*

clean: 
	rm -rf ${BINARIES} *.csv *.o *~ *.out *.qsub

cleanAll:
	make clean
	rm -rf $(MeasurementDIRS) *.tmp


