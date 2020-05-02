Import('env')

sources = [
    './src/main.c',
    './src/elfio.c',
]

objs = env.Object(
    source=sources,
)

drow = env.Program(
    source=objs,
    target='drow',
)

Return('drow')
