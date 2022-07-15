
from waflib import Logs
import os, shutil, copy, re

def getCPUvendor():
	vendor = ""
	cpuinfo = open("/proc/cpuinfo",'r')
	line = cpuinfo.readline()
	while line != '' and line[:9] != "vendor_id":
		line = cpuinfo.readline()
	if re.search("Intel",line):
		vendor = "_INTEL_"
	elif re.search("AMD",line):
		vendor = "_AMD_"
	return vendor
	
def configure(ctx):
    # C++ compiler
    if (ctx.options.cpp == ""):
        ctx.add_os_flags('CC')
        ctx.add_os_flags('CXX')
        
        if ('CXX' in ctx.env):
            ctx.options.cpp=ctx.env['CXX'][0].split('/')[-1]
        elif ('CC' in ctx.env):
            if (ctx.env['CC'][0].split('/')[-1] == 'icc'): # Intel
                ctx.options.cpp="icpc"
            elif (ctx.env['CC'][0].split('/')[-1] == 'gcc'): # GCC
                ctx.options.cpp="g++"
            elif (ctx.env['CC'][0].split('/')[-1] == 'clang'): # Clang
                ctx.options.cpp="clang++"
            # Cray (cc), Fujitsu (FCC), ARM (armclang++)
        else:
            ctx.options.cpp="g++" # default C++ compiler

    try:
        ctx.load(ctx.options.cpp)
    except:
        if ('CXX' in ctx.env):
            if (ctx.env['CXX'][0].endswith('clang++')):
                ctx.load("clang++")
            else:
                ctx.load("g++")
        else:
            ctx.fatal("C++ compiler not foud, set a correct one by '--cpp=' parameter")
    
    # MPI compiler
    if (ctx.options.mpi == ""):
        ctx.add_os_flags('MPICXX')
        if ('MPICXX' in ctx.env):
            ctx.options.mpi=ctx.env['MPICXX'][0].split('/')[-1]
        elif (ctx.options.cpp=="icpc"):
            ctx.options.mpi="mpiicpc"
        else:
            ctx.options.mpi="mpic++" # default MPI compiler
    
    if ctx.options.flags:
        ctx.env.append_unique("CXXFLAGS", ctx.options.flags.split( ))
    ctx.env.append_unique("CXXFLAGS", [ "-std=c++11", "-fPIC"])
    if (ctx.options.includes):
        for path in ctx.options.includes.split():
            if path[0] == '-':
                ctx.env.append_unique("CXXFLAGS",path)
            else:
                ctx.env.append_unique("CXXFLAGS","-I"+path)
    if (ctx.options.libpaths):
        for path in ctx.options.libpaths.split():
            if path[0] == '-':
                ctx.env.append_unique("LINKFLAGS", path)
            else:
                ctx.env.append_unique("LINKFLAGS", "-L"+path)
    
    if ctx.options.noopenmp:
        ctx.env.append_unique("NOOPENMP", "NOOPENMP" ) # without openMP
    else:
        ctx.env.append_unique("DEFINES", [ "HAVE_OMP_H=1" ]) # with openMP
        if ctx.options.cpp == "icpc":
            ctx.env.append_unique("CXXFLAGS",  [ "-qopenmp" ])
            ctx.env.append_unique("LINKFLAGS", [ "-qopenmp" ])
        elif ctx.options.cpp == "FCC":
            ctx.env.append_unique("CXXFLAGS",  [ "-Kopenmp" ])
            ctx.env.append_unique("LINKFLAGS", [ "-Kopenmp" ])
        else:
            ctx.env.append_unique("CXXFLAGS",  [ "-fopenmp" ])
            ctx.env.append_unique("LINKFLAGS", [ "-fopenmp" ])

    vendor = getCPUvendor()
    if vendor != "":
        ctx.env.append_unique("DEFINES", [ vendor ])
    	
    SBImandatory = False
    MSRSAFEavailable = False
    OCCavailable = False
    # check for msr_safe
    if os.access("/dev/cpu/0/msr_safe", os.R_OK) and os.access("/dev/cpu/0/msr_safe", os.W_OK):
        MSRSAFEavailable = True
        ctx.env.append_unique("DEFINES", [ "HAVE_MSR_SAFE=1" ])
    
    # check for OCC
    if os.access("/sys/firmware/opal/exports/occ_inband_sensors", os.R_OK):
        OCCavailable = True
        ctx.env.append_unique("DEFINES", [ "HAVE_OCC=1" ])
    
    # check for HW-tuning software
    if not ctx.options.sbi:
        if ((not ctx.check_cc(header_name="variorum.h", lib="variorum", uselib_store="variorum", mandatory=False)) and (not ctx.check_cc(header_name="msr_core.h", lib="msr", uselib_store="msr", mandatory=False)) and (not ctx.check_cc(header_name="geopm.h", lib="geopm", uselib_store="geopm", execute=False, mandatory=False)) and (not ctx.check_cc(header_name="x86_adapt.h", lib="x86_adapt", uselib_store="x86_adapt", execute=False, mandatory=False)) and (not MSRSAFEavailable)):
            if ctx.check_cxx(lib="cpufreq", mandatory=False):
                ctx.env.append_unique("DEFINES", [ "HAVE_CPUFREQ_H=1" ])
                ctx.env.append_unique("LINKFLAGS", [ "-lcpufreq" ])
        ctx.check_cc(header_name="nvml.h", lib="nvidia-ml", uselib_store="nvml", execute=False, mandatory=False)
        ctx.check_cc(header_name="stdint.h e_smi/e_smi.h", define_name="HAVE_E_SMI_E_SMI_H", lib="e_smi64", uselib_store="esmi", execute=False, mandatory=False, msg="Checking for header e_smi/e_smi.h")
        ctx.check_cc(header_name="numa.h", lib="numa", uselib_store="numa", execute=False, mandatory=False)
