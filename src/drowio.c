#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdbool.h>
#include "drowio.h"
#include "elf_patch.h"
#include "drow.h"

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

    printf(INFO "Loading ELF file: %s\n", elffile);
    *ctx = NULL;
    size = _get_file_size(elffile);
    if (size < 0) {
        fprintf(stderr, ERR "Failed to get ELF file size\n");
        return false;
    }

    fd = open(elffile, O_RDONLY);
    if (fd == -1) {
        fprintf(stderr, ERR "Failed to open ELF file\n");
        return false;
    }

    elf = (void *)mmap(NULL, size, PROT_READ|PROT_WRITE, MAP_PRIVATE, fd, 0);
    if (!elf) {
        fprintf(stderr, ERR "Failed to map ELF file\n");
        goto error;
    }

    *ctx = (drow_ctx_t *)malloc(sizeof(drow_ctx_t));
    if (!*ctx) {
        fprintf(stderr, ERR "Failed to allocate memory for drow context\n");
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
    if (fd != -1)
        close(fd);
    return false;
}

bool load_payload(payload_t **payload, const char *patchfile)
{
    int size;
    int fd;
    void *data = NULL;

    *payload = NULL;
    printf(INFO "Loading payload blob: %s\n", patchfile);
    size = _get_file_size(patchfile);
    if (size < 0) {
        fprintf(stderr, ERR "Failed to get payload file size\n");
        return false;
    }

    fd = open(patchfile, O_RDONLY);
    if (fd == -1) {
        fprintf(stderr, ERR "Failed to open payload\n");
        return false;
    }

    data = (uint8_t *)mmap(NULL, size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (!data) {
        fprintf(stderr, ERR "Failed to map payload\n");
        goto error;
    }

    *payload = (payload_t *)malloc(sizeof(payload_t));
    if (!*payload) {
        fprintf(stderr, ERR "Failed to allocate memory for payload\n");
        goto error;
    }

    (*payload)->fd = fd;
    (*payload)->data = data;
    (*payload)->size = size;
    return true;
error:
    free(*payload);
    if (data)
        munmap(data, size);
    if (fd != -1)
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

bool export_elf_file(drow_ctx_t *ctx, payload_t *payload, char *outfile, struct patchinfo *pinfo)
{
    int fd;
    int n;
    char *pad = NULL;
    size_t padsize;
    size_t remaining;
    bool rv = false;

    printf(INFO "Exporting patched ELF to %s ...\n", outfile);
    fd = open(outfile, O_RDWR|O_CREAT|O_TRUNC, 0777);
    if (fd == -1) {
        fprintf(stderr, ERR "Failed to create patched ELF\n");
        return false;
    }

    printf(INFO "Writing first part of ELF (size: %u)\n", pinfo->base);
    n = write(fd, ctx->elf, pinfo->base);
    if ((uint32_t)n != pinfo->base) {
        fprintf(stderr, ERR "Failed to export ELF (write 1st ELF chunk)\n");
        goto done;
    }

    printf(INFO "Writing payload (size: %lu)\n", payload->size);
    n = write(fd, payload->data, payload->size);
    if ((size_t)n != payload->size) {
        fprintf(stderr, ERR "Failed to export ELF (write payload)\n");
        goto done;
    }

    /* Allocate buffer for pad */
    padsize = pinfo->size - payload->size;
    pad = (char *)malloc(padsize);
    if (pad == NULL) {
        fprintf(stderr, ERR "Failed to export ELF (out of memory?)\n");
        goto done;
    }
    memset(pad, 0, padsize);

    printf(INFO "Writing pad to maintain page alignment (size: %lu)\n", padsize);
    n = write(fd, pad, padsize);
    if ((size_t)n != padsize) {
        fprintf(stderr, ERR "Failed to export ELF (write pad)\n");
        goto done;
    }

    /* Write rest of the ELF */
    remaining = ctx->size - pinfo->base;
    if (remaining) {
        printf(INFO "Writing remaining data (size: %lu)\n", remaining);
        n = write(fd, ctx->elf + pinfo->base, remaining);
        if ((size_t)n != remaining) {
            fprintf(stderr, ERR "Failed to export ELF (write remaining)\n");
            goto done;
        }
    }

    rv = true;
done:
    free(pad);
    close(fd);
    return rv;
}

