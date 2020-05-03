#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <getopt.h>
#include "elfio.h"
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
        "[v0.0.1]\n" // TODO: Use preprocessor/git describe version
        "drow [options] infile payload outfile\n"
        "options:\n"
        "  -h    display usage\n"
    );
}

int main(int argc, char **argv)
{
    int opt;
    char *infile = NULL;
    char *outfile = NULL;
    bool verbose = false;
    bool rv;
    struct shinfo *sinfo = NULL;
    struct patchinfo pinfo = {0};
    char *payload_file = NULL;
    int i = 1;
    payload_t *payload;
    drow_ctx_t *ctx;

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
            case 'v':
                verbose = true;
                (void)verbose;
                break;
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
                payload_file = argv[i];
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

    if (payload_file == NULL) {
        fprintf(stderr, "no payload file\n");
        return 1;
    }

    if (outfile == NULL) {
        fprintf(stderr, "no out ELF file path\n");
        return 1;
    }

    print_banner();

    /* Map in the ELF */
    rv = load_elf(&ctx, infile);
    if (rv == false)
        return 1;

    /* Map in payload */
    rv = load_payload(&payload, payload_file);
    if (rv == false)
        goto done;

    printf(INFO "Finding last section in executable segment ...\n");
    sinfo = find_exe_seg_last_section(ctx);
    if (sinfo == NULL) {
        rv = false;
        fprintf(stderr, ERR "Failed to find last section in executable segment!?\n");
        goto done;
    }
    printf(SUCCESS "Found %s at 0x%08x with a size of %u bytes\n", sinfo->name, *sinfo->offset, *sinfo->size);

    /* Expand the section by name */
    rv = expand_section(ctx, sinfo, &pinfo);
    if (rv == false)
        goto done;

    /* Write out new ELF file */
    rv = export_elf_file(ctx, payload, outfile, &pinfo);
    if (rv == true)
        printf(SUCCESS "ELF patched successfully!\n");
    else
        printf(ERR "Failed to patch ELF file\n");

    /* Cleanup */
done:
    if (sinfo)
        free(sinfo);
    if (ctx)
        unload_elf(ctx);
    return (rv == false);
}
