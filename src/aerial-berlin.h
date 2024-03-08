#ifndef AERIAL_BERLIN_H
#define AERIAL_BERLIN_H

#ifndef _DEFAULT_SOURCE
#define _DEFAULT_SOURCE
#include <stdlib.h>
#endif // _DEFAULT_SOURCE

#define VERSION "v0.0.2024.1"

extern char *base_url;

typedef enum {
    RGB_IMAGE       = 0,
    CIR_IMAGE       = 1,
    RGBI_IMAGE      = 2,
    GRAYSCALE_IMAGE = 3,
    DEFAULT_IMAGE   = 4,
} PRODUCT_TYPE;

typedef enum {
    REGION_CENTER     = 0,
    REGION_NORTH      = 1,
    REGION_NORTHEAST  = 2,
    REGION_NORTHWEST  = 3,
    REGION_EAST       = 4,
    REGION_SOUTH      = 5,
    REGION_SOUTHEAST  = 6,
    REGION_SOUTHWEST  = 7,
    REGION_WEST       = 8,
    REGION_ALL        = 9,
} PRODUCT_REGION;

typedef struct {
    size_t type_count;
    char **requested_type;
    size_t region_count;
    char **requested_region;
    size_t year_count;
    int *year;
    int convert_to_png;
    int quiet;
    char *outdir;
} options;

void print_help(void);
void print_version(void);

options *create_options(void);
void destroy_options(options *option);

int parse_image_types(options *option, const char *optstring);
int parse_image_years(options *option, const char *optstring);
int parse_image_regions(options *option, const char *optstring);

void print_options(const options *option);

#endif // AERIAL_BERLIN_H