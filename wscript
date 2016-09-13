
from waflib import Logs

def configure(ctx):
    ctx.load(ctx.options.cpp)

    ctx.env.append_unique("CXXFLAGS", [ "-std=c++11", "-g", "-O0" ])
    if ctx.options.dummy:
        ctx.env.append_unique("DEFINES", [ "USE_DUMMY" ])

    ctx.find_program(ctx.options.mpi, var="MPICXX", mandatory=False)
    if ctx.env.MPICXX:
        ctx.setenv("mpi", ctx.env.derive())
        ctx.env.CXX = list(ctx.env.MPICXX)
    else:
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

    if not ctx.env.MPICXX:
        return

    ctx.stlib(
        source       = ctx.path.ant_glob('src/meric/distributed/*.cpp'),
        target       = "mericmpi",
        use          = "basis",
        install_path = "lib",
        env          = ctx.all_envs["mpi"]
    )


def options(opt):
    opt.add_option(
        "--dummy",
        action="store_true",
        default=False,
        help="use dummy HDEEM wrapper"
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
