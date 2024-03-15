#include <stdio.h>
#include <stdint.h>
#include <getopt.h>
#include <stdlib.h>

#include "src/aerial-berlin.h"
#include "src/tile.h"

// user needs to choose three bands bc png only supports three bands

int main(int argc, char **argv) {
    options *opts = create_options();

    int opt;
    const char *shortopts = "+b:qvh";
    const struct option longopts[] = {
        {"bands",   required_argument,  NULL,   'b'},
        {"quiet",   no_argument,        NULL,   'q'},
        {"version", no_argument,        NULL,   'v'},
        {"help",    no_argument,        NULL,   'h'},
        {0,         0,                  0,      0}
    };

    while ((opt = getopt_long(argc, argv, shortopts, longopts, NULL)) != -1) {
        switch (opt) {
            case 'b':
                if (parse_bands(opts, optarg))
                    exit(EXIT_FAILURE);
                break;
            case 'q':
                opts->quiet = 1;
                break;
            case 'v':
                print_version();
                destroy_options(opts);
                return 0;
            case 'h':
                print_convert_help();
                destroy_options(opts);
                return 0;
            case '?':
                break;
        }
    }

    if (argc - optind == 2) {
        opts->indir = argv[optind++];
        opts->outdir = argv[optind++];
    }
    else {
        fprintf(stderr, "ERROR: Expected 2 positional argument: input directory and output directory. Found %d\n", argc - optind);
        destroy_options(opts);
        return 1;
    }
    
    if (check_outdir(opts->outdir)) {
        fprintf(stderr, "ERROR: Could not access directory '%s'\n", opts->outdir);
        destroy_options(opts);
        return 1;
    }

    List *files = gather_files(opts->indir);

    convert_files(files, opts);
    delete_list(files);
    destroy_options(opts);
    return 0;
}