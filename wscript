
from waflib import Logs

def configure(ctx):
    ctx.load("g++")

    ctx.env.append_unique("CXXFLAGS", [ "-std=c++11", "-g", "-O0" ])

    ctx.find_program("mpic++", var="MPICXX", mandatory=False)
    if ctx.env.MPICXX:
        ctx.setenv("mpi", ctx.env.derive())
        ctx.env.CXX = list(ctx.env.MPICXX)
    else:
        Logs.warn("MPI compiler not found. Only shared memory parallelization is supported")

def build(ctx):
    ctx.objects(
        source = ctx.path.ant_glob('src/basis/*.cpp') + ctx.path.ant_glob('src/wrappers/*.cpp') + ctx.path.ant_glob('src/meric/*.cpp'),
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