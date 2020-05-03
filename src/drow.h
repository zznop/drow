#ifndef _DROW_H
#define _DROW_H

typedef struct {
    int fd;
    int size;
    void *elf;
} elf_t;

struct patchinfo {
    uint32_t base;
    size_t size;
};

#define INFO "\033[0;36m[*]\033[0m "
#define SUCCESS "\033[0;32m[+]\033[0m "
#define ERR  "\033[0;31m[!]\033[0m "

#endif
