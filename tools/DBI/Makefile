# Makefile for DBI tools
# DBI libraries require to specify their location
#	${DYNAMORIO_HOME}
#	${PIN_ROOT}
#	${DINST_HOME} + ${BOOST_ROOT}

drio:
	g++ -std=c++11 -fPIC -shared -lgcc -DLINUX -DX86_64 -I${DYNAMORIO_HOME}/include -I${DYNAMORIO_HOME}/ext/include dr_profile.cpp -L${DYNAMORIO_HOME}/ext/lib64/release/ -ldrmgr -ldrwrap -ldrsyms -o librio.so

pin:
	g++ -g -Wall -Werror -Wno-unknown-pragmas -D__PIN__=1 -DPIN_CRT=1 -fno-stack-protector -fno-exceptions -funwind-tables -fasynchronous-unwind-tables -fno-rtti -DTARGET_IA32E -DHOST_IA32E -fPIC -DTARGET_LINUX -fabi-version=2  -I${PIN_ROOT}/source/include/pin -I${PIN_ROOT}/source/include/pin/gen -isystem ${PIN_ROOT}/extras/stlport/include -isystem ${PIN_ROOT}/extras/libstdc++/include -isystem ${PIN_ROOT}/extras/crt/include -isystem ${PIN_ROOT}/extras/crt/include/arch-x86_64 -isystem ${PIN_ROOT}/extras/crt/include/kernel/uapi -isystem ${PIN_ROOT}/extras/crt/include/kernel/uapi/asm-x86 -I${PIN_ROOT}/extras/components/include -I${PIN_ROOT}/extras/xed-intel64/include/xed -I${PIN_ROOT}/source/tools/InstLib -O3 -fomit-frame-pointer -fno-strict-aliasing   -c -o obj-intel64/pin_profile.o pin_profile.cpp
	g++ -shared -Wl,--hash-style=sysv ${PIN_ROOT}/intel64/runtime/pincrt/crtbeginS.o -Wl,-Bsymbolic -Wl,--version-script=${PIN_ROOT}/source/include/pin/pintool.ver -fabi-version=2    -o obj-intel64/pin_profile.so obj-intel64/pin_profile.o  -L${PIN_ROOT}/intel64/runtime/pincrt -L${PIN_ROOT}/intel64/lib -L${PIN_ROOT}/intel64/lib-ext -L${PIN_ROOT}/extras/xed-intel64/lib -lpin -lxed ${PIN_ROOT}/intel64/runtime/pincrt/crtendS.o -lpin3dwarf  -ldl-dynamic -nostdlib -lstlport-dynamic -lm-dynamic -lc-dynamic -lunwind-dynamic
	# if you copy makefile and makefile.rules from ${PIN_ROOT}/source/tools/MyPinTool/ and edit TEST_TOOL_ROOTS variable to name of the compiled tool then you may use following command instead of these ugly two
	# make PIN_ROOT=${PIN_ROOT} obj-intel64/pin_profile.so

dinst:
	g++ -g -Wall -std=c++11 -L/usr/local/lib -o dinst_profile dinst_profile.cpp \
	-ldyninstAPI -I${DINST_HOME}/dyninstAPI/h/ -L${DINST_HOME}/build/dyninstAPI/ \
	-I${DINST_HOME}/common/h/ -I${DINST_HOME}/build/common/h/ -I${DINST_HOME}/symtabAPI/h/ -I${DINST_HOME}/proccontrol/h/ -I${DINST_HOME}/instructionAPI/h/ -I${DINST_HOME}/dataflowAPI/h/

sbi:
	g++ -g -Wall -std=c++11 -L/usr/local/lib -o dinst_instrument dinst_instrument.cpp \
	-ldyninstAPI -I${DINST_HOME}/dyninstAPI/h/ -L${DINST_HOME}/build/dyninstAPI/ \
	-I${DINST_HOME}/common/h/ -I${DINST_HOME}/build/common/h/ -I${DINST_HOME}/symtabAPI/h/ -I${DINST_HOME}/proccontrol/h/ -I${DINST_HOME}/instructionAPI/h/ -I${DINST_HOME}/dataflowAPI/h/ \
	-I${DINST_HOME}/instructionAPI/h/ -L${DINST_HOME}/build/instructionAPI/ -linstructionAPI


################################################################################
################################################################################

# run command name copy the compilation name and adds "run" prefix
# run commands run ./test application to analyze - edit it if needed, this is just an example case

rundrio:
	rm -f uniquefile-*
	rm -f lock.f
	${DYNAMORIO_HOME}/bin64/drrun -stderr_mask 15 -c librio.so -- ./test

runpin:
	${PIN_ROOT}/pin -t obj-intel64/pin_profile.so -- ./test

rundinst:
	./dinst_profile ../../test/test

runsbi:
	./dinst_instrument ../../test/test


