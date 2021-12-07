#gcc -O3 ../mosaik2_git/mosaik2/join.c -o ../mosaik2_git/mosaik2/join -lm -lgd -lcurl
#gcc -O3 ../mosaik2_git/mosaik2/gathering.c -o ../mosaik2_git/mosaik2/gathering -lm -lgd -lcrypto

echo "compiling tiler program"
gcc -O3 tiler.c -o tiler_hex_md5 -lm -lgd -lcrypto -lexif
echo "compiling gathering program"
gcc -O3 gathering.c -o gathering -lm -lgd -lcrypto -lexif
echo "compiling join program"
gcc -O3 join.c -o join -lm -lgd -lcurl -lexif # no exif needed for orientation, but for labeling the mosaik2 product
echo "compiling duplicates program"
gcc -O3 duplicates.c -o duplicates
echo "compiling invalid program"
gcc -O3 invalid.c -o invalid -lcrypto
