
COMPILER=gcc
CFLAGS=-O3 -march=native -mtune=native

all: directory bin/mosaik2

directory:
	mkdir -p bin/

bin/mosaik2: src/mosaik2.c bin/libmosaik2.o bin/init.o bin/index.o bin/tiler.o bin/gathering.o bin/join.o bin/duplicates.o bin/invalid.o
	${COMPILER} ${CFLAGS} src/mosaik2.c bin/init.o bin/index.o bin/tiler.o bin/gathering.o bin/join.o bin/duplicates.o bin/invalid.o bin/libmosaik2.o -o bin/mosaik2 -lm -lgd -lcrypto -lexif -lcurl

bin/libmosaik2.o: src/libmosaik2.c src/libmosaik2.h
	${COMPILER} ${CFLAGS} -c src/libmosaik2.c -o bin/libmosaik2.o -lexif -lgd

bin/init.o: src/init.c
	${COMPILER} ${CFLAGS} -c src/init.c -o bin/init.o

bin/index.o: src/index.c 
	${COMPILER} ${CFLAGS} -c src/index.c -o bin/index.o

bin/tiler.o: src/tiler.c
	${COMPILER} ${CFLAGS} -c src/tiler.c -o bin/tiler.o

bin/gathering.o: src/gathering.c
	${COMPILER} ${CFLAGS} -c src/gathering.c -o bin/gathering.o

bin/join.o: src/join.c
	${COMPILER} ${CFLAGS} -c src/join.c -o bin/join.o

bin/duplicates.o: src/duplicates.c
	${COMPILER} ${CFLAGS} -c src/duplicates.c -o bin/duplicates.o

bin/invalid.o: src/invalid.c
	${COMPILER} ${CFLAGS} -c src/invalid.c -o bin/invalid.o

clean:
	rm -f bin/*
	rm -rf test

test:
	mkdir -p test/flower
	wget -N "https://storage.googleapis.com/download.tensorflow.org/example_images/flower_photos.tgz" --directory-prefix=test/flower
	wget -N "https://upload.wikimedia.org/wikipedia/commons/5/52/2014.03.29.-08-Mannheim_Neckarau_Waldpark-Wiesen-Schaumkraut.jpg" --directory-prefix=test/flower
	tar xfz test/flower/flower_photos.tgz -C test/flower 
	find test/flower/flower_photos -type f -iregex '.*\.jpe?g$$' -size +10000c -fprintf  flower_photos.file_list "%p\t%s\t%T@\n" 
	node bin/mosaik2.js init test/flower/flowerphotos16 16
	node bin/mosaik2.js index test/flower/flowerphotos16 8 8 < flower_photos.file_list
	node bin/mosaik2.js init test/flower/flowerphotos32 32
	node bin/mosaik2.js index test/flower/flowerphotos32 8 8 < flower_photos.file_list
	bin/mosaik2 gathering 30 5868357  test/flower/Wiesen-Schaumkraut.jpeg 100 1 test/flower/flowerphotos16 < test/flower/2014.03.29.-08-Mannheim_Neckarau_Waldpark-Wiesen-Schaumkraut.jpg
	bin/mosaik2 gathering 30 5868357  test/flower/Wiesen-Schaumkraut.jpeg 100 1 test/flower/flowerphotos32 < test/flower/2014.03.29.-08-Mannheim_Neckarau_Waldpark-Wiesen-Schaumkraut.jpg
	bin/mosaik2 join test/flower/Wiesen-Schaumkraut.jpeg 100 0 1 test/flower/flowerphotos16
	bin/mosaik2 join test/flower/Wiesen-Schaumkraut.jpeg 100 0 1 test/flower/flowerphotos32


