ifeq ($(CXX),)
	CXX=g++
endif

ifeq ($(CC),)
	CC=gcc
endif

ifeq ($(shell echo ${CXX} | awk '{n=split($$1,A,"/"); print A[n]}'),icpc)
	OPENMP=-qopenmp
else ifeq ($(shell echo ${CXX} | awk '{n=split($$1,A,"/"); print A[n]}'),FCC)
	OPENMP=-Kopenmp
else
	OPENMP=-fopenmp
endif


################################################################################
MERIC=-L../../../lib -I../../../include
LIBS=-lrt $(MERIC) $(OPENMP) -lnetsnmp $(CXX_FLAGS)
BINARIES=master
################################################################################

default: all

all: master

MASTER_SRCS=master.cpp ip_utils.cpp slave_script.cpp signal_server.cpp

master: $(MASTER_SRCS)
	$(CXX) -O2 $(MASTER_SRCS) $(LIBS) -lmeric -lstdc++ -lm -o master

clean: 
	rm -rf ${BINARIES} slave.sh *.csv *.o *~ *.out *.qsub
