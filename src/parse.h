#ifndef _PARSE_H
#define _PARSE_H

#include <stdbool.h>
#include <elf.h>
#include <stdint.h>
#include "slackinfo.h"

/**
 * Parse the ELF sections and populate slack information linked list
 *
 * @param ctx Drow context
 * @param sinfo Output linked list of slack space information
 * @return true for success, false for failure
 */
bool find_slackspace(drow_ctx_t *ctx, struct slackinfo **sinfo);

#endif
