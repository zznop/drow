#include <elf.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "elfio.h"

#define ELF_MAGIC "\177ELF"

bool _is_elf64(void *elf) {
    Elf32_Ehdr *ehdr = (Elf32_Ehdr *)elf;
    if (ehdr->e_ident[EI_CLASS] == ELFCLASS64)
        return true;
    return false;
}

bool analyze_elf_sections(drow_ctx_t *ctx)
{
    Elf64_Ehdr *ehdr;
    Elf64_Shdr *shtable;
    char *shstr;
    int i;
    int slackspace;

    /* Make sure input is an ELF file */
    if (!strncmp((char *)ctx->elf, ELF_MAGIC, sizeof(ELF_MAGIC))) {
        fprintf(stderr, "invalid ELF file\n");
        return 1;
    }

    /* Make sure input is a Elf64 (TODO: add Elf32 support) */
    if (!_is_elf64(ctx->elf)) {
        fprintf(stderr, "Elf32 not implemented yet\n");
        return false;
    }

    ehdr = (Elf64_Ehdr *)ctx->elf;
    shtable = (Elf64_Shdr *)((uintptr_t)ctx->elf + ehdr->e_shoff);
    shstr = (char *)((uintptr_t)ctx->elf + shtable[ehdr->e_shstrndx].sh_offset);
    for (i = 0; i < ehdr->e_shnum; i++) {
        /* Skip non-executable sections */
        //if (!(shtable[i].sh_flags & SHF_EXECINSTR))
        //    continue;

        /* Display generic section information */
        printf("%s\n", shstr + shtable[i].sh_name);
        printf("  -- offset      : %08x\n", shtable[i].sh_offset);
        printf("  -- addr        : %08x\n", shtable[i].sh_addr);
        printf("  -- size        : %08x\n", shtable[i].sh_size);

        if (i == ehdr->e_shnum) {
            printf("  -- slack space : infinite (last section)\n", slackspace);
            continue;
        }

        slackspace = shtable[i+1].sh_addr - shtable[i].sh_addr;
        printf("  -- slack space : %i bytes\n", slackspace);
    }

    return true;
}
