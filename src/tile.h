#ifndef TILE_C
#define TILE_C

#include "aerial-berlin.h"

#define BYTES_PER_PIXEL 3

#define RED(X)   (X + 0)
#define GREEN(X) (X + 1)
#define BLUE(X)  (X + 2)

typedef struct _node
{
  char *base;
  char *file;
  struct _node *next;
} List;

List *gather_files(const char *directory);

void delete_list(List *root);

int check_dir(const char *directory);

void tile_files(List *files, const options *option);

void convert_files(List *files, const options *option);

#endif // TILE_C