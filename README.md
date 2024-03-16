# Aerial Berlin

The programs `ab-download`, `ab-tile` and `ab-convert` are a re-implementation of `aeria-images.sh` in C. They offer convenient ways of downloading, tiling and finally converting aerial imagery publically available of Berlin. While the programs are feature-complete (more or less), this is still a *toy project* and used as a learning exercise. Errors, Bugs and other forms of malfunctioning should be expected - even though all programs are compiled with runtime sanitizers.

## Installation

### Installing Dependencies

Aerial Berlin depends on `libcurl`, `libgdal` and `libpng`. Because aerial imagery of Berlin is distributed in the ECW file format, which is proprietary, you most likely need to re-compile GDAL with support for the ECW driver. To check if your installation of GDAL already supports reading ECW files, run the following command:

```bash
gdalinfo --formats | grep ECW
```

To re-compile GDAL, follow the instructions listed [here](https://github.com/bogind/libecwj2-3.3). Note that one of the URLs does not work anymore, a fix is provided in this [GitHub repo](https://github.com/erasta/libecwj2-3.3/tree/patch-1). However, all paths and filenames are handled as ASCII. Thus, support for multi-byte characters is not needed anyway. You can use any version of GDAL you want (and it's preferable to install a version newer than 2.2.2). GDAL 3.4 was the last major release using Makefiles, later releases switched to CMake which is configured differently.

In case your library/inculde paths are different, you may need to change them accordingly in the Makefile.

```bash
make
sudo make install
make clean
```

## Code Styling

The `astyle` formatting options can be found in `.astyle`. To reformat any C files, run the following command

```bash
astyle --project=.astyle --recursive "./*.c,*.h"
```
