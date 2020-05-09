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
 * @param sinfo Section information struct
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

/**
 * Overwrite ELF e_entry so that it points to the injected patch
 *
 * @param elfinfo ELF informations struct
 * @param pinfo Patch information struct
 */
void patch_entry(elf_t *elfinfo, struct patchinfo *pinfo);

#endif
