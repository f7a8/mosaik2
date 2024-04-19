include config.mk

COMPILER=gcc
#CFLAGS=-O3 -Wall -march=native -mtune=native
MOSAIK2LIBS=src/libmosaik2.c src/libmosaik2.h src/mosaik2.h

all: build-src build-man

build-src:
	@${MAKE} -C src

build-man:
	@${MAKE} -C man

clean:
	@${MAKE} -C src clean
	@${MAKE} -C man clean
	rm -rf test phash

install: install-man install-bin

install-man: man/mosaik2.1
	@echo installing manuals to ${man_dir}
	@mkdir -p ${man_dir}/man1
	@cp man/mosaik2.1 ${man_dir}/man1
	@chmod 644 ${man_dir}/man1/mosaik2.1

install-bin: src/mosaik2
	@echo installing executables to ${bin_dir}
	@mkdir -p ${bin_dir}
	@cp src/mosaik2 ${bin_dir}/mosaik2.tmp
	@mv ${bin_dir}/mosaik2.tmp ${bin_dir}/mosaik2
	@chmod 755 ${bin_dir}/mosaik2

uninstall:
	rm -f ${man_dir}/man1/mosaik2.1
	rm -f ${bin_dir}/mosaik2


phash: phash/src/.libs/libpHash.so.0.0.0

phash/src/.libs/libpHash.so.0.0.0:
	mkdir -p phash
	wget -nc "https://phash.org/releases/pHash-0.9.6.tar.gz" -P phash
	tar xfz phash/pHash-0.9.6.tar.gz -C phash
	apt install cimg-dev
	cd phash; pHash-0.9.6/configure --enable-video-hash=no --enable-audio-hash=no --without-libpng
	cd phash; make

test: all
	mkdir -p test/flower
	wget -N "https://storage.googleapis.com/download.tensorflow.org/example_images/flower_photos.tgz" --directory-prefix=/tmp
	wget -N "https://upload.wikimedia.org/wikipedia/commons/5/52/2014.03.29.-08-Mannheim_Neckarau_Waldpark-Wiesen-Schaumkraut.jpg" --directory-prefix=/tmp
	tar xfz /tmp/flower_photos.tgz -C test/flower
	find test/flower/flower_photos -type f -iregex '.*\.jpe?g$$' -size +10000c -size -100000000c > test/flower/flower_photos.file_list
	src/mosaik2 init -r 8 test/flower/flowerphotos8
	src/mosaik2 init -r 16 test/flower/flowerphotos16
	src/mosaik2 index -q test/flower/flowerphotos8 < test/flower/flower_photos.file_list
	src/mosaik2 index -q test/flower/flowerphotos16 < test/flower/flower_photos.file_list
	src/mosaik2 duplicates -P 10 test/flower/flowerphotos8
	src/mosaik2 duplicates -P 10 test/flower/flowerphotos16
	src/mosaik2 gathering -q -u -t 16 test/flower/Wiesen-Schaumkraut8.jpeg test/flower/flowerphotos8 < /tmp/2014.03.29.-08-Mannheim_Neckarau_Waldpark-Wiesen-Schaumkraut.jpg
	src/mosaik2 gathering -q -u -t 16 test/flower/Wiesen-Schaumkraut16.jpeg test/flower/flowerphotos16 < /tmp/2014.03.29.-08-Mannheim_Neckarau_Waldpark-Wiesen-Schaumkraut.jpg
	src/mosaik2 join -q -p 100 test/flower/Wiesen-Schaumkraut8.jpeg test/flower/flowerphotos8
	src/mosaik2 join -q -p 100 test/flower/Wiesen-Schaumkraut16.jpeg test/flower/flowerphotos16
	diff misc/test/Wiesen-Schaumkraut8.jpeg test/flower/Wiesen-Schaumkraut8.jpeg
	diff misc/test/Wiesen-Schaumkraut16.jpeg test/flower/Wiesen-Schaumkraut16.jpeg

.PHONY: all test install uninstall clean install-man install-bin

