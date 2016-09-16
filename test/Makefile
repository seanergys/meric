
BINARIES=test_mpi basic_test weird_test
OUTPUT=A B

default: basicTest weirdTest

all: mpiTest basicTest weirdTest 

mpiTest: distributed.cpp
	mpic++ -O2 distributed.cpp -L../lib -lmericmpi -L../hdeem -lhdeem -lrt -o test_mpi

basicTest: shared.cpp
	c++    -O2 shared.cpp      -L../lib -lmeric    -L../hdeem -lhdeem -lrt -o basic_test

weirdTest: weird.cpp
	gcc    -O2 weird.cpp       -L../lib -lmeric    -L../hdeem -lhdeem -lrt -o weird_test

clean: 
	rm -f ${BINARIES} ${OUTPUT} *.o *.tmp
