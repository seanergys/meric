

def configure(ctx):
    ctx.load("g++")
    
    ctx.setenv("mpi", ctx.env.derive())
    ctx.env.CXX = [ "mpic++" ]

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

    ctx.stlib(
        source       = ctx.path.ant_glob('src/meric/distributed/*.cpp'),
        target       = "mericmpi",
        use          = "basis",
        install_path = "lib",
        env          = ctx.all_envs["mpi"]
    )