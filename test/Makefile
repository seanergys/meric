
all:
	c++    -O2 shared.cpp      -L../lib -lmeric    -L../hdeem -lhdeem -lrt -o test
	gcc    -O2 weird.cpp       -L../lib -lmeric    -L../hdeem -lhdeem -lrt -o weird

mpi:
	mpic++ -O2 distributed.cpp -L../lib -lmericmpi -L../hdeem -lhdeem -lrt -o test_mpi

mpiicpc:
	mpiicpc -O2 distributed.cpp -L../lib -lmericmpi -L../hdeem -lhdeem -lrt -o test_mpi
