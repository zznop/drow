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

extern uint8_t *g_stager;
extern uint8_t *g_stager_end;

static int _get_file_size(const char *filepath)
{
    struct stat st;
    if (stat(filepath, &st) < 0)
        return -1;
    return st.st_size;
}

bool load_fmap(fmap_t **file, const char *filename)
{
    int size;
    int fd;
    void *data = NULL;

    printf(INFO "Mapping file: %s\n", filename);
    *file = NULL;
    size = _get_file_size(filename);
    if (size < 0) {
        fprintf(stderr, ERR "Failed to get file size\n");
        return false;
    }

    fd = open(filename, O_RDONLY);
    if (fd == -1) {
        fprintf(stderr, ERR "Failed to open file\n");
        return false;
    }

    data = (void *)mmap(NULL, size, PROT_READ|PROT_WRITE, MAP_PRIVATE, fd, 0);
    if (!data) {
        fprintf(stderr, ERR "Failed to map file\n");
        goto error;
    }

    *file = (fmap_t *)malloc(sizeof(fmap_t));
    if (!*file) {
        fprintf(stderr, ERR "Failed to allocate memory for file mapping");
        goto error;
    }

    (*file)->fd = fd;
    (*file)->size = size;
    (*file)->data = data;
    return true;
error:
    free(*file);
    if (data)
        munmap(data, size);
    if (fd != -1)
        close(fd);
    return false;
}

void unload_fmap(fmap_t *file)
{
    if (!file)
        return;

    if (file->data)
        munmap(file->data, file->size);
    if (file->fd != -1)
        close(file->fd);
    free(file);
}

bool export_elf_file(fmap_t *elf, fmap_t *patch, char *outfile, struct tgt_info *tinfo, uint32_t old_entry, int inject_method)
{
    int fd;
    int n;
    char *pad = NULL;
    size_t padsize;
    size_t remaining;
    bool rv = false;
    uint8_t *stager_buf = NULL;
    uint32_t stager_size;

    printf(INFO "Exporting patched ELF to %s ...\n", outfile);
    fd = open(outfile, O_RDWR|O_CREAT|O_TRUNC, 0777);
    if (fd == -1) {
        fprintf(stderr, ERR "Failed to create patched ELF\n");
        return false;
    }

    printf(INFO "Writing first part of ELF (size: %u)\n", tinfo->base);
    n = write(fd, elf->data, tinfo->base);
    if ((uint32_t)n != tinfo->base) {
        fprintf(stderr, ERR "Failed to export ELF (write 1st ELF chunk)\n");
        goto done;
    }

    stager_size = (uintptr_t)&g_stager_end - (uintptr_t)&g_stager;
    stager_buf = (uint8_t *)malloc(stager_size);
    if (!stager_buf) {
        fprintf(stderr, ERR "Failed to allocate memory for the stager buffer\n");
        goto done;
    }
    memcpy(stager_buf, &g_stager, stager_size);

    printf(INFO "Setting old and new e_entry values in stager ...\n");
    *(uint32_t *)(stager_buf + 2) = old_entry;
    *(uint32_t *)(stager_buf + 6) = tinfo->base;

    printf(INFO "Writing stager stub (size: %u) ...\n", stager_size);
    n = write(fd, stager_buf, stager_size);
    if ((size_t)n != stager_size) {
        fprintf(stderr, ERR "Failed to export ELF (stager write)\n");
        goto done;
    }

    printf(INFO "Writing patch/payload (size: %u)\n", patch->size);
    n = write(fd, patch->data, patch->size);
    if (n != patch->size) {
        fprintf(stderr, ERR "Failed to export ELF (write patch)\n");
        goto done;
    }

    /* Allocate buffer for pad */
    if (inject_method == METHOD_EXPAND_AND_INJECT) {
        padsize = tinfo->size - patch->size - stager_size;
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
        remaining = elf->size - tinfo->base;
    } else {
        remaining = elf->size - tinfo->base - (stager_size + tinfo->size);
    }

    if (!remaining) {
        rv = true;
        goto done;
    }

    /* Write rest of the ELF */
    printf(INFO "Writing remaining data (size: %lu)\n", remaining);
    if (inject_method == METHOD_EXPAND_AND_INJECT)
        n = write(fd, elf->data + tinfo->base, remaining);
    else
        n = write(fd, elf->data+tinfo->base+stager_size+tinfo->size, remaining);

    if ((size_t)n != remaining) {
        fprintf(stderr, ERR "Failed to export ELF (write remaining)\n");
        goto done;
    }
    rv = true;
done:
    free(stager_buf);
    free(pad);
    close(fd);
    return rv;
}

