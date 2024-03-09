#include <dirent.h>
#include <errno.h>
#include <gdal/cpl_error.h>
#include <gdal/ogr_srs_api.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <gdal/gdal.h>
#include <gdal/cpl_conv.h>
#include <stdint.h>
#include <strings.h>

#include "tile.h"

List *gather_files(const char *directory) {
    int written;
    List *root = NULL;
    DIR *root_dir = opendir(directory);
    struct dirent *d_entry = NULL;

    errno = 0;
    if (root_dir == NULL) {
        switch (errno) {
            case EACCES:
                fprintf(stderr, "ERROR: Permission to access '%s' not allowed\n", directory);
                break;
            case ENOENT:
                fprintf(stderr, "ERROR: Directory does not exist or you supplied an empty argument.\n");
                break;
            case ENOTDIR:
                fprintf(stderr, "ERROR: '%s' is not a directory\n", directory);
                break;
            default:
                break;
        }
    }

    while ((d_entry = readdir(root_dir))) {
        if (d_entry->d_type != DT_LNK && d_entry->d_type != DT_REG)
            continue;
        List *node = malloc(sizeof(List));
        node->next = root;
        node->file = calloc(1024, sizeof(char));
        if (node->file == NULL) {
            fprintf(stderr, "ERROR: Failed to allocate memory for file list.\n");
            closedir(root_dir);
            delete_list(root);
            return NULL;
        }
        written = snprintf(node->file, 1024, "%s%s%s",
                           directory,
                           directory[strlen(directory) - 1] == '/' ? "" : "/",
                           d_entry->d_name);
        if (written >= 1024) {
            fprintf(stderr, "ERROR: File path longer than 1024 bytes\n");
            closedir(root_dir);
            delete_list(root);
            return NULL;
        }
        node->base = calloc(1024, sizeof(char));
        if (node->base == NULL) {
            fprintf(stderr, "EROR: Failed to allocate memory for base dir in file list\n");
            closedir(root_dir);
            delete_list(root);
            return NULL;
        }
        written = snprintf(node->base, 1024, "%s", d_entry->d_name);
        if (written >= 1024) {
            fprintf(stderr, "ERROR: File path longer than 1024 bytes\n");
            closedir(root_dir);
            delete_list(root);
            return NULL;
        }
        char *file_extension = rindex(node->base, '.');
        if (file_extension == NULL) {
            fprintf(stderr, "ERROR: Supplied file name withouth file extension\n");
            closedir(root_dir);
            delete_list(root);
            return NULL;
        }
        *file_extension = '\0';

        root = node;
    }

    closedir(root_dir);
    return root;
}

void delete_list(List *root) {
    List *tmp = root;
    while (root) {
        tmp = root;
        root = root->next;
        free(tmp->base);
        free(tmp->file);
        free(tmp);
    }
}

int check_outdir(const char *directory) {
    DIR *dir = opendir(directory);
    if (dir == NULL) 
        return 1;
    closedir(dir);
    return 0;
}

