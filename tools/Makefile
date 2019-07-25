NUMA=-I${HOME}/Software/numactl/usr/local/include/ -L${HOME}/Software/numactl/usr/local/lib/ -lnuma -DHAVE_NUMA_H
MSR=-L${HOME}/Software/libmsr/lib/ -lmsr
MERIC=-L../lib/ -lmeric

all:
	icpc -std=c++11 ${NUMA} ${MSR} ${MERIC} wrapper.cpp -o wrapper
