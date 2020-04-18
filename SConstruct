env = Environment(
    BUILD_DIR='build',
)

drow = env.SConscript(
    './SConscript',
    variant_dir='$BUILD_DIR',
    duplicate=False,
    exports=dict(env=env),
)
