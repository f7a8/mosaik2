
COMPILER=gcc
CFLAGS=-O3 -Wall -march=native -mtune=native
MOSAIK2LIBS=src/libmosaik2.c src/libmosaik2.h src/mosaik2.h

all: bin/ bin/mosaik2

bin/:
	mkdir -p bin/

bin/mosaik2: src/mosaik2.h src/mosaik2.c bin/libmosaik2.o bin/init.o bin/index.o bin/tiler.o bin/gathering.o bin/join.o bin/duplicates.o bin/invalid.o bin/info.o bin/crop.o
	${COMPILER} ${CFLAGS} src/mosaik2.c bin/init.o bin/index.o bin/tiler.o bin/gathering.o bin/join.o bin/duplicates.o bin/invalid.o bin/info.o bin/crop.o bin/libmosaik2.o -o bin/mosaik2 -lm -lgd -lcrypto -lexif -lcurl

bin/libmosaik2.o: src/libmosaik2.c src/libmosaik2.h
	${COMPILER} ${CFLAGS} -c src/libmosaik2.c -o bin/libmosaik2.o -lexif -lgd

bin/init.o: src/init.c $(MOSAIK2LIBS)
	${COMPILER} ${CFLAGS} -c src/init.c -o bin/init.o

bin/index.o: src/index.c $(MOSAIK2LIBS)
	${COMPILER} ${CFLAGS} -c src/index.c -o bin/index.o

bin/tiler.o: src/tiler.c $(MOSAIK2LIBS)
	${COMPILER} ${CFLAGS} -c src/tiler.c -o bin/tiler.o

bin/gathering.o: src/gathering.c $(MOSAIK2LIBS)
	${COMPILER} ${CFLAGS} -c src/gathering.c -o bin/gathering.o

bin/join.o: src/join.c $(MOSAIK2LIBS)
	${COMPILER} ${CFLAGS} -c src/join.c -o bin/join.o

bin/duplicates.o: src/duplicates.c $(MOSAIK2LIBS)
	${COMPILER} ${CFLAGS} -c src/duplicates.c -o bin/duplicates.o

bin/invalid.o: src/invalid.c $(MOSAIK2LIBS)
	${COMPILER} ${CFLAGS} -c src/invalid.c -o bin/invalid.o

bin/info.o: src/info.c $(MOSAIK2LIBS)
	${COMPILER} ${CFLAGS} -c src/info.c -o bin/info.o

bin/crop.o: src/crop.c $(MOSAIK2LIBS)
	${COMPILER} ${CFLAGS} -c src/crop.c -o bin/crop.o

clean:
	rm -rf bin
	rm -rf test

test: all
	mkdir -p test/flower
	wget -N "https://storage.googleapis.com/download.tensorflow.org/example_images/flower_photos.tgz" --directory-prefix=/tmp
	wget -N "https://upload.wikimedia.org/wikipedia/commons/5/52/2014.03.29.-08-Mannheim_Neckarau_Waldpark-Wiesen-Schaumkraut.jpg" --directory-prefix=/tmp
	tar xfz /tmp/flower_photos.tgz -C test/flower
	find test/flower/flower_photos -type f -iregex '.*\.jpe?g$$' -size +10000c -size -100000000c -fprintf test/flower/flower_photos.file_list "%p\t%s\t%T@\n" 
	bin/mosaik2 init -r 8 test/flower/flowerphotos8
	bin/mosaik2 init -r 16 test/flower/flowerphotos16
	bin/mosaik2 index -q test/flower/flowerphotos8 < test/flower/flower_photos.file_list
	bin/mosaik2 index -q test/flower/flowerphotos16 < test/flower/flower_photos.file_list
	bin/mosaik2 duplicates test/flower/flowerphotos8
	bin/mosaik2 duplicates test/flower/flowerphotos16
	bin/mosaik2 gathering -q -u -t 16 test/flower/Wiesen-Schaumkraut8.jpeg test/flower/flowerphotos8 < /tmp/2014.03.29.-08-Mannheim_Neckarau_Waldpark-Wiesen-Schaumkraut.jpg
	bin/mosaik2 gathering -q -u -t 16 test/flower/Wiesen-Schaumkraut16.jpeg test/flower/flowerphotos16 < /tmp/2014.03.29.-08-Mannheim_Neckarau_Waldpark-Wiesen-Schaumkraut.jpg
	bin/mosaik2 join -q -p 100 test/flower/Wiesen-Schaumkraut8.jpeg test/flower/flowerphotos8
	bin/mosaik2 join -q -p 100 test/flower/Wiesen-Schaumkraut16.jpeg test/flower/flowerphotos16
	diff misc/test/Wiesen-Schaumkraut8.jpeg test/flower/Wiesen-Schaumkraut8.jpeg
	diff misc/test/Wiesen-Schaumkraut16.jpeg test/flower/Wiesen-Schaumkraut16.jpeg
