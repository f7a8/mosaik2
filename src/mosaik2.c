#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "mosaik2.h"

int main(int argc, char **argv) {

	if(argc==1) {
		fprintf(stderr,"wrong parameter. append an action.\n");
		exit(EXIT_FAILURE);
	}

	const char tiler [] = { "tiler" };
	const char gathering [] = { "gathering" };
	const char join [] = { "join" };
	const char invalid [] = { "invalid" };
	const char duplicates [] = { "duplicates" };
	
	if(strncmp(	argv[1], tiler, strlen(tiler)) == 0) {
	
		if(argc!=4) {
			fprintf(stderr,"wrong parameter. usage param 1=> tile_count, 2=> file_size of the image in bytes. Image data is only accepted via stdin stream\n");
			exit(EXIT_FAILURE);
		}
		uint32_t tile_count = atoi(argv[2]);
		uint32_t file_size = atoi(argv[3]);
		return mosaik2_tiler(tile_count,file_size);
	} else if(strncmp( argv[1], gathering, strlen(gathering)) == 0) {
//		return mosaik2_gathering(argc, argv);
	} else if(strncmp( argv[1], join, strlen(join)) == 0) {
//		return mosaik2_join(argc, argv);
	} else if(strncmp( argv[1], invalid, strlen(invalid)) == 0) {
//		return mosaik2_invalid(argc, argv);
	} else if(strncmp( argv[1], duplicates, strlen(duplicates)) == 0) {
//		return mosaik2_duplicate(argc, argv);
	} else {
		fprintf(stderr, "invalid action\n");
		exit(EXIT_FAILURE);
	}
	return 0;
}
