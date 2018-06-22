# To use the Makefile on your machine check paths settings and using correct 
# INCLUDES_MECHINE and LIBPATH_MACHINE

## TAURUS ######################################################################
PAPI_INC_TAURUS=-I/sw/taurus/libraries/papi/5.5.1/include/
PAPI_LIB_TAURUS=-L/sw/taurus/libraries/papi/5.5.1/lib/

INCLUDES_TAURUS= $(PAPI_INC_TAURUS) $(X86A_INC_TAURUS) $(CPUFREQ_INC_TAURUS) $(FREEIPMI_INC_TAURUS)
LIBPATH_TAURUS=  $(PAPI_LIB_TAURUS) $(X86A_LIB_TAURUS) $(CPUFREQ_LIB_TAURUS) $(FREEIPMI_LIB_TAURUS)

## SALOMON #####################################################################
#X86A_INC_SALOMON=-I$(HOME)/readex/x86adapt/x86_adapt/library/include/
#X86A_LIB_SALOMON=-L$(HOME)/readex/x86adapt/x86_adapt/build/

INCLUDES_SALOMON= $(PAPI_INC_SALOMON) $(X86A_INC_SALOMON) $(CPUFREQ_INC_SALOMON)
LIBPATH_SALOMON=  $(PAPI_LIB_SALOMON) $(X86A_LIB_SALOMON) $(CPUFREQ_LIB_SALOMON)

## JETSON ######################################################################
PAPI_INC_JETSON=-I/apps/papi/5.4.3/include
PAPI_LIB_JETSON=-L/apps/papi/5.4.3/lib

INCLUDES_JETSON= $(PAPI_INC_JETSON) $(X86A_INC_JETSON) $(CPUFREQ_INC_JETSON) $(FREEIPMI_INC_JETSON)
LIBPATH_JETSON=  $(PAPI_LIB_JETSON) $(X86A_LIB_JETSON) $(CPUFREQ_LIB_JETSON) $(FREEIPMI_LIB_JETSON)


INCLUDES_DAVIDE=-I/davide/home/userexternal/lriha000/json/single_include/ -I/davide/home/userexternal/lriha000/rest-client/include/
LIBPATH_DAVIDE=-L/davide/home/userexternal/lriha000/rest-client/libs/ -lrestclient-cpp

################################################################################
INCLUDES_MERIC= $(INCLUDES_DAVIDE)
LIBPATH_MERIC= $(LIBPATH_DAVIDE)
unexport CXX

all:
	make configure
	make install

configure:
	./waf configure --cpp=icpc --mpi=mpiicpc --flag="$(LIBPATH_MERIC) $(INCLUDES_MERIC)" # -DVERBOSE"

install:
	./waf install  #-v

i:
	make install

ddt:
	make love
	./waf configure --cpp=icpc --mpi=mpiicpc --flag="$(LIBPATH_MERIC) $(INCLUDES_MERIC) -O0 -g" --verbose
	make install

gcc:
	./waf configure --withgcc --cpp=g++ --flag="$(LIBPATH_MERIC) $(INCLUDES_MERIC) -DVERBOSE"
	make install

arm:
	./waf configure --withgcc --cpp=g++ --flag="$(LIBPATH_JETSON) $(INCLUDES_JETSON)" --dummy
	make install

love:
	./waf clean

clean:
	make love

