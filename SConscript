Import('env')

sources = [
    './src/drowio.c',
    './src/elf_patch.c',
    './src/main.c',
]

objs = env.Object(
    source=sources,
)

drow = env.Program(
    source=objs,
    target='drow',
)

Return('drow')
