#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdbool.h>
#include "elfio.h"
#include "slackinfo.h"
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
        fprintf(stderr, "failed to get ELF file size\n");
        return false;
    }

    fd = open(elffile, O_RDONLY);
    if (fd == -1) {
        fprintf(stderr, "failed to open ELF file\n");
        return false;
    }

    elf = (void *)mmap(NULL, size, PROT_READ|PROT_WRITE, MAP_PRIVATE, fd, 0);
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

bool expand_section_by_name(struct slackinfo *sinfo, char *section_name, struct patchinfo *pinfo)
{
    struct slackinfo *curr;
    size_t adjust;
    size_t size;
    uint32_t newoff;
    bool found = false;

    curr = sinfo;
    while (curr != NULL) {
        if (!found && strcmp(curr->name, section_name)) {
            curr = curr->next;
            continue;
        }
        found = true;

        if (!curr->slackspace) {
            fprintf(stderr, "Section contains no slack space\n");
            return false;
        }
        printf(INFO "Expanding section %s by %lu bytes ...\n", curr->name, curr->slackspace);

        /* Set patch information */
        size = *curr->size;
        pinfo->base = *curr->offset + size;
        pinfo->size = curr->slackspace;

        /* Fix up size */
        *curr->size = (size + curr->slackspace);
        adjust = curr->slackspace;
        break;
    }

    if (!found) {
        fprintf(stderr, "Section does not exist: %s\n", section_name);
        return false;
    }

    printf(INFO "Fixing remaining section header offsets ...\n");
    curr = curr->next;
    while (curr != NULL) {
        newoff = *curr->offset + adjust;
        printf("   -- %s old=%08x new=%08x\n", curr->name, *curr->offset, newoff);
        *curr->offset = newoff;
        curr = curr->next;
    }

    return true;
}

bool export_elf_file(drow_ctx_t *ctx, char *outfile, struct patchinfo *pinfo)
{
    int fd;
    int n;
    char *patch;
    size_t remaining;
    bool rv = false;

    fd = open(outfile, O_RDWR|O_CREAT|O_TRUNC, 0777);
    if (fd == -1) {
        fprintf(stderr, ERR "Failed to create patched ELF\n");
        return false;
    }

    /* Write ELF data up until where we are writing our patch */
    n = write(fd, ctx->elf, pinfo->base);
    if ((uint32_t)n != pinfo->base) {
        fprintf(stderr, ERR "Failed to export ELF (write)\n");
        goto done;
    }

    /* Allocate patch buffer */
    patch = (char *)malloc(pinfo->size);
    if (patch == NULL) {
        fprintf(stderr, ERR "Out of memory?\n");
        goto done;
    }
    memset(patch, 'A', pinfo->size);

    /* TODO: write payload to patch */

    /* Write patch to file */
    n = write(fd, patch, pinfo->size);
    if ((size_t)n != pinfo->size) {
        fprintf(stderr, ERR "Failed to export ELF (write patch)\n");
        goto done;
    }

    /* Write rest of the ELF */
    remaining = ctx->size - pinfo->base;
    n = write(fd, ctx->elf + pinfo->base, remaining);
    if ((size_t)n != remaining) {
        fprintf(stderr, ERR "Failed to export ELF (write remaining)\n");
        goto done;
    }

    rv = true;
done:
    close(fd);
    return rv;
}
