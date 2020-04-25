base_env = Environment(
    BUILD_DIR='build',
    CFLAGS=['-W', '-Wall', '-O2'],
)


drow = base_env.SConscript(
    './SConscript',
    variant_dir='$BUILD_DIR',
    duplicate=False,
    exports=dict(env=base_env),
)
