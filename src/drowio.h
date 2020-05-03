#ifndef _DROWIO_H
#define _DROWIO_H

#include <stdbool.h>
#include <stdint.h>
#include "drow.h"

typedef struct {
    int fd;
    uint8_t *data;
    size_t size;
} payload_t;

/**
 * Load a target ELF file and initialize the drow context
 *
 * @param ctx Uninitialized pointer to drow context pointer
 * @param elffile File path to target ELF file
 * @return true for success, false for failure
 */
bool load_elf(drow_ctx_t **ctx, const char *elffile);

/**
 * Map in payload blob
 *
 * @param payload Output payload structure
 * @param patchfile Path to payload blob
 */
bool load_payload(payload_t **payload, const char *patchfile);

/**
 * Unload ELF from drow and cleanup memory
 *
 * @param ctx Drow context
 */
void unload_elf(drow_ctx_t *ctx);

/**
 * Export fixed up ELF file
 *
 * @param ctx Drow context
 * @param payload Payload file structure
 * @param outfile Path to output file
 * @param pinfo Patch information
 * @return true for success, false for failure
 */
bool export_elf_file(drow_ctx_t *ctx, payload_t *payload, char *outfile, struct patchinfo *pinfo);

#endif
