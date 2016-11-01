BINARIES=test_mpi basic_test weird_test
FOLDERS=A B

CC=icpc
MPICC=mpiicpc

HDEEM=-L../hdeem -lhdeem
X86ADAPT=-L/usr/local/lib -lx86_adapt
CPUFREQ=-lcpufreq


default: all

all: mpiTest basicTest weirdTest 

mpiTest: distributed.cpp
	$(MPICC) -O2 -g distributed.cpp -L../lib -lmericmpi -lrt $(HDEEM) $(X86ADAPT) $(CPUFREQ) -o test_mpi

mpiicpc: distributed.cpp
	mpiicpc -O2 -g distributed.cpp -L../lib -lmericmpi $(HDEEM) $(X86ADAPT) $(CPUFREQ) -lrt -o test_mpi

basicTest: shared.cpp
	${CC}    -O0 -g shared.cpp      -L../lib -lmeric   $(HDEEM) $(X86ADAPT) $(CPUFREQ) -lrt -o basic_test

weirdTest: weird.cpp
	${CC}    -O2 -g weird.cpp       -L../lib -lmeric   $(HDEEM) $(X86ADAPT) $(CPUFREQ) -lrt -o weird_test

clean: 
	rm -rf ${BINARIES} ${FOLDERS} *.csv *.o *.tmp hdeemMeasurement

cleanFolders:
	rm -rf ${FOLDERS} hdeemMeasurement

run:
	. /home/vysocky/meric+/test/environment
	mpirun -n 10 -genv I_MPI_DEBUG=5 ./test_mpi

srun:
	srun -N 1 -c 24 --tasks-per-node=1 --exclusive -p haswell --x11 --pty bash
