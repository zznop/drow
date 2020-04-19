#ifndef _PARSE_H
#define _PARSE_H

#include <stdbool.h>
#include <elf.h>

/**
 * Parse the ELF sections
 *
 * @param ctx Drow context
 * @return true for success, false for failure
 */
bool analyze_elf_sections(drow_ctx_t *ctx);

#endif
