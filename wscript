
from waflib import Logs
import os

def configure(ctx):
    ctx.load(ctx.options.cpp)

    if ctx.options.flags:
        ctx.env.append_unique("CXXFLAGS", ctx.options.flags.split( ))
    ctx.env.append_unique("CXXFLAGS", [ "-std=c++11", "-fPIC"])                 #no openMP
    if (ctx.options.includes):
        for path in ctx.options.includes.split():
            if len(path) > 2 and path[0:2] == "-I":
                ctx.env.append_unique("CXXFLAGS",path)
            else:
                ctx.env.append_unique("CXXFLAGS","-I"+path)
    if (ctx.options.libpaths):
        for path in ctx.options.libpaths.split():
            if len(path) > 2 and path[0:2] == "-L":
                ctx.env.append_unique("LINKFLAGS", path)
            else:
                ctx.env.append_unique("LINKFLAGS", "-L"+path)

    if ctx.options.withgcc:
        if not ctx.options.noopenmp:
            ctx.env.append_unique("CXXFLAGS",  [ "-fopenmp" ])                  #GCC
            ctx.env.append_unique("LINKFLAGS", [ "-fopenmp" ])
            ctx.env.append_unique("DEFINES", [ "HAVE_OMP_H=1" ])                #with openMP
        else:
            ctx.env.append_unique("NOOPENMP", "NOOPENMP" )
    else:
        if not ctx.options.noopenmp:
            ctx.env.append_unique("CXXFLAGS",  [ "-qopenmp" ])                  #Intel
            ctx.env.append_unique("LINKFLAGS", [ "-qopenmp" ])
            ctx.env.append_unique("DEFINES", [ "HAVE_OMP_H=1" ])                #with openMP
        else:
            ctx.env.append_unique("NOOPENMP", "NOOPENMP" )
   
   # check for HW-tuning software
    NUMA_MANDATORY=True
    NUMA_errmsg="not found"
    if (ctx.check_cc(header_name="variorum.h", lib="variorum", uselib_store="variorum", mandatory=False)):
    # When variorum will prvide the information about the topology, I may check just for the msr-safe !!!
        if (ctx.check_cc(header_name="msr_core.h", lib="msr", uselib_store="msr", mandatory=False) or ctx.check_cc(header_name="geopm.h", lib="geopm", uselib_store="geopm", execute=False, mandatory=False) or ctx.check_cc(header_name="x86_adapt.h", lib="x86_adapt", uselib_store="x86_adapt", execute=False, mandatory=False)):
            NUMA_MANDATORY=False
    elif (ctx.check_cc(header_name="msr_core.h", lib="msr", uselib_store="msr", mandatory=False)):
        NUMA_MANDATORY=False
    elif (ctx.check_cc(header_name="geopm.h", lib="geopm", uselib_store="geopm", execute=False, mandatory=False)):
        NUMA_MANDATORY=False
    elif (ctx.check_cc(header_name="x86_adapt.h", lib="x86_adapt", uselib_store="x86_adapt", execute=False, mandatory=False)):
        NUMA_MANDATORY=False
    else:
        if ctx.check_cxx(lib="cpufreq", mandatory=False):
            ctx.env.append_unique("DEFINES", [ "HAVE_CPUFREQ_H=1" ])
            ctx.env.append_unique("LINKFLAGS", [ "-lcpufreq" ])
    if (NUMA_MANDATORY):
        NUMA_errmsg += " - When x86_adapt, msr-safe, geopm are missing, numactl is mandatory!"
    ctx.check_cc(header_name="numa.h", lib="numa", uselib_store="numa", execute=False, mandatory=NUMA_MANDATORY, errmsg=NUMA_errmsg)
    
    # check for energy measurment systems
    if (not ctx.check_cc(header_name="hdeem.h", execute=False, mandatory=False)):
        # missing include <freeipmi/freeipmi.h>
        ctx.env.append_unique("CXXFLAGS", [ "-I"+ctx.path.abspath()+"/include/"])
    ctx.check_cc(header_name="restclient-cpp/restclient.h", execute=False, mandatory=False)
    
    #check for libs to read performance counters
    ctx.check_cc(header_name="papi.h", execute=False, mandatory=False)
    ctx.check_cc(header_name="linux/perf_event.h", execute=False, mandatory=False)

    ctx.check_cc(header_name="hdf5.h", execute=False, mandatory=False)

