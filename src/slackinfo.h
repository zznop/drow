#ifndef _SLACKINFO_H
#define _SLACKINFO_H

#include <stdint.h>
#include <stdbool.h>

#define MAX_SH_NAMELEN 128

struct slackinfo {
    struct slackinfo *next;
    bool is_exec;
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
 * Iterate and free a slackinfo linked list
 *
 * @param head Head of slackinfo linked list
 */
void free_slackinfo_list(struct slackinfo *head);

#endif
