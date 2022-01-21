# MERIC library Makefile, works as user-friendly interface for compilation using waf
# To use the Makefile on your machine set INCLUDES_MERIC and LIBPATH_MERIC variables
# specifying the path to librararies and header files of the dependencies
################################################################################

INCLUDES_MERIC=
LIBPATH_MERIC=


ifeq ($(FC),ifort)
	FORTFLAGS=-module
else
	FORTFLAGS=-J
endif

all:
	make configure
	make install

configure:
	./waf configure --includes="$(INCLUDES_MERIC)" --libpaths="$(LIBPATH_MERIC)"

install:
	./waf install

i:
	make install

noopenmp:
	./waf configure --includes="$(INCLUDES_MERIC)" --libpaths="$(LIBPATH_MERIC)" --noopenmp
	make install

ddt:
	make love
	./waf configure --flag="-O0 -g" --includes="$(INCLUDES_MERIC)" --libpaths="$(LIBPATH_MERIC)" --verbose
	./waf install -v

scorep:
	make love
	./waf configure  --flag="-g" --scorep --includes="$(INCLUDES_MERIC)" --libpaths="$(LIBPATH_MERIC)" --verbose
	./waf install -j 1 -v

intel:
	./waf configure --cpp=icpc --mpi=mpiicpc --includes="$(INCLUDES_MERIC)" --libpaths="$(LIBPATH_MERIC)"
	make install

fortran:
	$(FC) -c include/meric_mod.F90 $(FORTFLAGS) include/ -o include/meric_mod.o

sbi:
	./waf configure --sbi --includes="$(INCLUDES_MERIC)" --libpaths="$(LIBPATH_MERIC)"
	make install

love:
	./waf distclean

clean:
	make love

