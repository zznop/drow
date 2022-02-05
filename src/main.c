#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <getopt.h>
#include "io.h"
#include "elf_patch.h"
#include "drow.h"

static void _print_banner(void)
{
    printf(
        "     ____  ____  _____  _    _\n"
        "    (  _ \\(  _ \\(  _  )( \\/\\/ )\n"
        "     )(_) ))   / )(_)(  )    (\n"
        "    (____/(_)\\_)(_____)(__/\\__)\n\n"
    );
}

static void _print_help(void)
{
    printf(
        "[v0.0.1]\n"
        "drow [options] infile patchfile outfile\n"
        "options:\n"
        "  -h    display usage\n"
    );
}

static bool _patch_elf_file(char *infile, char *patchfile, char *outfile)
{
    struct shinfo *sinfo = NULL;

    // Map the ELF
    fmap_t *elf;
    bool rv = load_fmap(&elf, infile);
    if (rv == false)
        return 1;

    // Map the patch
    fmap_t *patch;
    rv = load_fmap(&patch, patchfile);
    if (!rv)
        goto done;

    printf(INFO "Finding last section in executable segment ...\n");
    sinfo = find_exe_seg_last_section(elf, patch->size);
    if (!sinfo) {
        rv = false;
        goto done;
    }

    // Expand the section
    struct tgt_info tinfo = {0};
    rv = expand_section(elf, sinfo, &tinfo, patch->size);
    if (!rv)
        goto done;

    // Overwrite ELF header e_entry to make the patch the entry
    uint32_t old_entry;
    patch_entry(elf, &tinfo, &old_entry);

    // Write out new ELF
    rv = export_elf_file(elf, patch, outfile, &tinfo, old_entry, sinfo->inject_method);
    if (rv == true)
        printf(SUCCESS "ELF patched successfully!\n");
    else
        printf(ERR "Failed to patch ELF file\n");
done:
    free(sinfo);
    unload_fmap(elf);
    unload_fmap(patch);
    return (rv == false);
}

int main(int argc, char **argv)
{
    int opt;
    char *patchfile = NULL;
    char *infile = NULL;
    char *outfile = NULL;
    int i = 1;

    if (argc < 3) {
        _print_help();
        return 0;
    }

    while (i < argc) {
        if ((opt = getopt(argc, argv, "hv")) != -1) {
            switch (opt) {
            case 'h':
                _print_help();
                return 0;
            default:
                _print_help();
                return 1;
            }
        } else {
            switch (i) {
            case 1:
                infile = argv[i];
                break;
            case 2:
                patchfile = argv[i];
                break;
            case 3:
                outfile = argv[i];
                break;
            default:
                _print_help();
                return 1;
            }
            i++;
        }
    }

    if (!infile) {
        fprintf(stderr, "no input ELF file\n");
        return 1;
    }

    if (!patchfile) {
        fprintf(stderr, "no patch file\n");
        return 1;
    }

    if (!outfile) {
        fprintf(stderr, "no out ELF file path\n");
        return 1;
    }

    _print_banner();
    return _patch_elf_file(infile, patchfile, outfile);
}
