Import('env')

# Build the stager blob
stager_obj = env.Object('./src/arch/x86-64/stager.s')
stager_blob = env.Command(
    'stager.bin',
    stager_obj,
    'objcopy -O binary --only-section=.text $SOURCE $TARGET'
)

# Build rappers delight blob
rappers_delight_obj = env.Object('./src/arch/x86-64/rappers-delight.s')
rappers_delight_blob = env.Command(
    'rappers_delight.bin',
    rappers_delight_obj,
    'objcopy -O binary --only-section=.text $SOURCE $TARGET'
)

# Build drow
sources = [
    './src/inc_stager.s',
    './src/io.c',
    './src/elf_patch.c',
    './src/main.c',
]

objs = env.Object(
    source=sources,
)
env.Depends(objs, stager_blob)

drow = env.Program(
    source=objs,
    target='drow',
)

Return('drow')
