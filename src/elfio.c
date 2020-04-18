#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdbool.h>
#include "elfio.h"

static int _get_file_size(const char *filepath)
{
    struct stat st;
    if (stat(filepath, &st) < 0)
        return -1;
    return st.st_size;
}

bool load_elf(drow_ctx_t **ctx, const char *elffile)
{
    int size;
    int fd;
    void *elf = NULL;

    *ctx = NULL;
    size = _get_file_size(elffile);
    if (size < 0) {
        fprintf(stderr, "failed to get ELF file size\n");
        return false;
    }

    fd = open(elffile, O_RDONLY);
    if (fd == -1) {
        fprintf(stderr, "failed to open ELF file\n");
        return false;
    }

    elf = (void *)mmap(NULL, size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (!elf) {
        fprintf(stderr, "failed to map ELF file\n");
        goto error;
    }

    *ctx = (drow_ctx_t *)malloc(sizeof(drow_ctx_t));
    if (!*ctx) {
        fprintf(stderr, "failed to allocate memory for drow context");
        goto error;
    }

    (*ctx)->fd = fd;
    (*ctx)->size = size;
    (*ctx)->elf = elf;
    return true;
error:
    free(*ctx);
    if (elf)
        munmap(elf, size);
    if (fd == -1)
        close(fd);
    return false;
}

void unload_elf(drow_ctx_t *ctx)
{
    if (!ctx)
        return;

    if (ctx->elf)
        munmap(ctx->elf, ctx->size);

    if (ctx->fd != -1)
        close(ctx->fd);
}
