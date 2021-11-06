#gcc -O3 ../mosaik2_git/mosaik2/join.c -o ../mosaik2_git/mosaik2/join -lm -lgd -lcurl
#gcc -O3 ../mosaik2_git/mosaik2/gathering.c -o ../mosaik2_git/mosaik2/gathering -lm -lgd -lcrypto

echo "compiling gathering program"
gcc -O3 gathering.c -o gathering -g -lm -lgd -lcrypto
echo "compiling join program"
gcc -O3 join.c -o join -lm -lgd -lcurl
echo "compiling tiler program"
gcc -O3 tiler.c -o tiler -lgd
