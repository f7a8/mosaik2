#all: test
#
#test: test.o anotherTest.o
#    gcc -Wall test.c anotherTest.c -o test -I.
#
#test.o: test.c
#    gcc -Wall -c test.c -I.
#
#anotherTest.o: anothertest.c
#    gcc -Wall -c anotherTest.c -I.
#
#clean:
#    rm -rf *o test
#
#

all: tiler gathering join duplicates invalid

tiler: tiler.c
	gcc -O3 tiler.c -o tiler_hex_md5 -lm -lgd -lcrypto -lexif

gathering: gathering.c
	gcc -O3 gathering.c -o gathering -lm -lgd -lcrypto -lexif

join: join.c
	gcc -O3 join.c -o join -lm -lgd -lcurl -lexif

duplicates: duplicates.c
	gcc -O3 duplicates.c -o duplicates

invalid: invalid.c
	gcc -O3 invalid.c -o invalid -lcrypto

clean:
	rm -rf gathering tiler_hex_md5 join duplicates invalid
