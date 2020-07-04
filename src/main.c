#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <getopt.h>
#include "io.h"
#include "elf_patch.h"
#include "drow.h"

static void print_banner(void)
{
    printf(
        "     ____  ____  _____  _    _\n"
        "    (  _ \\(  _ \\(  _  )( \\/\\/ )\n"
        "     )(_) ))   / )(_)(  )    (\n"
        "    (____/(_)\\_)(_____)(__/\\__)\n\n"
    );
}

static void print_help(void)
{
    printf(
        "[v0.0.1]\n"
        "drow [options] infile patchfile outfile\n"
        "options:\n"
        "  -h    display usage\n"
    );
}

static bool do_work(char *infile, char *patchfile, char *outfile)
{
    bool rv;
    struct shinfo *sinfo = NULL;
    struct tgt_info tinfo = {0};
    uint32_t old_entry;
    fmap_t *patch;
    fmap_t *elf;

    /* Map in the ELF */
    rv = load_fmap(&elf, infile);
    if (rv == false)
        return 1;

    /* Map in patch */
    rv = load_fmap(&patch, patchfile);
    if (rv == false)
        goto done;

    printf(INFO "Finding last section in executable segment ...\n");
    sinfo = find_exe_seg_last_section(elf, patch->size);
    if (sinfo == NULL) {
        rv = false;
        goto done;
    }

    /* Expand the section */
    rv = expand_section(elf, sinfo, &tinfo, patch->size);
    if (rv == false)
        goto done;

    /* Overwrite ELF header e_entry to make the patch the entry */
    patch_entry(elf, &tinfo, &old_entry);

    /* Write out new ELF file */
    rv = export_elf_file(elf, patch, outfile, &tinfo, old_entry, sinfo->inject_method);
    if (rv == true)
        printf(SUCCESS "ELF patched successfully!\n");
    else
        printf(ERR "Failed to patch ELF file\n");

    /* Cleanup */
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
        print_help();
        return 0;
    }

    while (i < argc) {
        if ((opt = getopt(argc, argv, "hv")) != -1) {
            switch (opt) {
            case 'h':
                print_help();
                return 0;
            default:
                print_help();
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
                print_help();
                return 1;
            }
            i++;
        }
    }

    if (infile == NULL) {
        fprintf(stderr, "no input ELF file\n");
        return 1;
    }

    if (patchfile == NULL) {
        fprintf(stderr, "no patch file\n");
        return 1;
    }

    if (outfile == NULL) {
        fprintf(stderr, "no out ELF file path\n");
        return 1;
    }

    print_banner();
    return do_work(infile, patchfile, outfile);
}
