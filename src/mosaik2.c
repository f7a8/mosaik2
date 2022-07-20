#include <libgen.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <openssl/md5.h>
#include <err.h>
#include <unistd.h>
#include <error.h>

#include "mosaik2.h"

void print_usage();
void print_help();
void print_version();
void get_mosaik2_arguments(mosaik2_arguments *args, int argc, char **argv);


int main(int argc, char **argv) {

  mosaik2_arguments args;
	get_mosaik2_arguments(&args, argc, argv);

	if(strncmp( args.mode, "init", strlen("init")) == 0) {
		return mosaik2_init(&args);
	} else if(strncmp( args.mode, "index", strlen("index")) == 0) {
		return mosaik2_index(&args);
	} else if(strncmp( args.mode, "gathering", strlen("gathering")) == 0) {
		return mosaik2_gathering(&args);
	} else if(strncmp( args.mode, "join", strlen("join")) == 0) {
		return mosaik2_join(&args);
	} else if(strncmp( args.mode, "invalid", strlen("invalid")) == 0) {
		return mosaik2_invalid(&args);
	} else if(strncmp( args.mode, "duplicates", strlen("duplicates")) == 0) {
		return mosaik2_duplicates(&args);
	}

	return 0;
}

void get_mosaik2_arguments(mosaik2_arguments *args, int argc, char **argv) {

	memset(args,0,sizeof(mosaik2_arguments));

	args->database_image_resolution = 16;
	args->color_stddev_ratio = 100;
	args->pixel_per_tile = 200;
	args->num_tiles = 20;

	const int mode_init = 0;
	const int mode_index = 1;
	const int mode_gathering = 2;
	const int mode_join = 3;
	const int mode_duplicates = 4;
	const int mode_invalid = 5;
	char *modes[] = {"init", "index", "gathering", "join", "duplicates", "invalid"};
	int modes_used[] = {0,0,0,0,0,0};

	char *all_options = "dhij:l:np:r:R:st:uvVy?";
	char *mode_init_options = "Vr:";
	char *mode_index_options = "Vj:l:";
	char *mode_gathering_options = "Vt:uR:";
	char *mode_join_options = "Vp:sd";
	char *mode_duplicates_options = "Viyj";
	char *mode_invalid_options = "Viyn";

	int opt;
	while((opt = getopt(argc, argv, "dhij:l:np:r:R:st:uvVy")) != -1 ) {
		switch(opt) {
			case 'd': args->duplicate_reduction = 1; 
								modes_used[mode_join]++;
								break;
			case 'h': print_usage(); print_help(); exit(EXIT_SUCCESS); break;
			case 'i': args->ignore_old_invalids = 1; 
								modes_used[mode_duplicates]++;
								break;
			case 'j': args->max_jobs = atoi(optarg); 
								modes_used[mode_index]++;
								break;
			case 'l': args->max_load = atoi(optarg);
								modes_used[mode_index]++;
								break;
			case 'n': args->no_hash_cmp = 1;
								modes_used[mode_invalid]++;
								break;
			case 'p': args->pixel_per_tile = atoi(optarg);
								modes_used[mode_join]++;
								break;
			case 'r': args->database_image_resolution = atoi(optarg);
								modes_used[mode_init]++;
								break;
			case 'R': args->color_stddev_ratio = atoi(optarg);
								modes_used[mode_gathering]++;
								break;
			case 's': args->symlink_cache = 1;
								modes_used[mode_join]++;
								break;
			case 't': args->num_tiles = atoi(optarg);
								modes_used[mode_gathering]++;
								break;
			case 'u': args->unique = 1;
								modes_used[mode_gathering]++;
								break;
			case 'v': print_version(); exit(0);
			case 'V': args->verbose = 1; break;
			case 'y': args->dry_run = 1; break;
			default: /* ? */ print_usage(); break;
		}
	}

	if(optind >= argc) {
		print_usage();
		exit(EXIT_FAILURE);
	}

	
	args->mode = argv[optind];
	int mode = -1;
	for(int i=0;i<6;i++) {
		if(strncmp(args->mode, modes[i], strlen(args->mode))==0) {
			mode=i; break;
		}
	}

	if(mode==-1) {
		print_usage();
		exit(EXIT_FAILURE);
	}

	for(int i=0;i<6;i++) {
		if(modes_used[i] > 0 && i != mode) {
			print_usage();
			exit(EXIT_FAILURE);
		}
	}
	// special case, dry-run is valid in two modes, modes_used was not incremented for it
	if(args->dry_run == 1 && ( mode != mode_invalid || mode != mode_duplicates ) ) {
		print_usage();
		exit(EXIT_FAILURE);
	}

	int marg = argc-optind;
	int invalid = 
		 (mode == 0 && marg != 2)
	|| (mode == 1 && marg != 2)
	|| (mode == 2 && marg != 3)
	|| (mode == 3 && marg < 3)
	|| (mode == 4 && marg != 3)
	|| (mode == 5 && marg != 2);

	if(invalid) {
		print_usage();
		exit(EXIT_FAILURE);
	}


	switch(mode) {
		case 0: args->mosaik2db = argv[optind+1]; break;
		case 1: args->mosaik2db = argv[optind+1]; break;
		case 2: /*gathering */ 
						args->dest_image = argv[optind+1];
            args->mosaik2db = argv[optind+2]; break;
		case 3: /* join */ 
						args->dest_image = argv[optind+1];
            args->mosaik2dbs= &argv[optind+2];
						args->mosaik2dbs_count = marg-2; break;
		case 4: args->mosaik2db =  argv[optind+2];
            args->mosaik2dbs= &argv[optind+3]; 
						args->mosaik2dbs_count = marg-2; break;
		case 5: args->mosaik2db = argv[optind+2]; break;
	}
	
	if(args->verbose) {
		printf ("mode = %s\n", args->mode);
		printf ("mosaik2db = %s\n", args->mosaik2db);
		for(int i=0;i<args->mosaik2dbs_count;i++) {
			printf("mosaik2dbs[%i] = %s\n", i, args->mosaik2dbs[i]);
		}
		printf ("dest-image = %s\n", args->dest_image);
	  printf ("options:\nverbose = %s\ndry-run = %s\ndatabase_image_resolution = %i\nmax_load = %i\nmax_jobs = %i\nunique = %s\ncolor_stddev_ratio = %i\npixel_per_tile = %i\nduplicate_reduction = %s\nsymlink_cache = %s\nignore_old_invalids = %s\nno_hash_cmp = %s\n\n",
              args->verbose ? "yes" : "no",

              args->dry_run ? "yes" : "no",
              args->database_image_resolution,
              args->max_load,
              args->max_jobs,
              args->unique ? "yes" : "no",
              args->color_stddev_ratio,
              args->pixel_per_tile,
              args->duplicate_reduction ? "yes" : "no",
              args->symlink_cache ? "yes" : "no",
              args->ignore_old_invalids ? "yes" : "no",
              args->no_hash_cmp ? "yes" : "no");
	}
}

