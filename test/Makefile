################################################################################
# MERIC test applications Makefile                                             #
# Reads $CXX, $FC, and $MPICXX environment variables.                          #
# If env variables not set, using GNU compiler as default.                     #
################################################################################

ifeq ($(CXX),)
	CXX=g++
endif

ifeq ($(CC),)
	CC=gcc
endif

ifeq ($(NVCC),)
	NVCC=nvcc
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

ifeq ($(MAKECMDGOALS),cudaTest)
	OPENMP=-Xcompiler -fopenmp
endif

################################################################################
LIBS=-L../lib -I../include
BINARIES=plainTest test test_mpi blasTest samples sleepTest fortTest overHead
################################################################################

default: all

all: plainTest samplesTest mpiTest basicTest sleepTest overhead blasTestInfo fortTestInfo cudaTestInfo

 ######

plainTest: plain_test.cpp
	$(CXX) -O0 -g plain_test.cpp $(LIBS) $(OPENMP) -o plainTest

midlib_test: midlib_test.c
	$(CC) -O0 -g  midlib_test.c -lmeric_ext $(LIBS) $(OPENMP) -lstdc++ -lm -o midlibTest

midlib_work_meric_test: midlib_work_meric_test.cpp
	$(CXX) -O0 -g midlib_work_meric_test.cpp -lmeric $(LIBS) $(OPENMP) -o midlib_work_meric_test

sleepTest: sleep_test.cpp
	$(CXX) -O0 -g sleep_test.cpp -lmeric $(LIBS) -o sleepTest
	
mpiTest: test_mpi.cpp
	$(MPICXX) -O0 -g test_mpi.cpp -lmericmpi $(LIBS) $(OPENMP) -o test_mpi

basicTest: test.cpp
	$(CXX) -O0 -g test.cpp -o test -lmeric $(LIBS) $(OPENMP)

overhead: overhead_test.cpp
	$(CXX)   -g overhead_test.cpp -std=c++11 -lmeric $(LIBS) -o overHead

samplesTest: samples_test.cpp
	$(CXX)    -O0 -g samples_test.cpp -lmeric $(LIBS) $(OPENMP) -o samples

blasTest: blas_test.c
	$(CC) -O3 -g blas_test.c -lmeric -mkl $(LIBS) $(OPENMP) -o blasTest

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

cudaTest: test_cuda.cu
	$(NVCC) -O0 -g test_cuda.cu -lmeric $(LIBS) -o testCuda
	
cudaTestInfo:
	$(info )
	$(info INFO: to compile test_cuda.cu set NVCC and compile the test app using "make cudaTest" )
	$(info )

 ######
MeasurementDIRS=mericMeasurement*

clean: 
	rm -rf ${BINARIES} *.csv *.o *~ *.out *.qsub

cleanAll:
	make clean
	rm -rf $(MeasurementDIRS) *.tmp


