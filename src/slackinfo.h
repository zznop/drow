#ifndef _SLACKINFO_H
#define _SLACKINFO_H

#include <stdint.h>

struct slackinfo {
    struct slackinfo *next;
    uint32_t *offset;
    uint32_t *size;
    size_t slackspace;
};

/**
 * Iterate and free a slackinfo linked list
 *
 * @param head Head of slackinfo linked list
 */
void free_slackinfo_list(struct slackinfo *head);

#endif
