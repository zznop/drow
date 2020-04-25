#include <stdlib.h>
#include <stdio.h>
#include "slackinfo.h"

void free_slackinfo_list(struct slackinfo *head)
{
    struct slackinfo *curr;
    while (head != NULL) {
        curr = head;
        head = head->next;
        free(curr);
    }
}