#        ctx.check_cc(header_name="cilk/cilk_api.h", execute=False, mandatory=False)
        
        # check for energy measurment systems
        if (not ctx.check_cc(header_name="hdeem.h", lib="hdeem", uselib_store="hdeem", execute=False, mandatory=False) or not ctx.check_cc(header_name="freeipmi/freeipmi.h", lib="freeipmi", uselib_store="freeipmi", execute=False, mandatory=False)):
            # missing include <freeipmi/freeipmi.h>
            ctx.env.append_unique("CXXFLAGS", [ "-I"+ctx.path.abspath()+"/include/"])
        ctx.check_cc(header_name="restclient-cpp/restclient.h", execute=False, mandatory=False)
        
        #check for libs to read performance counters
        ctx.check_cc(header_name="papi.h", execute=False, mandatory=False)
        ctx.check_cc(header_name="linux/perf_event.h", execute=False, mandatory=False)
        
        ctx.check_cc(header_name="hdf5.h", execute=False, mandatory=False)
    
    # SBI tool
    else:
        SBImandatory = True
        if ctx.options.cpp !="g++":
            ctx.fatal("Dyninst (SBI) tools require compilation with GNU compiler, "+ctx.options.cpp+" used instead")
    if ctx.options.cpp =="g++" and ctx.check_cc(header_name="BPatch.h", lib=["dyninstAPI", "instructionAPI"], uselib_store="dyninst", mandatory=SBImandatory) and ctx.check_cc(header_name="boost/thread.hpp", lib=["boost_thread", "boost_system", "boost_date_time"], uselib_store="boost", mandatory=SBImandatory):
        if ctx.options.sbi:
            ctx.env.append_unique("SBI", "SBI_ONLY")
            return
        else:
            ctx.env.append_unique("SBI", "SBI")
    
    configFile = open(ctx.path.abspath()+"/build/MERICconf.log", "w")
    for dep in ctx.env.DEFINES:
        if dep[:5] == "HAVE_":
            configFile.write(dep[5:-2]+"\n")
    configFile.close()
    
    if os.path.exists(ctx.path.abspath()+"/build/systemDefault.json"):
        os.remove(ctx.path.abspath()+"/build/systemDefault.json")
    
    ctx.env.append_unique("DEFINES", [ "MERIC_PATH=\""+ctx.path.abspath()+"\""])
    if ctx.options.debug:
        ctx.env.append_unique("DEFINES", [ "__MERIC_DEBUG__" ])
    if ctx.options.verbose:
        ctx.env.append_unique("DEFINES", [ "VERBOSE" ])
    
    if ctx.options.noopenmp:
        ctx.find_program(ctx.options.mpi, var="MPICXX", mandatory=True)
    else:
        ctx.find_program(ctx.options.mpi, var="MPICXX", mandatory=False)
    
    if ctx.options.scorep:
        ctx.env.CXXFLAGS.insert(0, ctx.env.CXX[0])
        ctx.env.LINKFLAGS.insert(0, ctx.env.CXX[0])
        ctx.env.CXX[0] = "scorep"
    
    if ctx.env.MPICXX:
        ctx.setenv("mpi", ctx.env.derive())
        if ctx.options.scorep:
            ctx.env.CXXFLAGS = [ctx.env.MPICXX[0]] + ctx.env.CXXFLAGS[1:]
            ctx.env.LINKFLAGS = [ctx.env.MPICXX[0]] + ctx.env.LINKFLAGS[1:]
        else:
            ctx.env.CXX = list(ctx.env.MPICXX)
            ctx.env.LINK_CXX = ctx.env.CXX
        ctx.env.append_unique("DEFINES", [ "HAVE_MPI_H=1" ])
    
    ctx.msg("Compiler flags", " ".join(ctx.env["CXXFLAGS"]))
    if not ctx.env.MPICXX:
        Logs.warn("MPI compiler not found. Only shared memory parallelization is supported")
    env = ctx.env
    ctx.setenv('start', env)
    ctx.env.append_unique("DEFINES", [ "START" ])
    ctx.setenv('stop', env)
    ctx.env.append_unique("DEFINES", [ "STOP" ])
