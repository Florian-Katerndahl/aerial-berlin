#include <stdlib.h>
#include <unistd.h>
#include <curl/curl.h>
#include <curl/easy.h>

#include "download.h"
#include "aerial-berlin.h"

Node *queue_from_options(const options *option)
{
  Node *root = NULL;

  for (size_t year = 0; year < option->year_count; year++) {
    for (size_t type = 0; type < option->type_count; type++) {
      for (size_t region = 0; region < option->region_count; region++) {
        Node *new = malloc(sizeof(Node));
        if (new == NULL) {
          // TODO proper error management/memory freeing
          fprintf(stderr, "ERROR: Failed to allocate memory for download queueue entry.\n");
          exit(69);
        }
        new->year = &option->year[year];
        new->region = option->requested_region[region];
        new->type = option->requested_type[type];
        new->non_rectified = (int *) &option->allow_non_rectified;

        new->prev = NULL;
        if (root != NULL) {
          new->oldest = root->oldest;
          root->prev = new;
        } else {
          new->oldest = new;
        }
        root = new;
      }
    }
  }
  return root;
}

Node *dequeue(Node **queue)
{
  Node *item = (*queue)->oldest;
  (*queue)->oldest = item->prev;

  return item;
}

void download_datasets(Node *queue, const char *to, const int verbose)
{
  Node *item;

  CURL *handle = curl_easy_init();
  curl_easy_setopt(handle, CURLOPT_FAILONERROR, 1L);

  while (1) {
    item = dequeue(&queue);
    int written = 0;
    char *request_url = calloc(1024, sizeof(char));
    if (*item->year != 1928) {
      written = snprintf(request_url, 1024, "%s/DOP/dop20%s%s_%d/%s.zip",
                         base_url,
                         *item->non_rectified ? "" : "true_",
                         item->type,
                         *item->year,
                         item->region);
      if (written >= 1024) {
        fprintf(stderr, "ERROR: Request URL longer than 1024 bytes. Not performing request.\n");
        free(request_url);
        break;
      }
    } else {
      snprintf(request_url, 1024, "%s/luftbilder/1928/%s.zip",
               base_url,
               item->region);
    }

    if (curl_easy_setopt(handle, CURLOPT_URL, request_url) != CURLE_OK) {
      fprintf(stderr, "ERROR: Failed to set URL for request.\n");

      curl_easy_cleanup(handle);
      return;
    }

    char *out_path = calloc(1024, sizeof(char));
    written = snprintf(out_path, 1024, "%s/%d-%s-%s.zip",
                       to,
                       *item->year,
                       item->type,
                       item->region);

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
      if (verbose)
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
    if (result != CURLE_HTTP_RETURNED_ERROR)
      fclose(fout);
    if (item->oldest == NULL) {
      free(item);
      break;
    }
    free(item);
  }

  curl_easy_cleanup(handle);
}