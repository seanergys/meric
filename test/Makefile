BINARIES=test test_mpi blasTest

CC=icpc
MPICC=mpiicpc

HDEEM=-L../hdeem -lhdeem
X86ADAPT=-L/usr/local/lib -lx86_adapt
CPUFREQ=-lcpufreq
PAPI=-L/sw/taurus/libraries/papi/5.4.1/lib/ -lpapi

default: all

all: mpiTest basicTest blasTest

mpiTest: test_mpi.cpp
	$(MPICC) -O2 -g test_mpi.cpp    -L../lib -lmericmpi -lrt -openmp $(HDEEM) $(X86ADAPT) $(CPUFREQ) $(PAPI) -o test_mpi

basicTest: test.cpp
	$(CC)    -O2 -g test.cpp        -L../lib -lmeric -openmp  $(HDEEM) $(X86ADAPT) $(CPUFREQ) $(PAPI) -lrt -o test

blasTest: blas_test.c
	$(info )
	$(info WARNING: Makefile loads mkl module that is necessary for BLAS test )
	$(info )
	module load mkl;
	$(CC)   -O2 -g blas_test.c      -L../lib -I../include -lmeric -mkl      $(HDEEM) $(X86ADAPT) $(CPUFREQ) $(PAPI) -lrt -o blasTest

clean: 
	rm -rf ${BINARIES} *.csv *.o *~ *.out *.qsub

cleanAll:
	make clean
	rm -rf hdeemMeasurement *.tmp

run:
	. environment
	mpirun -n 10 -genv I_MPI_DEBUG=5 ./test_mpi

srun:
	srun -N 1 -c 24 --tasks-per-node=1 --exclusive -p haswell --x11 --pty bash
