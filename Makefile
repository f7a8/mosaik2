
COMPILER=gcc

all: bin/mosaik2 bin/mosaik2.js


bin/mosaik2: bin/libmosaik2.o bin/tiler.o bin/gathering.o bin/join.o bin/duplicates.o bin/invalid.o
	${COMPILER} -O3 src/mosaik2.c bin/tiler.o bin/gathering.o bin/join.o bin/duplicates.o bin/invalid.o bin/libmosaik2.o -o bin/mosaik2 -lm -lgd -lcrypto -lexif -lcurl

bin/libmosaik2.o:
	${COMPILER} -O3 -c src/mosaik21.c -o bin/libmosaik2.o -lexif -lgd

bin/tiler.o:
	${COMPILER} -O3 -c src/tiler.c -o bin/tiler.o

bin/gathering.o:
	${COMPILER} -O3 -c src/gathering.c -o bin/gathering.o

bin/join.o:
	${COMPILER} -O3 -c src/join.c -o bin/join.o

bin/duplicates.o:
	${COMPILER} -O3 -c src/duplicates.c -o bin/duplicates.o

bin/invalid.o:
	${COMPILER} -O3 -c src/invalid.c -o bin/invalid.o

bin/mosaik2.js: src/mosaik2.js
	cp src/mosaik2.js bin/mosaik2.js

clean:
	rm bin/*
