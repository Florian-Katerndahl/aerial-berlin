#! /bin/bash

set -e

show_help() {
  cat <<HELP
Usage: aerial-images [-t|--type] [-y|--year] [-r|--regions] [-p|--png] [-q|--quiet] [-v|--version] [-h|--help] output-directory

Optional parameters and flags:
  -t|--type       Indicating if RGB, CIR or both datasets should be downloaded. Default: RGB
                  For 2021 and 2023, the data is offered as four band stack (RGBI). Thus, this option is ignored when year = 2021 | 2023.
                  For 1928, the data is offered in grayscale only. Thus, this option is ignored when year = 1928.
  -y|--year       Indicating which year's images should be downloaded. Possible values: 1928, 2020, 2021, 2023. Default: 2020
  -r|--regions    Indicating regions to download. Possible values: Mitte, Nord, Nordost, Nordwest, Ost, Sued, Suedost, Suedwest, West. Default: all
  -p|--png        Indicating if the tiled GeoTiffs get converted to PNG. If not present: False
  -q|--quiet      Suppress outputs. Default, if not present: False.
  -v|--version    Print version and exit.
  -h|--help       Print this help and exit.

Positional arguments:
  output-directory Path, where all final and intermediate outputs should be saved. May not exist prior to invocation.

Copyright: Florian Katerndahl (2023)
HELP
}

show_version() {
  cat <<VERSION
Version: 0.0.1
VERSION
}

check_programs() {
  EXIT=0

  PROGRAMS=(wget unzip docker find convert gdalbuildvrt getopt)

  for p in ${PROGRAMS[@]}; do
    RETURN=$(command -v $p >/dev/null 2>&1; echo $?)
    if [ $RETURN -ne 0 ]; then
      echo ERROR: Missing program $p
      EXIT=1
    fi
  done

  if [ $EXIT -eq 1 ];then
    exit 1
  fi
}

create_directories() {
  DIRS=("raw-data" "regions" "tiles")
  for dir in ${DIRS[@]}; do
    if [ ! -d "$1/$dir" ]; then
      echo creating "$1/$dir"
      mkdir "$1/$dir"
    fi

    if [ ! -d "$1/$dir" ]; then
      echo ERROR: Failed to create directory $dir
      exit 1
    fi
  done
}

tile_regions() {
  parallel gdal_retile.py -ps 512 512 -overlap 0 -ot "Byte" -r "near" -co "COMPRESS=LZW" -co "PREDICTOR=2" -s_srs "EPSG:25833" -targetDir "$1/tiles" {} ::: $(find "$1/regions" -name "*tif" -type f)
  return 0
}

create_stack() {
  return 0
}

single_convert() {
  local INPATH=$1
  local OUTPATH="${INPATH/tif/png}"
  convert -quiet $INPATH $OUTPATH
}

export -f single_convert

convert_tiles_to_png() {
  parallel single_convert {} ::: $(find "$1/tiles" -maxdepth 1 -type f -name "*tif")
  return 0
}

build_vrt() {
  find "$1/tiles" -type f -name "*.tif" -fprint "$1/vrt_list.txt"
  gdalbuildvrt -input_file_list "$1/vrt_list.txt" "$1/output.vrt"
  rm "$1/vrt_list.txt"
  return 0
}

pre_process_tiles() {
  local BASE_NAME=$(basename $1)

  wget --quiet --content-disposition --directory-prefix "$2" "$4/${dataset}.${SUFFIX}"

  unzip -qq "$2/${dataset}.${SUFFIX}" -d "$2/${BASE_NAME}"
  
  docker run --rm -u$(id -u):$(id -g) -v $2/${BASE_NAME}:/data floriankaterndahl/ecw2tiff \
    /bin/bash -c $'find /data -maxdepth 1 -type f \( -name \'*jp2\' -o -name \'*ecw\' \) -print0 | xargs -0 -I{} bash -c \'gdal_translate -ot "Byte" -a_srs "EPSG:25833" -co "COMPRESS=LZW" -co "PREDICTOR=2" $1 $(sed "s/\(ecw\|jp2\)/tif/" <<< $1)\' bash {}'

  mv $2/${BASE_NAME}/*tif $3

  rm -r "$2/${BASE_NAME}"

  rm -r "$2/${dataset}.${SUFFIX}"

  return 0
}

if [ $# -eq  0 ]; then
  show_help
  exit 1
fi

check_programs

# parse cli flags
TYPE=("RGB")
YEAR="2020"
REGIONS=("Mitte" "Nord" "Nordost" "Nordwest" "Ost" "Sued" "Suedost" "Suedwest" "West")
PNG=0
QUIET=0

OPTIONS=$(getopt -n $0 -o t:y:r:pqvh -l type:,year:,regions:,png,quiet,version,help -- "$@")
if [ $? -ne 0 ]; then
  echo ERROR: Failed to parse options.
  exit 1
fi

set -- $OPTIONS
while : ; do
  case "$1" in
    -t|--type)
      TYPE_STR=$(echo "$2" | sed -e 's/,/ /g' | sed -e "s/'//g" | tr '[:upper:]' '[:lower:]')
      TYPE=( $TYPE_STR )
      shift ;; 
    -y|--year)
      YEAR=$(echo "$2" | sed -e "s/'//g")
      shift ;;
    -r|--regions)
      REGIONS_STR=$(echo "$2" | sed -e 's/,/ /g' | sed -e "s/'//g")
      REGIONS=( $REGIONS_STR )
      shift ;;
    -p|--png)
      PNG=1
      ;;
    -q|--quiet)
      QUIET=1
      ;;
    -v|--version)
      show_version
      exit 0 ;;
    -h|--help)
      show_help
      exit 0 ;;
    --)
      shift
      break ;; # separates optional from positional arguments
    *)
      echo WARNING: Skipping argument "$1"
      shift 
      break ;; # should be unreachable
  esac
  shift
done

OUT=$(echo ${@: -1} | sed -e "s/'//g")

if [ $OUT == "--" ]; then
  echo ERROR: Failed to specifiy output-directory
  exit 1
fi

# create output directory and check if it was successfull
if [ ! -d $OUT ]; then
  echo Creating output-directory $OUT
  mkdir -p $OUT
fi

if [ ! -d $OUT ]; then
  echo ERROR: Failed to create "output-directory"
  exit 1
fi

# create re-usable base path variable
if [ ${OUT:0:1} != "/" ]; then
  BASE_PATH="$PWD/$OUT"
else
  BASE_PATH=$OUT
fi

create_directories $BASE_PATH

SUFFIX="zip"
N_DATASETS=${#REGIONS[*]}
PROCESSED=0

#echo ${REGIONS[*]}

for type in "${TYPE[@]}"; do
  if [ $YEAR -eq 2023 ]; then
    type="rgbi"
  elif [ $YEAR -eq 2021 ]; then
    type="rgb"
  fi
  BASE_URL=$(printf "https://fbinter.stadt-berlin.de/fb/atom/DOP/dop20true_%s_%d" $type $YEAR)
  if [ $YEAR -eq 1928 ]; then
	  BASE_URL="https://fbinter.stadt-berlin.de/fb/atom/luftbilder/1928"
  fi
  for dataset in "${REGIONS[@]}"; do
    pre_process_tiles "${dataset}" "${BASE_PATH}/raw-data" "${BASE_PATH}/regions" $BASE_URL
  done
done

tile_regions $BASE_PATH

# create_stack "${BASE_PATH}/tiles"

build_vrt $BASE_PATH

if [ $PNG -eq 1 ]; then
  convert_tiles_to_png $BASE_PATH
fi

exit 0
