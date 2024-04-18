CC=gcc
CFLAGS=-Wall -Wextra -Wdouble-promotion -Wuninitialized -Winit-self -pedantic -flto
RCFLAGS=-Wall -Wextra -Wdouble-promotion -Wuninitialized -Winit-self -pedantic -flto
CSTD=--std=gnu2x
CURL=-lcurl
GDAL=-I/usr/local/include -L/usr/local/lib -lgdal
PNG=-lpng16 -I/usr/include/libpng16

.PHONY: all install

all: objs tile download convert clean

debug: CFLAGS += -Og -ggdb -fsanitize=undefined,address,leak #-fanalyze
debug: all

release: CFLAGS += -O3
release: all

install: objs download tile convert
	mv ab-download ab-tile ab-convert /usr/local/bin/

objs: src/tile.c src/aerial-berlin.c src/download.c
	${CC} ${CFLAGS} ${CSTD} -c src/tile.c -o src/tile.o ${GDAL} ${PNG}
	${CC} ${CFLAGS} ${CSTD} -c src/download.c -o src/download.o ${CURL}
	${CC} ${CFLAGS} ${CSTD} -c src/aerial-berlin.c -o src/aerial-berlin.o

download: ab-download.c objs
	${CC} ${CFLAGS} ${CSTD} -I src/ ab-download.c src/aerial-berlin.o src/download.o src/tile.o -o ab-download ${CURL} 

tile: ab-tile.c objs
	${CC} ${CFLAGS} ${CSTD} -I src/ ab-tile.c src/aerial-berlin.o src/tile.o -o ab-tile ${GDAL}

convert: ab-convert.c objs
	${CC} ${CFLAGS} ${CSTD} -I src/ ab-convert.c src/aerial-berlin.o src/tile.o -o ab-convert ${GDAL} ${PNG}

clean:
	rm -f src/*.o
