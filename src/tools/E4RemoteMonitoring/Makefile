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
#HDEEM=-lhdeem -lfreeipmi
#X86ADAPT=-lx86_adapt
#CPUFREQ=--lcpufreq
#CPUFREQ=-L/lib64/ -lcpupower
#PAPI=-lpapi
#RESTCLIENT= -lrestclient-cpp
#NUMA=-lnuma
#NVML=-lnvidia-ml
#ESMI=-le_smi64
SNMP= -lnetsnmp
#LIBMSR=-lmsr -L${LIBMSR_PATH}/lib -I${LIBMSR_PATH}/include
################################################################################
MERIC=-L../../../lib -I../../../include
LIBS=-lrt $(MERIC) $(OPENMP) $(HDEEM) $(X86ADAPT) $(CPUFREQ) $(PAPI) $(RESTCLIENT) $(NUMA) $(NVML) $(ESMI) $(LIBMSR) $(SNMP) $(CXX_FLAGS)
BINARIES=master slave.sh
################################################################################
MKL_PATHS=
################################################################################

default: all

all: master

 ######

MASTER_SRCS=master.cpp ip_utils.cpp slave_script.cpp signal_server.cpp

master: $(MASTER_SRCS)
	$(CXX) -O2 $(MASTER_SRCS) $(LIBS) -lmeric -lstdc++ -lm -o master

clean: 
	rm -rf ${BINARIES} *.csv *.o *~ *.out *.qsub