void tile_files(List *files, const options *option) {
    int written_chars;
    GDALAllRegister();

    while (files) {
        int x_chunk = 0;
        int y_chunk = 0;
        if (strstr(files->file, ".jp2") == NULL && strstr(files->file, ".ecw") == NULL) {
            files = files->next;
            continue;
        }
        if (!option->quiet)
            printf("Processing %s\n", files->file);
        
        GDALDatasetH raster_file = GDALOpen(files->file, GA_ReadOnly);
        if (raster_file == NULL) {
            fprintf(stderr, "ERROR: Failed to open file '%s'\n", files->file);
        }

        int nbands = GDALGetRasterCount(raster_file);
        int columns = GDALGetRasterXSize(raster_file);
        int rows = GDALGetRasterYSize(raster_file);
        double geo_transform[6];  

        if (columns % option->csize != 0) {
            fprintf(stderr, "ERROR: Columns are not evenly divisible by %d.\n", option->csize);
            // TODO proper cleanup
            exit(69);
        }
        if (rows % option->rsize != 0) {
            fprintf(stderr, "ERROR: Rows are not evenly divisible by %d.\n", option->rsize);
            // TODO proper cleanup
            exit(69);
        }

        if (GDALGetGeoTransform(raster_file, geo_transform) != CE_None) {
            fprintf(stderr, "ERROR: Could not read geo transform\n");
            // TODO proper cleanup
            exit(69);
        }

        uint8_t **data = malloc(sizeof(uint8_t *) * nbands);
        for (int i = 0; i < nbands; i++) data[i] = CPLMalloc(columns * rows * sizeof(uint8_t));
        
        GDALRasterBandH hband;
        GDALDataType dtype;
        for (int band = 1; band <=nbands; band++) {
            hband = GDALGetRasterBand(raster_file, band);
            dtype = GDALGetRasterDataType(hband);
            if (dtype != GDT_Byte) {
                fprintf(stderr, "ERROR: Unexpected data type: %s\n", GDALGetDataTypeName(dtype));
                // TODO proper cleanup
                exit(69);
            }
            CPLErr IOErr = GDALRasterIO(hband, GF_Read, 0, 0, columns, rows, data[band - 1], columns, rows, dtype, 0, 0);
            if (IOErr != CE_None) {
                fprintf(stderr, "ERROR: Encountered I/O error\n");
                // TODO proper cleanup
                exit(69);
            }
        }

        char *outpath = malloc(1024 * sizeof(char));  // TODO check return value
        char **creation_options = NULL;

        // since original data does not include projection reference, need to create our own. Hard-coded EPSG:25833
        OGRSpatialReferenceH spat_ref = OSRNewSpatialReference(NULL);
        OSRImportFromEPSGA(spat_ref, 25833);
        char *projection_ref = NULL;
        OSRExportToWkt(spat_ref, &projection_ref);
        OSRDestroySpatialReference(spat_ref);
        for (int x = 0; x < columns; x += option->csize) {
            memset(outpath, 0, 1024);
            y_chunk = 0;
            for(int y = 0; y < rows; y += option->rsize) {
                written_chars = snprintf(outpath, 1024, "%s%s%s-%s-X%.4d_Y%.4d.tif", 
                                         option->outdir,
                                         option->outdir[strlen(option->outdir) -1] == '/' ? "" : "/",
                                         option->prefix,
                                         files->base,
                                         x_chunk, y_chunk);
                if (written_chars >= 1024) {
                    fprintf(stderr, "ERROR: Output file path to long.\n");
                    // TODO proper cleanup
                    exit(69);
                }

                GDALDatasetH out_dataset = GDALCreate(GDALGetDriverByName("GTiff"), outpath, option->csize, option->rsize, nbands, dtype, creation_options);
                
                GDALSetGeoTransform(out_dataset, geo_transform); // TODO simply copying this value is wrong as it holds coordinates from top pixels
                GDALSetProjection(out_dataset, projection_ref);

                GDALRasterBandH *out_bands = malloc(sizeof(GDALRasterBandH *) * nbands); // TODO check return value
                for (int i = 1; i <= nbands; i++) {
                    out_bands[i - 1] = GDALGetRasterBand(out_dataset, i);
                    CPLErr write_error =
                      GDALRasterIO(out_bands[i - 1], GF_Write, 0, 0,option->csize, option->rsize,
                                   &data[i - 1][x + y * columns],
                                   option->csize, option->rsize, dtype, 0, columns);
                  if (write_error != CE_None) {
                    fprintf(stderr, "ERROR: Could not write raster band\n");
                    // TODO proper cleanup
                    exit(69);
                    }
                }

                GDALClose(out_dataset);
                free(out_bands);
                geo_transform[3] += option->rsize * geo_transform[5]; // north-up image is assumed
                y_chunk++;
            }
            geo_transform[0] += option->csize * geo_transform[1]; // north-up image is assumed
            geo_transform[3] -= ((double) columns / option->rsize) * (option->rsize * geo_transform[5]);
            x_chunk++;
        }
        CPLFree(projection_ref);
        for (int i = 0; i < nbands; i++) CPLFree(*(data + i));
        free(data);
        GDALClose(raster_file);
        free(outpath);

        files = files->next;
    }
}