#    print(ctx.env.CXX)
#    print(ctx.options)
#    print(ctx.env)
#    print(ctx.all_envs['mpi'])
#    print(ctx.all_envs['start'])
#    print(ctx.all_envs['stop'])

################################################################################
def build(ctx):
    # SBI TOOL
    if ctx.env.SBI:
        ctx.program(
            source       = ctx.path.ant_glob('src/tools/SBI/dinst_instrument.cpp'),
            target       = "dinst_instrument",
            use          = "boost dyninst",
            install_path = ctx.path.abspath()+"/tools"
        )
        ctx.program(
            source       = ctx.path.ant_glob('src/tools/SBI/dinst_profile.cpp'),
            target       = "dinst_profile",
            use          = "boost dyninst",
            install_path = ctx.path.abspath()+"/tools"
        )
    if "SBI_ONLY" in ctx.env.SBI:
        return
    
    ctx.objects(
        source =
            ctx.path.ant_glob('src/basis/*.cpp') +
            ctx.path.ant_glob('src/store/*.cpp') +
            ctx.path.ant_glob('src/meric/*.cpp') +
            ctx.path.ant_glob('src/wrapper/*.cpp'),
        target = "basis"
    )
    
    # STATIC MERIC TOOL
    ctx.program(
        source= 
            ctx.path.ant_glob('src/tools/staticMERICtool/staticMERICtool.cpp') +
            ctx.path.ant_glob('src/wrapper/environmentwrapper.cpp') +
            ctx.path.ant_glob('src/wrapper/raplwrapper.cpp') +
            ctx.path.ant_glob('src/wrapper/nvmlwrapper.cpp') +
            ctx.path.ant_glob('src/basis/json.cpp'),
        target="energyMeasureStart",
        use="msr x86_adapt numa geopm nvml esmi",
        env=ctx.all_envs['start'].derive(),
        install_path = ctx.path.abspath()+"/tools/"
    )
    ctx.program(
        source= 
            ctx.path.ant_glob('src/tools/staticMERICtool/staticMERICtool.cpp') +
            ctx.path.ant_glob('src/wrapper/environmentwrapper.cpp') +
            ctx.path.ant_glob('src/wrapper/raplwrapper.cpp') +
            ctx.path.ant_glob('src/wrapper/nvmlwrapper.cpp') +
            ctx.path.ant_glob('src/basis/json.cpp'),
        target="energyMeasureStop",
        use="msr x86_adapt numa geopm nvml esmi",
        env=ctx.all_envs['stop'].derive(),
        install_path = ctx.path.abspath()+"/tools/"
    )
    shutil.copy("src/tools/staticMERICtool/multiNodeStaticMeasureStart.sh" , ctx.path.abspath()+"/tools/")
    shutil.copy("src/tools/staticMERICtool/multiNodeStaticMeasureStop.sh" , ctx.path.abspath()+"/tools/")
    
    # MERICwrapper
    ctx.program(
        source= 
            ctx.path.ant_glob('src/tools/wrapper/wrapper.cpp') +
            ctx.path.ant_glob('src/wrapper/environmentwrapper.cpp') +
            ctx.path.ant_glob('src/wrapper/raplwrapper.cpp') +
            ctx.path.ant_glob('src/wrapper/nvmlwrapper.cpp') +
            ctx.path.ant_glob('src/wrapper/hdeemwrapper.cpp') +
            ctx.path.ant_glob('src/wrapper/thunderwrapper.cpp') +
            ctx.path.ant_glob('src/wrapper/jetsonwrapper.cpp') +
            ctx.path.ant_glob('src/wrapper/davidewrapper.cpp') +
            ctx.path.ant_glob('src/basis/json.cpp'),
        target="wrapper",
        use="msr x86_adapt numa geopm nvml hdeem freeipmi esmi",
        install_path = ctx.path.abspath()+"/tools/"
    )
    
    # SYSTEM INFO TOOL
    ctx.program(
        source=
            ctx.path.ant_glob('src/tools/systemInfo.cpp') +
            ctx.path.ant_glob('src/wrapper/environmentwrapper.cpp') +
            ctx.path.ant_glob('src/wrapper/raplwrapper.cpp') +
            ctx.path.ant_glob('src/wrapper/nvmlwrapper.cpp') +
            ctx.path.ant_glob('src/basis/json.cpp'),
        target="systemInfo",
        use="msr x86_adapt numa geopm nvml esmi",
        install_path = ctx.path.abspath()+"/tools"
    )
    
    if not ctx.env.NOOPENMP:
        #MERIC
        ctx.stlib(
            source       = ctx.path.ant_glob('src/meric/shared/*.cpp'),
            target       = "meric",
            use          = "basis",
            install_path = ctx.path.abspath()+"/lib"
        )
        
        ctx.shlib(
            source       = ctx.path.ant_glob('src/meric/shared/*.cpp'),
            target       = "meric",
            use          = "basis",
            install_path = ctx.path.abspath()+"/lib"
        )
        
        #TIMEPROF
        ctx.stlib(
            source       = ctx.path.ant_glob('src/timeprof/*.cpp'),
            target       = "timeprof",
            use          = "basis",
            install_path = ctx.path.abspath()+"/lib"
        )
        
        ctx.shlib(
            source       = ctx.path.ant_glob('src/timeprof/*.cpp'),
            target       = "timeprof",
            use          = "basis",
            install_path = ctx.path.abspath()+"/lib"
        )
        suffix=""
    else:
        suffix="only"
    
    #MERIC MPI
    if not ctx.env.MPICXX:
        return
    
    ctx.all_envs["mpi"].LINK_CXX = ctx.all_envs["mpi"].CXX
    
    ctx.stlib(
        source       = ctx.path.ant_glob('src/meric/distributed/*.cpp'),
        target       = "mericmpi"+suffix,
        use          = "basis",
        install_path = ctx.path.abspath()+"/lib",
        env          = ctx.all_envs["mpi"]
    )
    
    ctx.shlib(
        source       = ctx.path.ant_glob('src/meric/distributed/*.cpp'),
        target       = "mericmpi"+suffix,
        use          = "basis",
        install_path = ctx.path.abspath()+"/lib",
        env          = ctx.all_envs["mpi"]
    )
    
    #TIMEPROF MPI
    ctx.stlib(
        source       = ctx.path.ant_glob('src/timeprof/*.cpp'),
        target       = "timeprofmpi"+suffix,
        use          = "basis",
        install_path = ctx.path.abspath()+"/lib",
        env          = ctx.all_envs["mpi"]
    )
    
    ctx.shlib(
        source       = ctx.path.ant_glob('src/timeprof/*.cpp'),
        target       = "timeprofmpi"+suffix,
        use          = "basis",
        install_path = ctx.path.abspath()+"/lib",
        env          = ctx.all_envs["mpi"]
    )

