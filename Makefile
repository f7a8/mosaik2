
COMPILER=gcc

all: bin/mosaik2 bin/mosaik2.js


bin/mosaik2: src/mosaik2.c bin/libmosaik2.o bin/tiler.o bin/gathering.o bin/join.o bin/duplicates.o bin/invalid.o
	${COMPILER} -O3 src/mosaik2.c bin/tiler.o bin/gathering.o bin/join.o bin/duplicates.o bin/invalid.o bin/libmosaik2.o -o bin/mosaik2 -lm -lgd -lcrypto -lexif -lcurl

bin/libmosaik2.o: src/libmosaik2.c
	${COMPILER} -O3 -c src/libmosaik2.c -o bin/libmosaik2.o -lexif -lgd

bin/tiler.o: src/tiler.c
	${COMPILER} -O3 -c src/tiler.c -o bin/tiler.o

bin/gathering.o: src/gathering.c
	${COMPILER} -O3 -c src/gathering.c -o bin/gathering.o

bin/join.o: src/join.c
	${COMPILER} -O3 -c src/join.c -o bin/join.o

bin/duplicates.o: src/duplicates.c
	${COMPILER} -O3 -c src/duplicates.c -o bin/duplicates.o

bin/invalid.o: src/invalid.c
	${COMPILER} -O3 -c src/invalid.c -o bin/invalid.o

bin/mosaik2.js: src/mosaik2.js
	cp src/mosaik2.js bin/mosaik2.js

clean:
	rm -f bin/*
	rm -rf test

test:
	mkdir -p test/flower
	tar xfz flower_photos.tgz -C test/flower # <( wget -q0- "https://storage.googleapis.com/download.tensorflow.org/example_images/flower_photos.tgz" -O test/flower/flower_photos.tgz )
	~/Downloads/node-v14.16.0-linux-x64/bin/node bin/mosaik2.js init test/flower/flowerphotos16 16
	find test/flower/flower_photos -type f -iregex '.*\.jpe?g$$' -size +40000c -fprintf  flower_photos.file_list "%p\t%s\t%T@\n" 
	~/Downloads/node-v14.16.0-linux-x64/bin/node bin/mosaik2.js index test/flower/flowerphotos16 8 8 < flower_photos.file_list
	wget "https://upload.wikimedia.org/wikipedia/commons/5/52/2014.03.29.-08-Mannheim_Neckarau_Waldpark-Wiesen-Schaumkraut.jpg" -nd # -O 2014.03.29.-08-Mannheim_Neckarau_Waldpark-Wiesen-Schaumkraut.jpg
	bin/mosaik2 gathering 30 5868357  test/flower/Wiesen-Schaumkraut.jpeg 100 1 test/flower/flowerphotos16 < 2014.03.29.-08-Mannheim_Neckarau_Waldpark-Wiesen-Schaumkraut.jpg
	bin/mosaik2 join test/flower/Wiesen-Schaumkraut.jpeg 100 0 1 test/flower/flowerphotos16


