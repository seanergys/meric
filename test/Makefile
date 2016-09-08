
all:
	c++    -O2 shared.cpp      -L../lib -lmeric    -L../hdeem -lhdeem -lrt -o test

mpi:
	mpic++ -O2 distributed.cpp -L../lib -lmericmpi -L../hdeem -lhdeem -lrt -o test_mpi
