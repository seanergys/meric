ifeq ($(shell echo ${CXX} | awk '{n=split($$1,A,"/"); print A[n]}'),g++)
	OPENMP=-fopenmp
else
	OPENMP=-qopenmp
endif

all:
	${CXX} -std=c++11 ${OPENMP} -g main.cpp ../../basis/json.cpp ../../wrapper/environmentwrapper.cpp ../../wrapper/hdeemwrapper.cpp ../../wrapper/raplwrapper.cpp ../../wrapper/thunderwrapper.cpp ../../wrapper/davidewrapper.cpp ../../wrapper/jetsonwrapper.cpp ../../wrapper/nvmlwrapper.cpp ../../wrapper/occwrapper.cpp -I../../../include/ -DMERIC_PATH="\"/home/vys0053/Software/meric_dev/\"" -DHAVE_OMP_H -D_AMD_

run:
	./a.out -a /home/vys0053/analysis/bddcml/BDDCML-measurement2/ -m 1
	./a.out -s inputConfig.json -m 1 -- ${HOME}/readex/meric/test/test
