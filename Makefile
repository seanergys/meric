#PAPI - taurus
PAPI_INC=-I/sw/taurus/libraries/papi/5.4.1/include/
PAPI_LIB=-L/sw/taurus/libraries/papi/5.4.1/lib/
#CPUFREQ - taurus
CPUFREQ_INC=-I/projects/p_readex/cpufrequtils-icc/lib/
CPUFREQ_LIB=-L/projects/p_readex/cpufrequtils-icc
#X86_ADAPT - salomon
X86A_INC=-I$(HOME)/readex/x86adapt/x86_adapt/library/include/
X86A_LIB=-L$(HOME)/readex/x86adapt/x86_adapt/build/
#FREEIPMI - salomon
FREEIPMI_INC=-I$(HOME)/readex/meric/hdeem/freeipmi-1.5.5/libfreeipmi/include/
FREEIPMI_LIB=


INCLUDES= $(PAPI_INC) $(X86A_INC) $(CPUFREQ_INC) $(FREEIPMI_INC)
LIBPATH=  $(PAPI_LIB) $(X86A_LIB) $(CPUFREQ_LIB) $(FREEIPMI_LIB)


all:
	make configure
	make install

configure:
	./waf configure --cpp=icpc --mpi=mpiicpc --flag="$(LIBPATH) $(INCLUDES)" # -DVERBOSE"

install:
	./waf install #-v

i:
	make install

ddt:
	make love
	./waf configure --cpp=icpc --mpi=mpiicpc --flag="$(LIBPATH) $(INCLUDES) -O0 -g" --verbose
	make install

dummy:
	./waf configure --cpp=icpc --mpi=mpiicpc --flag="$(LIBPATH) $(INCLUDES) -g" --dummy
	make install

gcc:
	./waf configure --cpp=g++ --flag="$(LIBPATH) $(INCLUDES)"
	make install
love:
	./waf clean

clean:
	make love
