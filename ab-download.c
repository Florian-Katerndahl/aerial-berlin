#include <curl/curl.h>
#include <curl/easy.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>

#include "src/aerial-berlin.h"

int main(int argc, char *argv[]) {
    options *request_opts = create_options();
    int opt;
    const char *shortopts = "+t:y:r:pqvh";
    const struct option longopts[] = {
        {"type",    required_argument,  NULL,   't'},
        {"year",    required_argument,  NULL,   'y'},
        {"region",  required_argument,  NULL,   'r'},
        {"png",     no_argument,        NULL,   'p'},
        {"quiet",   no_argument,        NULL,   'q'},
        {"version", no_argument,        NULL,   'v'},
        {"help",    no_argument,        NULL,   'h'},
        {0,         no_argument,        0,      0}
    };

    while ((opt = getopt_long(argc, argv, shortopts, longopts, NULL)) != -1) {
        switch (opt) {
            case 't':
                if (parse_image_types(request_opts, optarg)) {
                    fprintf(stderr, "ERROR: Failed to parse requested image types: '%s'\n", optarg);
                    destroy_options(request_opts);
                    return 1;
                }
                break;
            case 'y':
                if (parse_image_years(request_opts, optarg)) {
                    fprintf(stderr, "ERROR: Failed to parse requested image years: '%s'\n", optarg);
                    destroy_options(request_opts);
                    return 1;
                }
                break;
            case 'r':
                if (parse_image_regions(request_opts, optarg)) {
                    fprintf(stderr, "ERROR: Failed to parse requested image regions: '%s'\n", optarg);
                    destroy_options(request_opts);
                    return 1;
                }
                break;
            case 'p':
                request_opts->convert_to_png = 1;
                break;
            case 'q':
                request_opts->quiet = 1;
                break;
            case 'v':
                print_version();
                destroy_options(request_opts);
                return 0;
            case 'h':
                print_help();
                destroy_options(request_opts);
                return 0;
            case '?':
                break;
        }
    }

    if (argc - optind == 1) {
        request_opts->outdir = argv[optind];
    }
    else {
        fprintf(stderr, "ERROR: Expected 1 positional argument: output directory. Found %d\n", argc - optind);
        destroy_options(request_opts);
        return 1;
    }

    if (!request_opts->quiet)
        print_options(request_opts);

    switch (curl_global_init(CURL_GLOBAL_ALL)) {
        case CURLE_FAILED_INIT:
            fprintf(stderr, "ERROR: CURL failed to initialize properly\n");
            destroy_options(request_opts);
            return 1;
        default:
            break;    
    };

    CURL *handle = curl_easy_init();
    curl_easy_setopt(handle, CURLOPT_FAILONERROR, 1L);

    for (size_t year = 0; year < request_opts->year_count; year++) {
        for (size_t type = 0; type < request_opts->type_count; type++) {
            for (size_t region = 0; region < request_opts->region_count; region++) {
                int written = 0;
                char *request_url = calloc(1024, sizeof(char));
                if (request_opts->year[year] != 1928) {
                    written = snprintf(request_url, 1024, "%s/DOP/dop20true_%s_%d/%s.zip",
                                            base_url, 
                                            request_opts->requested_type[type], 
                                            request_opts->year[year],
                                            request_opts->requested_region[region]);
                    if (written >= 1024) {
                        fprintf(stderr, "ERROR: Request URL longer than 1024 bytes. Not performing request.\n");
                        free(request_url);
                        break;
                    }
                } else {
                    snprintf(request_url, 1024, "%s/luftbilder/1928/%s.zip",
                                            base_url,
                                            request_opts->requested_region[region]);
                }
                
                if (curl_easy_setopt(handle, CURLOPT_URL, request_url) != CURLE_OK) {
                    fprintf(stderr, "ERROR: Failed to set URL for request.\n");

                    destroy_options(request_opts);
                    curl_easy_cleanup(handle);
                    curl_global_cleanup();
                    return 1;
                }

                char *out_path = calloc(1024, sizeof(char));
                written = snprintf(out_path, 1024, "%s/%d-%s-%s.zip",
                                            request_opts->outdir,
                                            request_opts->year[year],
                                            request_opts->requested_type[type], 
                                            request_opts->requested_region[region]);

                if (written >= 1024) {
                    fprintf(stderr, "ERROR: Local output path is to long. Not performing request.\n");
                    free(request_url);
                    free(out_path);
                    break;
                }

                FILE *fout = fopen(out_path, "wb");
                if (fout == NULL) {
                    fprintf(stderr, "ERROR: Failed to open output file %s\n", out_path);
                    free(request_url);
                    free(out_path);
                    break;
                }

                curl_easy_setopt(handle, CURLOPT_WRITEDATA, (void *) fout);
                CURLcode result = curl_easy_perform(handle);

                switch (result) {
                    case CURLE_OK:
                        if (!request_opts->quiet)
                            printf("Sucessfully downloaded file '%s'\n", out_path);
                        break;
                    case CURLE_HTTP_RETURNED_ERROR:
                        printf("WARNING: Failed to download file '%s' from %s\n", out_path, request_url);
                        fclose(fout);
                        unlink(out_path);
                        break;                    
                    default:
                        fprintf(stderr, "WARNING: Uncaught CURL return %d\n", result);
                        break;
                }

                free(request_url);
                free(out_path);
                if (result != CURLE_HTTP_RETURNED_ERROR) fclose(fout);
            }
        }
    }
    
    destroy_options(request_opts);
    curl_easy_cleanup(handle);
    curl_global_cleanup();

    return 0;
}
