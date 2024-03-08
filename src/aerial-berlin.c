#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <curl/curl.h>

#include "aerial-berlin.h"

char *base_url = "https://fbinter.stadt-berlin.de/fb/atom";

void print_help(void) {
    printf(
        "Usage: aerial-images [-t|--type] [-y|--year] [-r|--regions] [-p|--png] [-q|--quiet] [-v|--version] [-h|--help] output-directory\n\n"
        "Optional parameters and flags:\n"
        "\t-t|--type       Indicating if RGB, CIR or Grayscale datasets should be downloaded.\n"
        "\t                For 2021 and 2023, the data is offered as four band stack (RGBI). Thus, this option is ignored when year = 2021 | 2023.\n"
        "\t                For 1928, the data is offered in grayscale only. Thus, this option is ignored when year = 1928.\n"
        "\t-y|--year       Indicating which year's images should be downloaded. Possible values: 1928, 2020, 2021, 2023.\n"
        "\t-r|--regions    Indicating regions to download. Possible values: Mitte, Nord, Nordost, Nordwest, Ost, Sued, Suedost, Suedwest, West.\n"
        "\t-p|--png        Indicating if the tiled GeoTiffs get converted to PNG. If not present: False\n"
        "\t-q|--quiet      Suppress outputs. Default, if not present: False.\n"
        "\t-v|--version    Print version and exit.\n"
        "\t-h|--help       Print this help and exit.\n\n"
        "Positional arguments:\n"
        "\toutput-directory Path, where all final and intermediate outputs should be saved. May not exist prior to invocation.\n\n"
        "Copyright: Florian Katerndahl (2023-*)\n"
    );
}

void print_version(void) {
    printf("version: %s\n", VERSION);
}

options *create_options(void) {
    options *option = calloc(1, sizeof(options));
    if (option == NULL) {
        fprintf(stderr, "ERROR: Failed to allocate options.\n");
        exit(EXIT_FAILURE);
    }

    return option;
}

void destroy_options(options *option) {
    for (size_t region = 0; region < option->region_count; region++)
        free(option->requested_region[region]);
    for (size_t type = 0; type < option->type_count; type++)
        free(option->requested_type[type]);

    free(option->year);
    free(option->requested_region);
    free(option->requested_type);
    free(option);
    return;
}

int parse_image_types(options *option, const char *optstring) {
    char **newmem;
    size_t length;
    char *ptr = (char*) optstring;
    char *endptr = NULL;
    
    const char *allowed_types[] = { "RGB", "CIR", "Grayscale", "RGBI" };

    while (*ptr) {
        endptr = strchr(ptr, ',');
        if (endptr == NULL) {
            endptr = ptr;
            while (*endptr)
                endptr++;
        }
        length = endptr - ptr;
        
        option->type_count++;
        newmem = realloc(option->requested_type, option->type_count * sizeof(char *));
        if (newmem == NULL) {
            fprintf(stderr, "ERROR: Failed to allocate memory for requested types array.\n");
            return 1;
        }
        option->requested_type = newmem;
        option->requested_type[option->type_count - 1] = malloc(sizeof(char) * (length + 1));
        if (option->requested_type[option->type_count - 1] == NULL) {
            fprintf(stderr, "ERROR: Failed to allocate memory for requested type.\n");
            return 1;
        }
        strncpy(option->requested_type[option->type_count - 1], ptr, length);
        option->requested_type[option->type_count - 1][length] = '\0';

        if (*endptr == '\0')
            break;

        ptr = endptr;
        ptr++;
    }

    int match;
    for (size_t i = 0; i < option->type_count; i++) {
        match = 0;
        for (size_t j = 0; j < sizeof(allowed_types)/sizeof(allowed_types[0]); j++)
            if (strcmp(option->requested_type[i], allowed_types[j]) == 0) {
                match = 1;
                break;
            }
        if (!match) {
            fprintf(stderr, "ERROR: Image type '%s' not allowed.\n", option->requested_type[i]);
            return 1;
        }
    }

    return 0;
}

