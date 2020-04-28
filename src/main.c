#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <getopt.h>
#include "elfio.h"
#include "parse.h"
#include "slackinfo.h"
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

static void display_injectable(struct slackinfo *sinfo)
{
    struct slackinfo *curr = sinfo;
    printf(INFO "Finding injectable sections ...\n");
    while (curr != NULL) {
        if (!curr->is_exec || !curr->slackspace) {
            curr = curr->next;
            continue;
        }

        /* Display generic section information */
        printf("  -- %s\n", curr->name);
        printf("    -- offset      : 0x%08x\n", *curr->offset);
        printf("    -- size        : 0x%08x\n", *curr->size);
        printf("    -- slack space : %lu bytes\n", curr->slackspace);
        curr = curr->next;
    }
    printf("\n");
}

int main(int argc, char **argv)
{
    int opt;
    char *infile = NULL;
    char *outfile = NULL;
    bool verbose = false;
    bool analyze = true;
    bool rv;
    struct slackinfo *sinfo = NULL;
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

    /* Parse section headers */
    rv = parse_section_headers(ctx, &sinfo);
    if (rv == false)
        goto done;

    /* Display sections that are valid targets for injection */
    display_injectable(sinfo);

    /* If we're just analyzing, bail out */
    if (analyze)
        goto done;

    /* Expand the section by name */
    rv = expand_section_by_name(ctx, sinfo, ".text", &pinfo);
    if (rv == false)
        goto done;

    /* Write out new ELF file */
    rv = export_elf_file(ctx, outfile, &pinfo);

    /* Cleanup */
done:
    if (sinfo)
        free_slackinfo_list(sinfo);
    unload_elf(ctx);
    return (rv == false);
}
