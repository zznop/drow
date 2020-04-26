#ifndef _ELF_IO_H
#define _ELF_IO_H

#include <stdbool.h>
#include "slackinfo.h"

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

/**
 * Fixup ELF header to expand a section size
 *
 * @param sinfo Slack information linked list
 * @param section_name Name of section to expand
 * @param pinfo Output patch information
 * @return true for success, false for failure
 */
bool expand_section_by_name(struct slackinfo *sinfo, char *section_name, struct patchinfo *pinfo);

/**
 * Export fixed up ELF file
 *
 * @param ctx Drow context
 * @param outfile Path to output file
 * @param pinfo Patch information
 * @return true for success, false for failure
 */
bool export_elf_file(drow_ctx_t *ctx, char *outfile, struct patchinfo *pinfo);

#endif
