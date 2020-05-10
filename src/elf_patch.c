#include <stdbool.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <elf.h>
#include <unistd.h>
#include "elf_patch.h"
#include "drow.h"

#define PAGE_SIZE 0x1000
#define ALIGN_PAGE(val) ((PAGE_SIZE-1) & val) ? ((val + PAGE_SIZE) & ~(PAGE_SIZE -1)) : val

bool expand_section(fmap_t *elf, struct shinfo *sinfo, struct tgt_info *tinfo, size_t patch_size)
{
    Elf64_Ehdr *ehdr;
    size_t size;
    uint32_t newoff;
    Elf64_Shdr *shtable;
    size_t i;
    size_t patch_size_aligned = (ALIGN_PAGE(patch_size)) + PAGE_SIZE;

    ehdr = (Elf64_Ehdr *)elf->data;

    /* Set patch information */
    size = *sinfo->size;
    tinfo->base = *sinfo->offset + size;
    tinfo->size = patch_size_aligned;

    /* Fix up size */
    printf(INFO "Expanding %s size by %lu bytes...\n", sinfo->name, tinfo->size);
    *sinfo->size = size + patch_size_aligned;

    printf(INFO "Adjusting Section Header offsets ...\n");
    shtable = (Elf64_Shdr *)((uintptr_t)elf->data + ehdr->e_shoff);
    for (i = 0; i < ehdr->e_shnum; i++) {
        if (shtable[i].sh_offset < tinfo->base)
            continue;
        newoff = shtable[i].sh_offset + patch_size_aligned;
        shtable[i].sh_offset = newoff;
    }

    printf(INFO "Adjusting Program Header offsets ...\n");
    Elf64_Phdr *phdr = (Elf64_Phdr *)((uintptr_t)elf->data + ehdr->e_phoff);
    for (i = 0; i < ehdr->e_phnum; i++) {
        if (phdr[i].p_offset > tinfo->base) {
            phdr[i].p_offset = phdr[i].p_offset + tinfo->size;
        }

        if (phdr[i].p_flags & PF_X) {
            phdr[i].p_filesz += tinfo->size;
            phdr[i].p_memsz += tinfo->size;
        }
    }

    printf(INFO "Adjusting ELF header offsets ...\n");
    if (ehdr->e_shoff > tinfo->base)
        ehdr->e_shoff = ehdr->e_shoff + tinfo->size;

    if (ehdr->e_phoff > tinfo->base)
        ehdr->e_phoff = ehdr->e_phoff + tinfo->size;

    return true;
}

struct shinfo *find_exe_seg_last_section(fmap_t *elf)
{
    Elf64_Ehdr *ehdr;
    Elf64_Phdr *phdr;
    Elf64_Shdr *shtable;
    char *shstr;
    uint32_t segment_end;
    struct shinfo *sinfo = NULL;
    size_t i, j;

    ehdr = (Elf64_Ehdr *)elf->data;
    phdr = (Elf64_Phdr *)((uintptr_t)elf->data + ehdr->e_phoff);
    shtable = (Elf64_Shdr *)((uintptr_t)elf->data + ehdr->e_shoff);
    shstr = (char *)((uintptr_t)elf->data + shtable[ehdr->e_shstrndx].sh_offset);

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
                    break;
                }
            }
        }
    }
    return sinfo;
}

void patch_entry(fmap_t *elf, struct tgt_info *tinfo, uint32_t *old_entry)
{
    Elf64_Ehdr *ehdr;
    printf(INFO "Modifying ELF e_entry to point to the patch at 0x%08x ...\n", tinfo->base);
    ehdr = (Elf64_Ehdr *)elf->data;
    *old_entry = ehdr->e_entry;
    ehdr->e_entry = (uint32_t)tinfo->base;
}
