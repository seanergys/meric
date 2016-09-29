BINARIES=test_mpi basic_test weird_test
FOLDERS=A B

CC=icpc
MPICC=mpiicpc

default: all

all: mpiTest basicTest weirdTest 

mpiTest: distributed.cpp
	${MPICC} -O2 -g distributed.cpp -L../lib -lmericmpi -L../hdeem -lhdeem -lrt -o test_mpi

mpiicpc: distributed.cpp
	mpiicpc -O2 -g distributed.cpp -L../lib -lmericmpi -L../hdeem -lhdeem -lrt -o test_mpi

basicTest: shared.cpp
	${CC}    -O2 -g shared.cpp      -L../lib -lmeric    -L../hdeem -lhdeem -lrt -o basic_test

weirdTest: weird.cpp
	${CC}    -O2 -g weird.cpp       -L../lib -lmeric    -L../hdeem -lhdeem -lrt -o weird_test

clean: 
	rm -rf ${BINARIES} ${FOLDERS} *.csv *.o *.tmp hdeemMeasurement

cleanFolders:
	rm -rf ${FOLDERS} hdeemMeasurement
