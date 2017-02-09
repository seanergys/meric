BINARIES=test test_mpi blasTest

CC=icpc
MPICC=mpiicpc

HDEEM=-L../hdeem -lhdeem
FREEIPMI=-lfreeipmi
X86ADAPT=-L/usr/local/lib -lx86_adapt
CPUFREQ=-lcpufreq
PAPI=-L/sw/taurus/libraries/papi/5.4.1/lib/ -lpapi

default: all

all: samplesTest mpiTest basicTest blasTest

 ######
mpiTest: test_mpi.cpp
	$(MPICC) -O0 -g test_mpi.cpp    -L../lib -lmericmpi -lrt -qopenmp $(HDEEM) $(FREEIPMI) $(X86ADAPT) $(CPUFREQ) $(PAPI) -o test_mpi

basicTest: test.cpp
	$(CC)    -O0 -g test.cpp        -L../lib -lmeric -qopenmp  $(HDEEM) $(FREEIPMI) $(X86ADAPT) $(CPUFREQ) $(PAPI) -lrt -o test

samplesTest: samples_test.cpp
	$(CC)    -O0 -g samples_test.cpp        -L../lib -lmeric -qopenmp  $(HDEEM) $(FREEIPMI) $(X86ADAPT) $(CPUFREQ) $(PAPI) -lrt -o samples

blasTest: blas_test.c
	$(info )
	$(info WARNING: Makefile loads mkl module that is necessary for BLAS test )
	$(info )
	module load mkl;
	$(CC) -O0 -g blas_test.c      -L../lib -I../include -lmeric -mkl -qopenmp     $(HDEEM) $(FREEIPMI) $(X86ADAPT) $(CPUFREQ) $(PAPI) -lrt -o blasTest


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
