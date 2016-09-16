
BINARIES=test_mpi basic_test weird_test
OUTPUT=A B

default: basicTest weirdTest

all: mpiTest basicTest weirdTest 

mpiTest: distributed.cpp
	mpic++ -O2 -g distributed.cpp -L../lib -lmericmpi -L../hdeem -lhdeem -lrt -o test_mpi

mpiicpc: distributed.cpp
	mpiicpc -O2 -g distributed.cpp -L../lib -lmericmpi -L../hdeem -lhdeem -lrt -o test_mpi

basicTest: shared.cpp
	c++    -O2 -g shared.cpp      -L../lib -lmeric    -L../hdeem -lhdeem -lrt -o basic_test

weirdTest: weird.cpp
	gcc    -O2 -g weird.cpp       -L../lib -lmeric    -L../hdeem -lhdeem -lrt -o weird_test

clean: 
	rm -f ${BINARIES} ${OUTPUT} *.o *.tmp
