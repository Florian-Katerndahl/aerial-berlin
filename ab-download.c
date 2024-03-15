#include <curl/curl.h>
#include <curl/easy.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>

#include "src/aerial-berlin.h"
#include "src/download.h"

int main(int argc, char *argv[])
{
  options *request_opts = create_options();
  int opt;
  const char *shortopts = "+t:y:r:opqvh";
  const struct option longopts[] = {
    {"type",    required_argument,  NULL,   't'},
    {"year",    required_argument,  NULL,   'y'},
    {"region",  required_argument,  NULL,   'r'},
    {"ortho",   no_argument,        NULL,   'o'},
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
    case 'o':
      request_opts->allow_non_rectified = 1;
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
      print_download_help();
      destroy_options(request_opts);
      return 0;
    case '?':
      break;
    }
  }

  if (argc - optind == 1) {
    request_opts->outdir = argv[optind];
  } else {
    fprintf(stderr, "ERROR: Expected 1 positional argument: output directory. Found %d\n",
            argc - optind);
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

  Node *download_queue = queue_from_options(request_opts);
  download_datasets(download_queue, request_opts->outdir, request_opts->quiet);

  destroy_options(request_opts);
  curl_global_cleanup();

  return 0;
}
