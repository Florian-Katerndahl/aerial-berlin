CC=gcc
CFLAGS=-Wall -Wextra -Wdouble-promotion -Wuninitialized -Winit-self -pedantic -flto -O0 -ggdb -fsanitize=undefined,address,leak
CSTD=--std=c2x
CURL=-lcurl
GDAL=-I/usr/include/gdal -L/usr/lib -lgdal

.PHONY: all

download: ab-download.c aerial
	${CC} ${CFLAGS} ${CSTD} -c src/download.c -o src/download.o ${CURL}
	${CC} ${CFLAGS} ${CSTD} -I src/ ab-download.c src/aerial-berlin.o src/download.o -o ab-download ${CURL} 

tile: ab-tile.c aerial
	${CC} ${CFLAGS} ${CSTD} -I src/ ab-tile.c src/aerial-berlin.o -o ab-tile ${GDAL}

convert: ab-convert.c aerial
	${CC} ${CFLAGS} ${CSTD} -I src/ ab-convert.c src/aerial-berlin.o -o ab-convert

aerial: src/aerial-berlin.c src/aerial-berlin.h
	${CC} ${CFLAGS} ${CSTD} -c src/aerial-berlin.c -o src/aerial-berlin.o ${CURL}

clean:
	rm -f src/*.o

all: download tile convert aerial clean