int parse_image_years(options *option, const char *optstring) {
    int *newmem;
    long int year;
    char *ptr = (char*) optstring;
    char *endptr = ptr;
    errno = 0;

    const int allowed_years[] = { 1928, 2020, 2021, 2023 };
    
    while ((year = strtol(ptr, &endptr, 10)) != LONG_MIN && year != LONG_MAX) {
        if (year == 0 && *ptr == *endptr)
            return 1;
        option->year_count++;
        newmem = realloc(option->year, option->year_count * sizeof(int));
        if (newmem == NULL) {
            fprintf(stderr, "ERROR: Failed to allocate memory for requested years.\n");
            return 1;
        }
        option->year = newmem;
        option->year[option->year_count - 1] = (int) year;

        if (*endptr == '\0' && *ptr != '\0')
            break;

        ptr = endptr;
        ptr++;
    }

    if (errno == ERANGE) {
        fprintf(stderr, "ERROR: Year conversion resulted in under-/overflow\n");
        return 1;
    }

    int match;
    for (size_t j = 0; j < option->year_count; j++) {
        match = 0;
        for (size_t i = 0; i < sizeof(allowed_years)/sizeof(allowed_years[0]); i++) {
            if (option->year[j] == allowed_years[i]) {
                match = 1;
                break;
            }
        }
        if (!match) {
            fprintf(stderr, "ERROR: Supplied year not allowed.\n");
            return 1;
        }
    }

    return 0;
}

int parse_image_regions(options *option, const char *optstring) {
    char **newmem;
    size_t length;
    char *ptr = (char*) optstring;
    char *endptr = NULL;
    
    const char *allowed_regions[] = { "Mitte", "Nord", "Nordost", "Nordwest", "Ost", "Sued", "Suedost", "Suedwest", "West" };
    
    while (*ptr) {
        endptr = strchr(ptr, ',');
        if (endptr == NULL) {
            endptr = ptr;
            while (*endptr)
                endptr++;
        }
        length = endptr - ptr;

        if (strncmp("all", ptr, length) == 0) {
            option->region_count = sizeof(allowed_regions) / sizeof(allowed_regions[0]);
            newmem = realloc(option->requested_region, option->region_count * sizeof(char *));
            if (newmem == NULL) {
            fprintf(stderr, "ERROR: Failed to allocate memory for requested regions array.\n");
            return 1;
            }
            option->requested_region = newmem;
            for (size_t i = 0; i < option->region_count; i++) {
                 option->requested_region[i] = malloc(sizeof(char) * (strlen(allowed_regions[i]) + 1));
                 if (option->requested_region[i] == NULL) {
                    fprintf(stderr, "ERROR: Failed to allocate memory for requested region.\n");
                    return 1;
                }
                 strcpy( option->requested_region[i], allowed_regions[i]);
            }
            return 0;
        }
        
        option->region_count++;
        newmem = realloc(option->requested_region, option->region_count * sizeof(char *));
        if (newmem == NULL) {
            fprintf(stderr, "ERROR: Failed to allocate memory for requested regions array.\n");
            return 1;
        }
        option->requested_region = newmem;
        option->requested_region[option->region_count - 1] = malloc(sizeof(char) * (length + 1));
        if (option->requested_region[option->region_count - 1] == NULL) {
            fprintf(stderr, "ERROR: Failed to allocate memory for requested region.\n");
            return 1;
        }
        strncpy(option->requested_region[option->region_count - 1], ptr, length);
        option->requested_region[option->region_count - 1][length] = '\0';

        if (*endptr == '\0')
            break;

        ptr = endptr;
        ptr++;
    }

    int match;
    for (size_t i = 0; i < option->region_count; i++) {
        match = 0;
        for (size_t j = 0; j < sizeof(allowed_regions) / sizeof(allowed_regions[0]); j++)
            if (strcmp(option->requested_region[i], allowed_regions[j]) == 0) {
                match = 1;
                break;
            }
        if (!match) {
            fprintf(stderr, "ERROR: Image type '%s' not allowed.\n", option->requested_region[i]);
            return 1;
        }
    }

    return 0;
}

void print_options(const options *option) {
    printf("Options:\n");
    
    printf("\tRequested image types: ");
    for (size_t i = 0; i < option->type_count; i++)
        printf("%s ", option->requested_type[i]);
    printf("\n");
    
    printf("\tRequested image years: ");
    for (size_t i = 0; i < option->year_count; i++)
        printf("%d ", option->year[i]);
    printf("\n");

    printf("\tRequested image regions: ");
    for (size_t i = 0; i < option->region_count; i++)
        printf("%s ", option->requested_region[i]);
    printf("\n");

    printf("\tConvert tiles to PNG: %d\n", option->convert_to_png);

    printf("\tOutput directory: %s\n", option->outdir);

    return;
}