#    ctx.check_cc(header_name="cilk/cilk_api.h", execute=False, mandatory=False)

    configFile = open(ctx.path.abspath()+"/build/MERICconf.log", "w")
    for dep in ctx.env.DEFINES:
        if dep[:5] == "HAVE_":
            configFile.write(dep[5:-4]+"\n")
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
    if ctx.env.MPICXX:
        ctx.setenv("mpi", ctx.env.derive())
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
#    print ctx.env
#    print ctx.all_envs['start']
#    print ctx.all_envs['stop']

################################################################################
def build(ctx):
    # STATIC MERIC TOOL
    ctx.program(
        source= 
            ctx.path.ant_glob('tools/staticMERICtool/staticMERICtool.cpp') +
            ctx.path.ant_glob('src/wrapper/environmentwrapper.cpp') +
            ctx.path.ant_glob('src/wrapper/raplwrapper.cpp') +
            ctx.path.ant_glob('src/basis/json.cpp'),
        target="energyMeasureStart",
        use="msr x86_adapt numa geopm",
        env=ctx.all_envs['start'].derive(),
        install_path = ctx.path.abspath()+"/tools/staticMERICtool"
    )
    ctx.program(
        source= 
            ctx.path.ant_glob('tools/staticMERICtool/staticMERICtool.cpp') +
            ctx.path.ant_glob('src/wrapper/environmentwrapper.cpp') +
            ctx.path.ant_glob('src/wrapper/raplwrapper.cpp') +
            ctx.path.ant_glob('src/basis/json.cpp'),
        target="energyMeasureStop",
        use="msr x86_adapt numa geopm",
        env=ctx.all_envs['stop'].derive(),
        install_path = ctx.path.abspath()+"/tools/staticMERICtool"
    )

    # MERICwrapper
    ctx.program(
        source= 
            ctx.path.ant_glob('tools/wrapper.cpp') +
            ctx.path.ant_glob('src/wrapper/environmentwrapper.cpp') +
            ctx.path.ant_glob('src/wrapper/raplwrapper.cpp') +
            ctx.path.ant_glob('src/basis/json.cpp'),
        target="wrapper",
        use="msr x86_adapt numa geopm",
        install_path = ctx.path.abspath()+"/tools/"
    )
    
    # SYSTEM INFO TOOL    
    ctx.program(
        source=
            ctx.path.ant_glob('tools/systemInfo.cpp') +
            ctx.path.ant_glob('src/wrapper/environmentwrapper.cpp') +
            ctx.path.ant_glob('src/wrapper/raplwrapper.cpp') +
            ctx.path.ant_glob('src/basis/json.cpp'),
        target="systemInfo",
        use="msr x86_adapt numa geopm",
        install_path = ctx.path.abspath()+"/tools"
    )

    ctx.objects(
        source =
            ctx.path.ant_glob('src/basis/*.cpp') +
            ctx.path.ant_glob('src/store/*.cpp') +
            ctx.path.ant_glob('src/meric/*.cpp') +
            ctx.path.ant_glob('src/wrapper/*.cpp'),
        target = "basis"
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
        "--withgcc",
        action="store_true",
        default=False,
        help="use GCC instead of Intel compiler"
    )

    opt.add_option(
        "--verbose",
        action="store_true",
        default=False,
        help="verbose"
    )
    
    opt.add_option(
        "--dummy",
        action="store_true",
        default=False,
        help="use dummy HDEEM wrapper"
    )

    opt.add_option(
        "--debug",
        action="store_true",
        default=False,
        help="active debug output"
    )

    opt.add_option(
        "--cpp",
        action="store",
        default="g++",
        help="C++ compiler"
    )

    opt.add_option(
        "--mpi",
        action="store",
        default="mpic++",
        help="MPI C++ compiler"
    )

    opt.add_option(
        "--noopenmp",
        action="store",
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
        help="List of paths to include.")

    opt.add_option("--libpaths",
        action="store",
        type="string",
        metavar="LIB_PATHS",
        default="",
        help="List of paths to libraries.")

