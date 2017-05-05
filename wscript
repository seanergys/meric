
from waflib import Logs

def configure(ctx):
    ctx.load(ctx.options.cpp)

    if ctx.options.flags:
        ctx.env.append_unique("CXXFLAGS", ctx.options.flags.split( ))
    if ctx.options.withgcc:
        ctx.env.append_unique("CXXFLAGS", [ "-std=c++14", "-fPIC", "-fopenmp"])    #GCC
    else:
        ctx.env.append_unique("CXXFLAGS", [ "-std=c++14", "-fPIC", "-qopenmp"])    #Intel
#    ctx.env.append_unique("CXXFLAGS", [ "-std=c++14", "-fPIC"])                #no openMP
    ctx.env.append_unique("DEFINES", [ "HAVE_OMP_H" ])                         #with openMP

    ctx.env.append_unique("DEFINES", [ "JETSONMEASUREMENTPATH=\""+ctx.path.abspath()+"/tools/jetsonMeasurement\""])
    
    if ctx.options.debug:
        ctx.env.append_unique("DEFINES", [ "__MERIC_DEBUG__" ])
    if ctx.options.verbose:
        ctx.env.append_unique("DEFINES", [ "VERBOSE" ])
    if ctx.options.dummy:
        ctx.env.append_unique("DEFINES", [ "USE_DUMMY" ])
        ctx.env.append_unique("CXXFLAGS", [ "-I"+ctx.path.abspath()+"/include/"])
    else:
#        ctx.env.append_unique("DEFINES", [ "HAVE_HDEEM_H" ])	# if problem with searching
        ctx.check_cc(header_name="hdeem.h", execute=False, mandatory=True)
    ctx.check_cc(header_name="linux/perf_event.h", execute=False, mandatory=False)
    ctx.check_cc(header_name="cpufreq.h", execute=False, mandatory=False)
    ctx.check_cc(header_name="x86_adapt.h", execute=False, mandatory=False)
    ctx.check_cc(header_name="papi.h", execute=False, mandatory=False)
#    ctx.check_cc(header_name="cilk/cilk_api.h", execute=False, mandatory=False)

#    ctx.env.append_unique("DEFINES", [ "HAVE_LINUX_PERF_EVENT_H" ]) #if problem with searching
#    ctx.env.append_unique("DEFINES", [ "HAVE_X86_ADAPT_H" ])	#if problem with searching
    
    ctx.find_program(ctx.options.mpi, var="MPICXX", mandatory=False)
    if ctx.env.MPICXX:
        ctx.setenv("mpi", ctx.env.derive())
        ctx.env.CXX = list(ctx.env.MPICXX)
        ctx.env.append_unique("DEFINES", [ "HAVE_MPI_H" ])

    ctx.msg("Compiler flags", " ".join(ctx.env["CXXFLAGS"]))

    if not ctx.env.MPICXX:
        Logs.warn("MPI compiler not found. Only shared memory parallelization is supported")

def build(ctx):

    ctx.objects(
        source =
            ctx.path.ant_glob('src/basis/*.cpp') +
            ctx.path.ant_glob('src/store/*.cpp') +
            ctx.path.ant_glob('src/meric/*.cpp') +
            ctx.path.ant_glob('src/wrapper/*.cpp'),
        target = "basis",
    )

    ctx.stlib(
        source       = ctx.path.ant_glob('src/meric/shared/*.cpp'),
        target       = "meric",
        use          = "basis",
        install_path = "lib"
    )

    ctx.shlib(
        source       = ctx.path.ant_glob('src/meric/shared/*.cpp'),
        target       = "meric",
        use          = "basis",
        install_path = "lib"
    )

    if not ctx.env.MPICXX:
        return

    ctx.stlib(
        source       = ctx.path.ant_glob('src/meric/distributed/*.cpp'),
        target       = "mericmpi",
        use          = "basis",
        install_path = "lib",
        env          = ctx.all_envs["mpi"]
    )

    ctx.shlib(
        source       = ctx.path.ant_glob('src/meric/distributed/*.cpp'),
        target       = "mericmpi",
        use          = "basis",
        install_path = "lib",
        env          = ctx.all_envs["mpi"]
    )


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
        "--flags",
        action="store",
        default="-O2",
        help="Compiler flags (-std=c++14, -fPIC and -qopenmp are default)"
    )
