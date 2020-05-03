#include <stdbool.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <elf.h>
#include <unistd.h>
#include "elf_patch.h"
#include "drow.h"

bool expand_section(elf_t *elfinfo, struct shinfo *sinfo, struct patchinfo *pinfo)
{
    Elf64_Ehdr *ehdr;
    size_t adjust;
    size_t size;
    uint32_t newoff;
    Elf64_Shdr *shtable;
    size_t i;

    ehdr = (Elf64_Ehdr *)elfinfo->elf;

    /* Set patch information */
    size = *sinfo->size;
    pinfo->base = *sinfo->offset + size;
    pinfo->size = getpagesize();

    /* Fix up size */
    printf(INFO "Expanding %s size by %lu bytes...\n", sinfo->name, pinfo->size);
    *sinfo->size = size + sinfo->expand_size;
    adjust = getpagesize();

    printf(INFO "Adjusting Section Header offsets ...\n");
    shtable = (Elf64_Shdr *)((uintptr_t)elfinfo->elf + ehdr->e_shoff);
    for (i = 0; i < ehdr->e_shnum; i++) {
        if (shtable[i].sh_offset < pinfo->base)
            continue;
        newoff = shtable[i].sh_offset + adjust;
        shtable[i].sh_offset = newoff;
    }

    printf(INFO "Adjusting Program Header offsets ...\n");
    Elf64_Phdr *phdr = (Elf64_Phdr *)((uintptr_t)elfinfo->elf + ehdr->e_phoff);
    for (i = 0; i < ehdr->e_phnum; i++) {
        if (phdr[i].p_offset > pinfo->base) {
            phdr[i].p_offset = phdr[i].p_offset + pinfo->size;
        }

        if (phdr[i].p_flags & PF_X) {
            phdr[i].p_filesz += pinfo->size;
            phdr[i].p_memsz += pinfo->size;
        }
    }

    printf(INFO "Adjusting ELF header offsets ...\n");
    if (ehdr->e_shoff > pinfo->base)
        ehdr->e_shoff = ehdr->e_shoff + pinfo->size;

    if (ehdr->e_phoff > pinfo->base)
        ehdr->e_phoff = ehdr->e_phoff + pinfo->size;

    return true;
}

struct shinfo *find_exe_seg_last_section(elf_t *elfinfo)
{
    Elf64_Ehdr *ehdr;
    Elf64_Phdr *phdr;
    Elf64_Shdr *shtable;
    char *shstr;
    uint32_t segment_end;
    struct shinfo *sinfo = NULL;
    size_t i, j;

    ehdr = (Elf64_Ehdr *)elfinfo->elf;
    phdr = (Elf64_Phdr *)((uintptr_t)elfinfo->elf + ehdr->e_phoff);
    shtable = (Elf64_Shdr *)((uintptr_t)elfinfo->elf + ehdr->e_shoff);
    shstr = (char *)((uintptr_t)elfinfo->elf + shtable[ehdr->e_shstrndx].sh_offset);

    for (i = 0; i < ehdr->e_phnum; i++) {
        if (phdr[i].p_flags & PF_X) {
            printf(SUCCESS "Found executable segment at 0x%08lx (size:%08lx)\n", phdr[i].p_offset, phdr[i].p_memsz);
            /* Found the executable segment, now find the last section in the segment */
            segment_end = phdr[i].p_vaddr + phdr[i].p_memsz;
            for (j = 0; j < ehdr->e_shnum; j++) {
                if (shtable[j].sh_addr + shtable[j].sh_size == segment_end) {
                    sinfo = (struct shinfo *)malloc(sizeof(*sinfo));
                    if (!sinfo) {
                        fprintf(stderr, ERR "Out of memory!?");
                        return NULL;
                    }

                    strncpy(sinfo->name, shstr+shtable[j].sh_name, MAX_SH_NAMELEN);
                    sinfo->offset     = (uint32_t *)&shtable[j].sh_offset;
                    sinfo->size       = (uint32_t *)&shtable[j].sh_size;
                    sinfo->expand_size = getpagesize();
                }
            }
        }
    }
    return sinfo;
}
