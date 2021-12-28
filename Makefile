
all: bin/tiler_hex_md5 bin/gathering bin/join bin/duplicates bin/invalid bin/mosaik2.js

bin/tiler_hex_md5: src/tiler.c src/mosaik22.h
	gcc -O3 -Wall src/tiler.c -o bin/tiler_hex_md5 -lm -lgd -lcrypto -lexif

bin/gathering: src/gathering.c src/mosaik22.h
	gcc -O3 -Wall src/gathering.c -o bin/gathering -lm -lgd -lcrypto -lexif

bin/join: src/join.c src/mosaik22.h
	gcc -O3 -Wall src/join.c -o bin/join -lm -lgd -lcurl -lexif

bin/duplicates: src/duplicates.c src/mosaik21.h
	gcc -O3 -Wall src/duplicates.c -o bin/duplicates

bin/invalid: src/invalid.c src/mosaik21.h
	gcc -O3 -Wall src/invalid.c -o bin/invalid -lcrypto

bin/mosaik2.js: src/mosaik2.js
	cp src/mosaik2.js bin/mosaik2.js

clean:
	rm bin/tiler_hex_md5 bin/gathering bin/join bin/duplicates bin/invalid bin/mosaik2.js
