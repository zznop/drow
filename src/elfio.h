#ifndef _ELF_IO_H
#define _ELF_IO_H

#include <stdbool.h>
#include <stdint.h>

#define MAX_SH_NAMELEN 128

typedef struct {
    int fd;
    int size;
    void *elf;
} drow_ctx_t;


struct shinfo {
    char name[MAX_SH_NAMELEN];
    uint32_t *offset;
    uint32_t *size;
    size_t slackspace;
};

struct patchinfo {
    uint32_t base;
    size_t size;
};

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
 * @param ctx Drow context
 * @param sinfo Slack information linked list
 * @param pinfo Output patch information
 * @return true for success, false for failure
 */
bool expand_section(drow_ctx_t *ctx, struct shinfo *sinfo, struct patchinfo *pinfo);

/**
 * Export fixed up ELF file
 *
 * @param ctx Drow context
 * @param outfile Path to output file
 * @param pinfo Patch information
 * @return true for success, false for failure
 */
bool export_elf_file(drow_ctx_t *ctx, char *outfile, struct patchinfo *pinfo);

/**
 * Locate last section in executable segment
 *
 * @param ctx Drow context
 * @return shinfo structure containing information on section to be expanded, or NULL on failure
 */
struct shinfo *find_exe_seg_last_section(drow_ctx_t *ctx);

#endif
