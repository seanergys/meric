COMPILER=INTEL
#COMPILER=GCC

ifeq ($(COMPILER),INTEL)
	CC=icpc
	MPICC=mpiicpc
	OPENMP=-qopenmp
else #ifeq($(COMPILER),GCC)
	CC=g++
	MPICC=mpic++
	OPENMP=-fopenmp
endif

STARTbinary = energyMeasureStart
STOPbinary  = energyMeasureStop
################################################################################
X86ADAPT=-lx86_adapt

################################################################################
default: all

all: 
	$(CC) -O0 -g -std=c++11 -Wall -lrt -pedantic staticMERICtool.cpp $(X86ADAPT) -DSTART -o $(STARTbinary)
	$(CC) -O0 -g -std=c++11 -Wall -lrt -pedantic staticMERICtool.cpp $(X86ADAPT) -DSTOP  -o $(STOPbinary)
	mkdir -p $(HOME)/bin
	cp $(STARTbinary) $(HOME)/bin/
	cp $(STOPbinary) $(HOME)/bin/

######
love:
	make clean
	
clean: 
	rm -rf $(STARTbinary) $(STOPbinary)


 ######
run:
	./$(STARTbinary)
	sleep 5
	./$(STOPbinary)

