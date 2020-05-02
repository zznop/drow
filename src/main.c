#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <getopt.h>
#include "elfio.h"
#include "drow.h"

static void print_help(void)
{
    printf(
        "[v0.0.1]\n" // TODO: Use preprocessor/git describe version
        "drow [-options] infile\n"
        "options:\n"
        "  -a         Analyze ELF and display segment information and available space\n"
        "  -i file    Inject a payload blob in available space\n"
        "  -o file    Path to patched ELF file\n"
    );
}

int main(int argc, char **argv)
{
    int opt;
    char *infile = NULL;
    char *outfile = NULL;
    bool verbose = false;
    bool analyze = true;
    bool rv;
    struct shinfo *sinfo = NULL;
    struct patchinfo pinfo = {0};
    drow_ctx_t *ctx;

    if (argc < 2) {
        print_help();
        return 0;
    }

    while (optind < argc) {
        if ((opt = getopt(argc, argv, "hi:o:v")) != -1) {
            switch (opt) {
            case 'h':
                print_help();
                return 0;
            case 'i':
                analyze = false;
                break;
            case 'v':
                verbose = true;
                (void)verbose;
                break;
            case 'o':
                outfile = optarg;
                break;
            default:
                print_help();
                return 1;
            }
        } else {
            if (infile != NULL) {
                print_help();
                return 1;
            }

            infile = argv[optind];
            optind++;
        }
    }

    if (infile == NULL) {
        fprintf(stderr, "no input ELF file\n");
        return 1;
    }

    if (analyze == false && outfile == NULL) {
        fprintf(stderr, "no out ELF file path\n");
        return 1;
    }

    /* Map in the ELF */
    rv = load_elf(&ctx, infile);
    if (rv == false)
        return 1;

    printf(INFO "Finding last section in executable segment ...\n");
    sinfo = find_exe_seg_last_section(ctx);
    if (sinfo == NULL) {
        rv = false;
        goto done;
    }

    printf("  -- %s\n", sinfo->name);
    printf("    -- offset      : 0x%08x\n", *sinfo->offset);
    printf("    -- size        : 0x%08x\n", *sinfo->size);

    /* If we're just analyzing, bail out */
    if (analyze)
        goto done;

    /* Expand the section by name */
    rv = expand_section(ctx, sinfo, &pinfo);
    if (rv == false)
        goto done;

    /* Write out new ELF file */
    rv = export_elf_file(ctx, outfile, &pinfo);

    /* Cleanup */
done:
    if (sinfo)
        free(sinfo);
    unload_elf(ctx);
    return (rv == false);
}
