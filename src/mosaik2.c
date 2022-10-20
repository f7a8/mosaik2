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

#define MODE_INIT 0
#define MODE_INDEX 1
#define MODE_GATHERING 2
#define MODE_JOIN 3
#define MODE_DUPLICATES 4
#define MODE_INVALID 5
#define MODE_INFO 6
#define MODE_COUNT 7

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
	} else if(strncmp( args.mode, "info", strlen("info")) == 0) {
		return mosaik2_info(&args);
	}

	return 0;
}

void get_mosaik2_arguments(mosaik2_arguments *args, int argc, char **argv) {

	memset(args,0,sizeof(mosaik2_arguments));

	args->database_image_resolution = 16;
	args->color_stddev_ratio = 100;
	args->pixel_per_tile = 200;
	args->num_tiles = 20;
	args->color_distance = MOSAIK2_ARGS_COLOR_DISTANCE_DEFAULT;
	args->has_element_number = 0;

	char *modes[] = {"init", "index", "gathering", "join", "duplicates", "invalid", "info"};
	
	int modes_used[] = {0,0,0,0,0,0,0};

	char *all_options = "dD:he:ij:l:np:r:R:st:uvVy?";

	int opt;
	while((opt = getopt(argc, argv, all_options)) != -1 ) {
		switch(opt) {
			case 'd': args->duplicate_reduction = 1; 
								modes_used[MODE_JOIN]++;
								break;
			case 'D':
								if(strncmp("manhattan", optarg, strlen(optarg)) == 0) {
									args->color_distance = MOSAIK2_ARGS_COLOR_DISTANCE_MANHATTAN;
								} else if(strncmp("euclidian", optarg, strlen(optarg)) == 0) {
									args->color_distance = MOSAIK2_ARGS_COLOR_DISTANCE_EUCLIDIAN;
								} else if(strncmp("chebyshev", optarg, strlen(optarg)) == 0) {
									args->color_distance = MOSAIK2_ARGS_COLOR_DISTANCE_CHEBYSHEV;
								} else {
									print_usage(); exit(EXIT_FAILURE);
								}
								modes_used[MODE_GATHERING]++;
								break;
			/* -h should be the only parameter, but this code will print help
			immediatly when -h option is parsed. Same with -v. But I think, thats common sense. */
			case 'e': args->element_number = atoi(optarg);
				  args->has_element_number = 1;
				  break; //no modes_used because it appears in several modes
			case 'h': print_usage(); print_help(); exit(EXIT_SUCCESS); break;
			case 'i': args->ignore_old_invalids = 1; 
								modes_used[MODE_DUPLICATES]++;
								break;
			case 'j': args->max_jobs = atoi(optarg); 
								modes_used[MODE_INDEX]++;
								break;
			case 'l': args->max_load = atoi(optarg);
								modes_used[MODE_INDEX]++;
								break;
			case 'n': args->no_hash_cmp = 1;
								modes_used[MODE_INVALID]++;
								break;
			case 'p': args->pixel_per_tile = atoi(optarg);
								modes_used[MODE_JOIN]++;
								break;
			case 'r': args->database_image_resolution = atoi(optarg);
								modes_used[MODE_INIT]++;
								break;
			case 'R': args->color_stddev_ratio = atoi(optarg);
								modes_used[MODE_GATHERING]++;
								break;
			case 's': args->symlink_cache = 1;
								modes_used[MODE_JOIN]++;
								break;
			case 't': args->num_tiles = atoi(optarg);
								modes_used[MODE_GATHERING]++;
								break;
			case 'u': args->unique = 1;
								modes_used[MODE_GATHERING]++;
								break;
			case 'v': print_version(); exit(EXIT_SUCCESS); 
			case 'V': args->verbose = 1; break; // no modes_used because it appears in serveral modes
			case 'y': args->dry_run = 1; break; // no modes_used because it appears in serveral modes

			default: /* ? */ print_usage(); exit(EXIT_FAILURE); break;
		}
	}

	if(optind >= argc) {
		print_usage();
		exit(EXIT_FAILURE);
	}
	
	args->mode = argv[optind];
	int mode = -1;
	for(int i=0;i<MODE_COUNT;i++) {
		if(strncmp(args->mode, modes[i], strlen(args->mode))==0) {
			mode=i; break;
		}
	}

	if(mode==-1) { // mode is not found in modes
		print_usage();
		exit(EXIT_FAILURE);
	}

	for(int i=0;i<MODE_COUNT;i++) {
		if(modes_used[i] > 0 && i != mode) {
			print_usage();
			exit(EXIT_FAILURE);
		}
	}
	// special case, dry-run is valid in two modes, modes_used was not incremented for it
	if(args->dry_run == 1 && !( mode == MODE_INVALID || mode == MODE_DUPLICATES ) ) {
		print_usage();
		exit(EXIT_FAILURE);
	}

	int marg = argc-optind;
	int invalid = 
		 (mode == MODE_INIT       && marg != 2)
	|| (mode == MODE_INDEX      && marg != 2)
	|| (mode == MODE_GATHERING  && marg != 3)
	|| (mode == MODE_JOIN       && marg < 3)
	|| (mode == MODE_DUPLICATES && (marg < 2 || marg > 3))
	|| (mode == MODE_INVALID    && marg != 2)
	|| (mode == MODE_INFO       && marg != 2);

	if(invalid) {
		print_usage();
		exit(EXIT_FAILURE);
	}


	switch(mode) {
		case MODE_INIT: 
			args->mosaik2db = argv[optind+1]; 
			break;
		case MODE_INDEX:
			args->mosaik2db = argv[optind+1];
			break;
		case MODE_GATHERING: 
			args->dest_image = argv[optind+1];
			args->mosaik2db = argv[optind+2];
			break;
		case MODE_JOIN:
			args->dest_image = argv[optind+1];
      args->mosaik2dbs= &argv[optind+2];
			args->mosaik2dbs_count = marg-2;
			break;
		case MODE_DUPLICATES:
			args->mosaik2db =  argv[optind+1];
			args->mosaik2dbs_count = 0;
			if(marg == 3) {
      	args->mosaik2dbs= &argv[optind+2];
				args->mosaik2dbs_count = 1;
			}
			break;
		case MODE_INVALID:
			args->mosaik2db = argv[optind+1];
			//either or
			if(args->has_element_number==1 && ( args->ignore_old_invalids == 1 || args->dry_run == 1 || args->no_hash_cmp == 1)) {
				print_usage();
				exit(EXIT_FAILURE);
			}
			break;
		case MODE_INFO:
			args->mosaik2db = argv[optind+1];
			break;
	}
	
	if(args->verbose) {
		fprintf (stderr,"mode = %s\n", args->mode);
		fprintf (stderr,"mosaik2db = %s\n", args->mosaik2db);
		for(int i=0;i<args->mosaik2dbs_count;i++) {
			fprintf(stderr,"mosaik2dbs[%i] = %s\n", i, args->mosaik2dbs[i]);
		}
		fprintf (stderr,"dest-image = %s\n", args->dest_image);
		fprintf (stderr,"options:\nverbose = %s\ndry-run = %s\ndatabase_image_resolution = %i\nmax_load = %i\nmax_jobs = %i\nunique = %s\ncolor_stddev_ratio = %i\npixel_per_tile = %i\nduplicate_reduction = %s\nsymlink_cache = %s\nignore_old_invalids = %s\nno_hash_cmp = %s\ncolor-distance = %s\n",
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
              args->no_hash_cmp ? "yes" : "no",
		args->color_distance == MOSAIK2_ARGS_COLOR_DISTANCE_MANHATTAN ? "manhattan" :
			args->color_distance == MOSAIK2_ARGS_COLOR_DISTANCE_EUCLIDIAN ? "euclidian" : "chevychev");
		if(args->has_element_number) {
			fprintf(stderr, "element_number = %i\n", args->element_number);
		} else {
			fprintf(stderr, "element_number = none\n");
		}
		fprintf(stderr, "\n");
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
"  or:  mosaik2 [-V] [-t <NUM>] [-u] [-R <PERCENT>] [-D DIST] gathering dest-image MOSAIK2DB < src-image\n"
"  or:  mosaik2 [-V] [-p <PIXEL>] [-s] [-d] join dest-image MOSAIK2DB_0 [MOSAIK2DB_1, ...]\n"
"  or:  mosaik2 [-V] [-i] [-y] duplicates MOSAIK2DB_0 [MOSAIK2DB_1]\n"
"  or:  mosaik2 [-V] { [-i] [-y] [-n] | -e <NUM> } invalid MOSAIK2DB\n"
"  or:  mosaik2 [-V] [-e <NUM>] info MOSAIK2DB\n"
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
"  -D DIST     Color-Distance-Method: manhatten (default), euclidian, chebyshev\n"
"  -p PIXEL    Image resolution in PIXEL of one image tile in\n"
"              the dest-image (default 200)\n"
"  -s          Symlinks instead of file copies\n"
//"  -c PATH                    Cache path (default ~/.mosaik2)\n"
"  -d          Fast but not complete reduction of duplicates\n"
"  -i          Ignore old invalid images\n"
"  -y          Dry run: does not change the database\n"
"  -n          No hash comparison\n"
"  -e NUM      Print infos of the NUMth database element\n"
"\n"
"Website: https://f7a8.github.io/mosaik2/\n"
"\n"
"Report bugs to https://github.com/f7a8/mosaik2/issues.\n");
}


