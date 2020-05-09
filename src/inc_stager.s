.global g_stager
.global g_stager_size

g_stager:
    .incbin "build/stager.bin"
stager_end:

g_stager_size:
    .long stager_end - g_stager

