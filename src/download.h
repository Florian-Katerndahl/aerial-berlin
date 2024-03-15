#ifndef _DOWNLOAD_H
#define _DOWNLOAD_H

#include "aerial-berlin.h"

// canonical implementation would be to have an head and previous pointer and using them to enqueue/dequeue
typedef struct Node
{
  int  *year;
  char *region;
  char *type;
  int  *non_rectified;
  struct Node *oldest;
  struct Node *prev;
} Node;

Node *queue_from_options(const options *option);

Node *enqueue(Node *queue, int year, char *region, char *type, int non_rectified);

Node *dequeue(Node **queue);

void download_datasets(Node *queue, const char *to, const int quiet);

#endif // _DOWNLOAD_H