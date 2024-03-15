CC=gcc
CFLAGS=-Wall -Wextra -Wdouble-promotion -Wuninitialized -Winit-self -pedantic -flto -O1 -ggdb -fsanitize=undefined,address,leak 
RCFLAGS=-Wall -Wextra -Wdouble-promotion -Wuninitialized -Winit-self -pedantic -flto -O1
CSTD=--std=gnu2x
CURL=-lcurl
GDAL=-I/usr/local/include -L/usr/local/lib -lgdal
PNG=-lpng16 -I/usr/include/libpng16

.PHONY: all install

all: aerial download tile convert clean

install: aerial download tile convert
	mv ab-download ab-tile ab-convert /usr/local/bin/

download: ab-download.c aerial
	${CC} ${CFLAGS} ${CSTD} -c src/download.c -o src/download.o ${CURL}
	${CC} ${CFLAGS} ${CSTD} -I src/ ab-download.c src/aerial-berlin.o src/download.o -o ab-download ${CURL} 

tile: ab-tile.c aerial src/tile.c
	${CC} ${CFLAGS} ${CSTD} -c src/tile.c -o src/tile.o ${GDAL} ${PNG}
	${CC} ${CFLAGS} ${CSTD} -I src/ ab-tile.c src/aerial-berlin.o src/tile.o -o ab-tile ${GDAL}

convert: ab-convert.c aerial src/tile.c
	${CC} ${CFLAGS} ${CSTD} -c src/tile.c -o src/tile.o ${GDAL} ${PNG}
	${CC} ${CFLAGS} ${CSTD} -I src/ ab-convert.c src/aerial-berlin.o src/tile.o -o ab-convert ${GDAL} ${PNG}

aerial: src/aerial-berlin.c src/aerial-berlin.h
	${CC} ${CFLAGS} ${CSTD} -c src/aerial-berlin.c -o src/aerial-berlin.o

clean:
	rm -f src/*.o
