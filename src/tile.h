#ifndef TILE_C
#define TILE_C

#include "aerial-berlin.h"

typedef struct _node {
    char *base;
    char *file;
    struct _node *next;
} List;

List *gather_files(const char *directory);
void delete_list(List *root);

int check_outdir(const char *directory);

void tile_files(List *files, const options *option);

#endif // TILE_C