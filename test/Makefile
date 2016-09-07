
all:
	c++    -O2 shared.cpp      -L../lib -lmeric    -L../hdeem -lhdeem -o test
	mpic++ -O2 distributed.cpp -L../lib -lmericmpi -L../hdeem -lhdeem -o test_mpi
