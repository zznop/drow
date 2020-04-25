#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <getopt.h>
#include "elfio.h"
#include "parse.h"
#include "slackinfo.h"

static void print_help(void)
{
    printf(
        "[v0.0.1]\n" // TODO: Use preprocessor/git describe version
        "drow [-options] elffile\n"
        "options:\n"
        "  -a         Analyze ELF and display segment information and available space\n"
        "  -s file    Stuff a payload blob in available space\n"
    );
}

int main(int argc, char **argv)
{
    int opt;
    char *elffile = NULL;
    bool verbose = false;
    bool analyze = true;
    bool rv;
    struct slackinfo *sinfo = NULL;
    drow_ctx_t *ctx;

    if (argc < 2) {
        print_help();
        return 0;
    }

    while (optind < argc) {
        if ((opt = getopt(argc, argv, "hs:v")) != -1) {
            switch (opt) {
            case 'h':
                print_help();
                return 0;
            case 's':
                printf("Not implemented\n");
                analyze = false;
                return 1;
            case 'v':
                verbose = true;
                (void)verbose;
                break;
            default:
                print_help();
                return 1;
            }
        } else {
            if (elffile != NULL) {
                print_help();
                return 1;
            }

            elffile = argv[optind];
            optind++;
        }
    }

    if (elffile == NULL) {
        fprintf(stderr, "no elf file\n");
        return 1;
    }

    rv = load_elf(&ctx, elffile);
    if (rv == false)
        return 1;

    if (analyze)
        rv = find_slackspace(ctx, &sinfo);

    /* Cleanup */
    printf("Cleaning up..\n");
    if (sinfo)
        free_slackinfo_list(sinfo);
    unload_elf(ctx);

    return (rv == false);
}
