#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdbool.h>
#include "io.h"
#include "elf_patch.h"
#include "drow.h"

static int _get_file_size(const char *filepath)
{
    struct stat st;
    if (stat(filepath, &st) < 0)
        return -1;
    return st.st_size;
}

bool load_elf(elf_t **elfinfo, const char *elffile)
{
    int size;
    int fd;
    void *elf = NULL;

    printf(INFO "Loading ELF file: %s\n", elffile);
    *elfinfo = NULL;
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

    *elfinfo = (elf_t *)malloc(sizeof(elf_t));
    if (!*elfinfo) {
        fprintf(stderr, ERR "Failed to allocate memory for ELF information");
        goto error;
    }

    (*elfinfo)->fd = fd;
    (*elfinfo)->size = size;
    (*elfinfo)->elf = elf;
    return true;
error:
    free(*elfinfo);
    if (elf)
        munmap(elf, size);
    if (fd != -1)
        close(fd);
    return false;
}

bool load_patch(patch_t **patch, const char *patchfile)
{
    int size;
    int fd;
    void *data = NULL;

    *patch = NULL;
    printf(INFO "Loading patch file: %s\n", patchfile);
    size = _get_file_size(patchfile);
    if (size < 0) {
        fprintf(stderr, ERR "Failed to get patch file size\n");
        return false;
    }

    fd = open(patchfile, O_RDONLY);
    if (fd == -1) {
        fprintf(stderr, ERR "Failed to open patch file\n");
        return false;
    }

    data = (uint8_t *)mmap(NULL, size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (!data) {
        fprintf(stderr, ERR "Failed to map patch file\n");
        goto error;
    }

    *patch = (patch_t *)malloc(sizeof(patch_t));
    if (!*patch) {
        fprintf(stderr, ERR "Failed to allocate memory for patch\n");
        goto error;
    }

    (*patch)->fd = fd;
    (*patch)->data = data;
    (*patch)->size = size;
    return true;
error:
    free(*patch);
    if (data)
        munmap(data, size);
    if (fd != -1)
        close(fd);
    return false;
}

void unload_elf(elf_t *elfinfo)
{
    if (!elfinfo)
        return;

    if (elfinfo->elf)
        munmap(elfinfo->elf, elfinfo->size);

    if (elfinfo->fd != -1)
        close(elfinfo->fd);
}

bool export_elf_file(elf_t *elfinfo, patch_t *patch, char *outfile, struct patchinfo *pinfo)
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
    n = write(fd, elfinfo->elf, pinfo->base);
    if ((uint32_t)n != pinfo->base) {
        fprintf(stderr, ERR "Failed to export ELF (write 1st ELF chunk)\n");
        goto done;
    }

    printf(INFO "Writing patch (size: %lu)\n", patch->size);
    n = write(fd, patch->data, patch->size);
    if ((size_t)n != patch->size) {
        fprintf(stderr, ERR "Failed to export ELF (write patch)\n");
        goto done;
    }

    /* Allocate buffer for pad */
    padsize = pinfo->size - patch->size;
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
    remaining = elfinfo->size - pinfo->base;
    if (remaining) {
        printf(INFO "Writing remaining data (size: %lu)\n", remaining);
        n = write(fd, elfinfo->elf + pinfo->base, remaining);
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

