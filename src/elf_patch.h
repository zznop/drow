#ifndef _ELF_PATCH_H
#define _ELF_PATCH_H

#include <stdbool.h>
#include <stdint.h>
#include "drow.h"

#define MAX_SH_NAMELEN 128

struct shinfo {
    char name[MAX_SH_NAMELEN];
    uint32_t *offset;
    uint32_t *size;
    size_t expand_size;
};

/**
 * Fixup ELF header to expand a section size
 *
 * @param elfinfo ELF information struct
 * @param sinfo Slack information linked list
 * @param pinfo Output patch information
 * @return true for success, false for failure
 */
bool expand_section(elf_t *elfinfo, struct shinfo *sinfo, struct patchinfo *pinfo);

/**
 * Locate last section in executable segment
 *
 * @param elfinfo ELF information struct
 * @return shinfo structure containing information on section to be expanded, or NULL on failure
 */
struct shinfo *find_exe_seg_last_section(elf_t *elfinfo);

#endif
