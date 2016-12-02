INCLUDES=-I/projects/p_readex/cpufrequtils-icc/lib/ \
	-I/sw/taurus/libraries/papi/5.4.1/include/	\
	-I$(HOME)/readex/x86adapt/x86_adapt/library/include/
LIBPATH=-L/projects/p_readex/cpufrequtils-icc	\
	-L/sw/taurus/libraries/papi/5.4.1/lib/	\
	-L$(HOME)/readex/x86adapt/x86_adapt/build/



all:
	make configure
	make install

configure:
	./waf configure --cpp=icpc --mpi=mpiicpc --flag="$(LIBPATH) $(INCLUDES)"

install:
	./waf install #-v

i:
	make install

ddt:
	make love
	./waf configure --cpp=icpc --flag="$(LIBPATH) $(INCLUDES) -O0 -g"
	make install

dummy:
	./waf configure --cpp=icpc --flag="$(LIBPATH) $(INCLUDES) -O0 -g" --dummy
	make install

love:
	./waf clean

clean:
	make love
