#ifndef AERIAL_BERLIN_H
#define AERIAL_BERLIN_H

#ifndef _DEFAULT_SOURCE
#define _DEFAULT_SOURCE
#include <stdlib.h>
#endif // _DEFAULT_SOURCE

#define VERSION "v0.0.2024.1"

extern char *base_url;

typedef struct {
    size_t type_count;
    char **requested_type;
    size_t region_count;
    char **requested_region;
    size_t year_count;
    int *year;
    int allow_non_rectified;
    int convert_to_png;
    int quiet;
    char *prefix;
    int rsize;
    int csize;
    char *indir;
    char *outdir;
} options;

void print_download_help(void);
void print_tile_help(void);
void print_version(void);
void print_options(const options *option);

options *create_options(void);
void destroy_options(options *option);

int parse_image_types(options *option, const char *optstring);
int parse_image_years(options *option, const char *optstring);
int parse_image_regions(options *option, const char *optstring);



#endif // AERIAL_BERLIN_H