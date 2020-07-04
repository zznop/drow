#ifndef _ELF_PATCH_H
#define _ELF_PATCH_H

#include <stdbool.h>
#include <stdint.h>
#include "drow.h"

#define MAX_SH_NAMELEN 128

enum {
    METHOD_LAST_PAGE_INJECT = 0,
    METHOD_EXPAND_AND_INJECT,
};

struct shinfo {
    char name[MAX_SH_NAMELEN];
    uint32_t *offset;
    uint32_t *size;
    int inject_method;
};

/**
 * Fixup ELF header to expand a section size
 *
 * @param elf ELF information struct
 * @param sinfo Section information struct
 * @param tinfo Information on patch placement and size
 * @param patch_size Size of payload
 * @return true for success, false for failure
 */
bool expand_section(fmap_t *elf, struct shinfo *sinfo, struct tgt_info *tinfo, size_t patch_size);

/**
 * Locate last section in executable segment
 *
 * @param elf ELF information struct
 * @param patch_size Size of patch/payload
 * @return shinfo structure containing information on section to be expanded, or NULL on failure
 */
struct shinfo *find_exe_seg_last_section(fmap_t *elf, size_t patch_size);

/**
 * Overwrite ELF e_entry so that it points to the injected patch
 *
 * @param elf ELF informations struct
 * @param tinfo Information on patch placement and size
 * @param old_entry Pointer to output original e_entry offset
 */
void patch_entry(fmap_t *elf, struct tgt_info *tinfo, uint32_t *old_entry);

#endif
