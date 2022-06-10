#include <libgen.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <openssl/md5.h>
#include <err.h>

#include "mosaik2.h"

int main(int argc, char **argv) {

	if(argc==1) {
		fprintf(stderr,"mosaik2 - creates real photo mosaics\n\n");
		fprintf(stderr,"mosaik2 is a Command Line Interface program for Linux, especially designed for large amounts of data.\n");
		fprintf(stderr,"website at https://f7a8.github.io/mosaik2/\n\n\nwrong parameter. append an action.\n");
		exit(EXIT_FAILURE);
	}

	const char *init = "init";
	const char *index = "index";
	//const char *tiler = "tiler";
	const char *gathering = "gathering";
	const char *join = "join";
	const char *invalid = "invalid";
	const char *duplicates = "duplicates";

	if(strncmp( argv[1], init, strlen(init)) == 0) {
		if(argc!=4) {
			fprintf(stderr,"wrong parameter. usage param 1=> mosaik2_database_name (path to directory), 2 => tilecount\n");
			exit(EXIT_FAILURE);
		}
		return mosaik2_init(argv[2],atoi(argv[3]));
	} else if(strncmp( argv[1], index, strlen(index)) == 0) {
		if(argc!=5) {
			fprintf(stderr, "wrong parameter. usage param 1=> mosaik2_database_name (path to directory), 2 => max_tiler_process (uint8_t), 3 => max_load_avg (uint32_t). File list for indexing is only accepted via stdin stream.\n");
			exit(EXIT_FAILURE);
		}
		return mosaik2_index(argv[2], atoi(argv[3]), atoi(argv[4]));
	/*} else if(strncmp(	argv[1], tiler, strlen(tiler)) == 0) {
	
		if(argc!=4) {
			fprintf(stderr,"wrong parameter. usage param 1=> tile_count, 2=> file_size of the image in bytes. Image data is only accepted via stdin stream\n");
			exit(EXIT_FAILURE);
		}
		uint32_t tile_count = atoi(argv[2]);
		uint32_t file_size = atoi(argv[3]);
		
		mosaik2_indextask task;
		task.tile_count = atoi(argv[2]);
		task.filesize = atoi(argv[3]);

		return mosaik2_tiler(NULL);*/
	} else if(strncmp( argv[1], gathering, strlen(gathering)) == 0) {

		if(argc!=8) {
			fprintf(stderr,"wrong parameter. usage param: \n\t1 => primary_tile_count (only approx. depends on the input image, can be slightly more)\n\t2 => file_size in bytes\n\t3 => dest_filename (including jpeg or png suffix)\n\t4 => ratio (0<=ratio<=100) of the weightning between image color and image standard deviation of the color (100 could be a good starting value)\n\t5 => unique (0 or 1) use a tile at least one time\n\t6 => pathname to mosaik2_thumb_db\nImage data is only accepted via stdin stream.\n");
			exit(EXIT_FAILURE);
		}

		uint8_t primary_tile_count = atoi(argv[2]);
		uint32_t file_size=atoi(argv[3]);
		char *dest_filename=argv[4];
		uint8_t ratio = atoi(argv[5]);
		uint8_t unique = atoi(argv[6]);
		char *mosaik2_db_name=argv[7];

		return mosaik2_gathering(primary_tile_count, file_size, dest_filename, ratio, unique, mosaik2_db_name);
	} else if(strncmp( argv[1], join, strlen(join)) == 0) {
	if(argc<=5) {
		fprintf(stderr,"wrong parameter. usage:\n\t1=> dest_filename (including jpeg or png suffix)\n\t2=> image width in per primary tile in px\n\t3=> unique_tiles ( 1 or 0 ) duplicate tiles can be supressed as much as thumbs_db are involved\n\t4 => local_cache ( 1 copy files into ~/.mosaik2/, 0 creates symbolic links),\n\t5 => thumbs_db_name_1\n\t[ ... ]\n");
		exit(EXIT_FAILURE);
	}

	char *dest_filename = argv[2];
	uint32_t dest_tile_width = atoi(argv[3]);
	uint8_t unique_tile = atoi(argv[4]);
	uint8_t local_cache = atoi(argv[5]);
	return mosaik2_join(dest_filename, dest_tile_width, unique_tile, local_cache, argc, argv);
//		return mosaik2_join(argc, argv);
	} else if(strncmp( argv[1], invalid, strlen(invalid)) == 0) {
	if(argc!=6) {
		fprintf(stderr,"wrong parameter. usage param 1=> mosaik2_db_dir, 2=> ignore_old_invalids ( 0 or 1; 1 ingores already marked invalids ), 3=> dry_run (0 or 1; if 1, then nothing is saved to the invalid file), 4 => no_hash_cmp (0 or 1; if 1 only file meta data is compared no file contents)\nThis program marks only new invalid files. It will print a filelist with the new valid informations to stdout, you can use this for creating a new mosaik2 database\n");
		exit(EXIT_FAILURE);
	}
		return mosaik2_invalid(argv[2],atoi(argv[3]), atoi(argv[4]), atoi(argv[5]));
//		return mosaik2_invalid(argc, argv);
	} else if(strncmp( argv[1], duplicates, strlen(duplicates)) == 0) {
		if(argc!=5) {
			fprintf(stderr,"wrong parameter. usage param 1=> mosaik2_db_dir, 2=> mosaik2_db_dir, 3=> dry_run (0 or 1)\n");
			exit(EXIT_FAILURE);
		}
		return mosaik2_duplicates(argv[2],argv[3],atoi(argv[4]));
	} else {
		fprintf(stderr, "invalid action, valid actions: {init,index,gathering,join,invalid,duplicates}\n");
		exit(EXIT_FAILURE);
	}
	return 0;
}
