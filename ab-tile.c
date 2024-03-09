#include <curl/multi.h>
#include <gdal/cpl_error.h>
#include <stdio.h>
#include <dirent.h>
#include <getopt.h>
#include <gdal/gdal.h>
#include <gdal/cpl_conv.h>
#include <stdlib.h>
#include <unistd.h>

#include "src/aerial-berlin.h"
#include "src/tile.h"

int main(int argc, char **argv) {
    options *opts = create_options();
    int opt;
    const char *shortopts = "+p:r:c:qvh";
    const struct option longopts[] = {
        {"prefix",  required_argument,  NULL,   'p'},
        {"row",     required_argument,  NULL,   'r'},
        {"column",  required_argument,  NULL,   'c'},
        {"quiet",   no_argument,        NULL,   'q'},
        {"version", no_argument,        NULL,   'v'},
        {"help",    no_argument,        NULL,   'h'},
        {0,         0,                  0,      0}
    };

    while ((opt = getopt_long(argc, argv, shortopts, longopts, NULL)) != -1) {
        switch (opt) {
            case 'p':
                opts->prefix = optarg;
                break;
            case 'r':
                opts->rsize = atoi(optarg);
                if (opts->rsize == 0) {
                    fprintf(stderr, "ERROR: Either specified 0 as number of rows per tile or conversion of '%s' to integer failed\n", optarg);
                    destroy_options(opts);
                }
                break;
            case 'c':
                opts->csize = atoi(optarg);
                if (opts->csize == 0) {
                    fprintf(stderr, "ERROR: Either specified 0 as number of columns per tile or conversion of '%s' to integer failed\n", optarg);
                    destroy_options(opts);
                }
                break;
            case 'q':
                opts->quiet = 1;
                break;
            case 'v':
                print_version();
                destroy_options(opts);
                return 0;
            case 'h':
                print_tile_help();
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

    if (!opts->quiet)
        print_options(opts);

    List *file_list = gather_files(opts->indir);

    if (check_outdir(opts->outdir)) {
        fprintf(stderr, "ERROR: Could not access directory '%s'\n", opts->outdir);
        delete_list(file_list);
        destroy_options(opts);
        return 1;
    }
    
    tile_files(file_list, opts);

    delete_list(file_list);
    destroy_options(opts);
    return 0;
}