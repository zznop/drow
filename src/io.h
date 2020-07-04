#ifndef _IO_H
#define _IO_H

#include <stdbool.h>
#include <stdint.h>
#include "drow.h"

/**
 * Map in a file and return a populated file mapping structure
 *
 * @param file Uninitialized pointer to output file mapping structure
 * @param filename Path to file
 * @return true for success, false for failure
 */
bool load_fmap(fmap_t **file, const char *filename);

/**
 * Unload file mapping and cleanup memory
 *
 * @param file File mapping structure
 */
void unload_fmap(fmap_t *file);

/**
 * Export fixed up ELF file
 *
 * @param elf ELF information struct
 * @param patch Mapped patch file information
 * @param outfile Path to output file
 * @param tinfo Information on patch placement and size
 * @param old_entry Old e_entry before it was overwritten to point to the payload
 * @param inject_method Injection method enum
 * @return true for success, false for failure
 */
bool export_elf_file(fmap_t *elf, fmap_t *patch, char *outfile, struct tgt_info *tinfo, uint32_t old_entry, int inject_method);

#endif