void print_version() {
	fprintf(stdout, "mosaik2 v%s\n", MOSAIK2_VERSION);
}

void print_usage() {
//	fprintf(stdout, 
//"Usage: mosaik2 [OPTION]... init MOSAIK2DB\n"
//"  or:  mosaik2 [OPTION]... index MOSAIK2DB < file-list\n"
//"  or:  mosaik2 [OPTION]... gathering dest-image MOSAIK2DB < src-image\n"
//"  or:  mosaik2 [OPTION]... join dest-image MOSAIK2DB_0 [MOSAIK2DB_1, ...]\n"
//"  or:  mosaik2 [OPTION]... duplicates MOSAIK2DB_0 MOSAIK2DB_1\n"
//"  or:  mosaik2 [OPTION]... invalid MOSAIK2DB\n"
//"  or:  mosaik2 [-h|-v]\n" );
	fprintf(stdout, 
"Usage: mosaik2 [-V] [-r <PIXEL>] init MOSAIK2DB\n"
"  or:  mosaik2 [-V] [-j <COUNT>] [-l <LOAD>] index MOSAIK2DB < file-list\n"
"  or:  mosaik2 [-V] [-t <NUM>] [-u] [-R <PERCENT>] gathering dest-image MOSAIK2DB < src-image\n"
"  or:  mosaik2 [-V] [-p <PIXEL>] [-s] [-d] join dest-image MOSAIK2DB_0 [MOSAIK2DB_1, ...]\n"
"  or:  mosaik2 [-V] [-i] [-y] duplicates MOSAIK2DB_0 MOSAIK2DB_1\n"
"  or:  mosaik2 [-V] [-i] [-y] [-n] invalid MOSAIK2DB\n"
"  or:  mosaik2 {-h|-v}\n" );
}

void print_help() {
	fprintf(stdout, 
"\nmosaik2 -- creates real photo mosaics. ready for large data sets.\n"
"\n"
" OPTIONS\n"
"  -V          Verbose output\n"
"  -h          Print this help and exit\n"
"  -v          Print program version and exit\n"
"  -r PIXEL    Database-image-resolution in PIXEL (default 16)\n"
"  -j COUNT    Limit concurrent worker jobs to COUNT (default\n"
"              processor cout)\n"
"  -l LOAD     Soft limit the system load to LOAD (default 0,\n"
"              means off)\n"
"  -t NUM      Use NUM image tiles (default 20)\n"
"  -u          Allow images only once (unique)\n"
"  -R PERCENT  Ratio between color matching and color\n"
"              standard devation (default 100 means only color\n"
"              matching only, 0 uses only stddev informations)\n"
"  -p PIXEL    Image resolution in PIXEL of one image tile in\n"
"              the dest-image (default 200)\n"
"  -s          Symlinks instead of file copies\n"
//"  -c PATH                    Cache path (default ~/.mosaik2)\n"
"  -d          Fast but slight reduction of duplicate images\n"
"  -i          Ignore old invalid images\n"
"  -y          Dry run: does not change the database\n"
"  -n          No hash comparison\n"
"\n"
"Website: https://f7a8.github.io/mosaik2/\n"
"\n"
"Report bugs to https://github.com/f7a8/mosaik2/issues.\n");
}


