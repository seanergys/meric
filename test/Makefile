BINARIES=test test_mpi weird_test test3
FOLDERS=A B

CC=icpc
MPICC=mpiicpc

HDEEM=-L../hdeem -lhdeem
X86ADAPT=-L/usr/local/lib -lx86_adapt
CPUFREQ=-lcpufreq
PAPI=-L/sw/taurus/libraries/papi/5.4.1/lib/ -lpapi

default: all

all: mpiTest basicTest weirdTest test3

mpiTest: test_mpi.cpp
	$(MPICC) -O2 -g test_mpi.cpp    -L../lib -lmericmpi -lrt -openmp $(HDEEM) $(X86ADAPT) $(CPUFREQ) $(PAPI) -o test_mpi

basicTest: test.cpp
	${CC}    -O2 -g test.cpp        -L../lib -lmeric -openmp  $(HDEEM) $(X86ADAPT) $(CPUFREQ) $(PAPI) -lrt -o test

weirdTest: weird.cpp
	${CC}    -O2 -g weird.cpp       -L../lib -lmeric -openmp  $(HDEEM) $(X86ADAPT) $(CPUFREQ) $(PAPI) -lrt -o weird_test

test3: test3.cpp
	${CC}    -O2 -g test3.cpp       -L../lib -lmeric -openmp  $(HDEEM) $(X86ADAPT) $(CPUFREQ) $(PAPI) -lrt -o test3


clean: 
	rm -rf ${BINARIES} *.csv *.o *~ *.out *.qsub

cleanAll:
	rm -rf hdeemMeasurement *.tmp

run:
	. /home/vysocky/meric+/test/environment
	mpirun -n 10 -genv I_MPI_DEBUG=5 ./test_mpi

srun:
	srun -N 1 -c 24 --tasks-per-node=1 --exclusive -p haswell --x11 --pty bash
