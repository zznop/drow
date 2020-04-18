#ifndef _ELF_IO_H
#define _ELF_IO_H

#include <stdbool.h>

typedef struct {
    int fd;
    int size;
    void *elf;
} drow_ctx_t;

/**
 * Load a target ELF file and initialize the drow context
 *
 * @param ctx Uninitialized pointer to drow context pointer
 * @param elffile File path to target ELF file
 * @return true for success, false for failure
 */
bool load_elf(drow_ctx_t **ctx, const char *elffile);

/**
 * Unload ELF from drow and cleanup memory
 *
 * @param ctx Drow context
 */
void unload_elf(drow_ctx_t *ctx);

#endif
