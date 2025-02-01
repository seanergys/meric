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

ifeq ($(CC),)
	CC=gcc
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

ifeq ($(shell echo ${CXX} | awk '{n=split($$1,0A,"/"); print A[n]}'),icpc)
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
NVML=-lnvidia-ml
#ESMI=-le_smi64
#LIBMSR=-lmsr -L${LIBMSR_PATH}/lib -I${LIBMSR_PATH}/include
################################################################################
MERIC=-L../lib -I../include 
LIBS=-lrt $(MERIC) $(OPENMP) $(HDEEM) $(X86ADAPT) $(CPUFREQ) $(PAPI) $(RESTCLIENT) $(NUMA) $(NVML) $(ESMI) $(LIBMSR) $(CXX_FLAGS)
BINARIES=test test_mpi blasTest samples sleepTest fortTest overHead
################################################################################
MKL_PATHS=
################################################################################

default: all

all: samplesTest mpiTest basicTest sleepTest overhead blasTestInfo fortTestInfo

 ######

midlib_test: midlib_test.c
	$(CC) -O0 -g midlib_test.c -L.. -la64fx_meric $(OPENMP) $(NVML) -o midlibTest -DHAVE_NVML_H

midlib_work_meric_test: midlib_work_meric_test.cpp
	$(CXX) -O0 -g midlib_work_meric_test.cpp -lmeric $(LIBS) -o midlib_work_meric_test

sleepTest: sleep_test.cpp
	$(CXX) -O0 -g sleep_test.cpp -lmeric $(LIBS) -o sleepTest
	
mpiTest: test_mpi.cpp
	$(MPICXX) -O0 -g test_mpi.cpp -lmericmpi $(LIBS) -o test_mpi

basicTest: test.cpp
	$(CXX)  -O0 -g test.cpp -o test -lmeric $(LIBS)

overhead: overhead_test.cpp
	$(CXX)   -g overhead_test.cpp -std=c++11 -lmeric $(LIBS) -o overHead

samplesTest: samples_test.cpp
	$(CXX)    -O0 -g samples_test.cpp -lmeric $(LIBS) -o samples

blasTest: blas_test.c
	$(CXX) -O3 -g blas_test.c -lmeric -mkl $(MKL_PATHS) $(LIBS) -o blasTest

blasTestInfo:
	$(info )
	$(info INFO: to compile blas_test.c set MKL include and lib paths and compile using "make blasTest" )
	$(info )

fortTest:
	$(FC) fort_test.f90 -cpp -lmeric $(LIBS) -o fortTest

fortTestInfo:
	$(info )
	$(info INFO: to compile fort_test.f90 compile MERIC fortran using "make fortran" and compile the test app using "make fortTest" )
	$(info )


 ######
MeasurementDIRS=mericMeasurement* TEST* TESTMPI*

clean: 
	rm -rf ${BINARIES} *.csv *.o *~ *.out *.qsub

cleanAll:
	make clean
	rm -rf $(MeasurementDIRS) *.tmp


