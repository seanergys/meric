INCLUDES=/projects/p_readex/cpufrequtils-icc/lib/
LIBPATH=/projects/p_readex/cpufrequtils-icc

all:
	./waf configure --cpp=icpc --flag="-L$(LIBPATH) -I$(INCLUDES)"
	make install

install:
	./waf install #-v

i:
	make install

ddt:
	make love
	./waf configure --cpp=icpc --flag="-L$(LIBPATH) -I$(INCLUDES) -O0"
	make install

love:
	./waf clean

clean:
	make love
