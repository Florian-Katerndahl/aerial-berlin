CC=gcc
CFLAGS=-Wall -Wextra -Wdouble-promotion -Wuninitialized -Winit-self -pedantic -flto -O1 -fsanitize=undefined,address,leak -ggdb
RCFLAGS=-Wall -Wextra -Wdouble-promotion -Wuninitialized -Winit-self -pedantic -flto -O1
CSTD=--std=gnu2x
CURL=-lcurl
GDAL=-I/usr/local/include -L/usr/local/lib -lgdal

.PHONY: all

all: aerial download tile convert clean

download: ab-download.c aerial
	${CC} ${CFLAGS} ${CSTD} -c src/download.c -o src/download.o ${CURL}
	${CC} ${CFLAGS} ${CSTD} -I src/ ab-download.c src/aerial-berlin.o src/download.o -o ab-download ${CURL} 

tile: ab-tile.c aerial
	${CC} ${CFLAGS} ${CSTD} -c src/tile.c -o src/tile.o ${GDAL}
	${CC} ${CFLAGS} ${CSTD} -I src/ ab-tile.c src/aerial-berlin.o src/tile.o -o ab-tile ${GDAL}

convert: ab-convert.c aerial
	${CC} ${CFLAGS} ${CSTD} -I src/ ab-convert.c src/aerial-berlin.o -o ab-convert

aerial: src/aerial-berlin.c src/aerial-berlin.h
	${CC} ${CFLAGS} ${CSTD} -c src/aerial-berlin.c -o src/aerial-berlin.o

clean:
	rm -f src/*.o
