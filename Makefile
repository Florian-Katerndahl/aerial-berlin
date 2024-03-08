CC=gcc
CFLAGS=-Wall -Wextra -Wdouble-promotion -Wuninitialized -Winit-self -pedantic -flto -O0 -ggdb -fsanitize=undefined,address,leak
CSTD=--std=c2x
CURL=`curl-config --cflags` `curl-config --libs`

.PHONY: all

download: ab-download.c aerial
	${CC} ${CFLAGS} ${CSTD} ${CURL} -I src/ ab-download.c src/aerial-berlin.o -o ab-download

tile: ab-tile.c aerial
	${CC} ${CFLAGS} ${CSTD} -I src/ ab-tile.c src/aerial-berlin.o -o ab-tile

convert: ab-convert.c aerial
	${CC} ${CFLAGS} ${CSTD} -I src/ ab-convert.c src/aerial-berlin.o -o ab-convert

aerial: src/aerial-berlin.c src/aerial-berlin.h
	${CC} ${CFLAGS} ${CSTD} -c src/aerial-berlin.c -o src/aerial-berlin.o

clean:
	rm -f src/aerial-berlin.o

all: download tile convert aerial clean