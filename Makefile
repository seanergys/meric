INCLUDES=-I/projects/p_readex/cpufrequtils-icc/lib/ \
	-I/sw/taurus/libraries/papi/5.4.1/include/
LIBPATH=-L/projects/p_readex/cpufrequtils-icc	\
	-L/sw/taurus/libraries/papi/5.4.1/lib/


all:
	./waf configure --cpp=icpc --flag="$(LIBPATH) $(INCLUDES)"
	make install

install:
	./waf install #-v

i:
	make install

ddt:
	make love
	./waf configure --cpp=icpc --flag="$(LIBPATH) $(INCLUDES) -O0 -g"
	make install

love:
	./waf clean

clean:
	make love