################################################################################
def options(opt):
    opt.add_option(
        "--verbose",
        action="store_true",
        default=False,
        help="Verbose mode"
    )
    
    opt.add_option(
        "--sbi",
        action="store_true",
        default=False,
        help="Compile SBI tools only"
    )
    
    opt.add_option(
        "--dummy",
        action="store_true",
        default=False,
        help="Use dummy HDEEM wrapper"
    )
    
    opt.add_option(
        "--debug",
        action="store_true",
        default=False,
        help="Activate debug output"
    )
    
    opt.add_option(
        "--scorep",
        action="store_true",
        default=False,
        help="Compile MERIC with Score-P"
    )
    
    opt.add_option(
        "--cpp",
        action="store",
        default="",
        help="C++ compiler, default GNU"
    )
    
    opt.add_option(
        "--mpi",
        action="store",
        default="",
        help="MPI C++ compiler, default mpic++"
    )
    
    opt.add_option(
        "--noopenmp",
        action="store_true",
        default=False,
        help="Compilation of MPI version without OpenMP"
    )
    
    opt.add_option(
        "--flags",
        action="store",
        default="-O2",
        help="Compiler flags (-std=c++11, -fPIC and -qopenmp/-fopenmp are default)"
    )
    
    opt.add_option("--includes",
        action="store",
        type="string",
        metavar="INCLUDE_PATHS",
        default="",
        help="List of paths to include")
    
    opt.add_option("--libpaths",
        action="store",
        type="string",
        metavar="LIB_PATHS",
        default="",
        help="List of paths to libraries")

