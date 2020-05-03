#ifndef _DROWIO_H
#define _DROWIO_H

#include <stdbool.h>
#include <stdint.h>
#include "drow.h"

typedef struct {
    int fd;
    uint8_t *data;
    size_t size;
} patch_t;

/**
 * Load a target ELF file and initialize the elfinfo struct
 *
 * @param elfinfo Uninitialized pointer to output elfinfo struct
 * @param elffile File path to target ELF file
 * @return true for success, false for failure
 */
bool load_elf(elf_t **elfinfo, const char *elffile);

/**
 * Map in payload blob
 *
 * @param payload Output payload structure
 * @param patchfile Path to payload blob
 */
bool load_patch(patch_t **patch, const char *patchfile);

/**
 * Unload ELF from drow and cleanup memory
 *
 * @param elfinfo ELF information struct
 */
void unload_elf(elf_t *elfinfo);

/**
 * Export fixed up ELF file
 *
 * @param elfinfo ELF information struct
 * @param payload Payload file structure
 * @param outfile Path to output file
 * @param pinfo Patch information
 * @return true for success, false for failure
 */
bool export_elf_file(elf_t *elfinfo, patch_t *patch, char *outfile, struct patchinfo *pinfo);

#endif
