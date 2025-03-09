#include <libgen.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <openssl/md5.h>
#include <err.h>
#include <unistd.h>
#include <error.h>



#include "libmosaik2.h"
#include "data_types.h"
#include "mosaik2.h"

#define MODE_INIT 0
#define MODE_INDEX 1
#define MODE_GATHERING 2
#define MODE_JOIN 3
#define MODE_DUPLICATES 4
#define MODE_INVALID 5
#define MODE_INFO 6
#define MODE_CROP 7
#define MODE_COUNT 8

void print_usage();
void print_help();
void print_version();
void get_mosaik2_arguments(mosaik2_arguments *args, int argc, char **argv);
int is_number(char *string);
void cleanup(mosaik2_arguments *args);

int main(int argc, char **argv) {

	mosaik2_arguments args;
	get_mosaik2_arguments(&args, argc, argv);

	if(strcmp( args.mode, "init") == 0) {
		return mosaik2_init(&args);
	} else if(strcmp( args.mode, "index") == 0) {
		return mosaik2_index(&args);
	} else if(strcmp( args.mode, "gathering") == 0) {
		return mosaik2_gathering(&args);
	} else if(strcmp( args.mode, "join") == 0) {
		return mosaik2_join(&args);
	} else if(strcmp( args.mode, "invalid") == 0) {
		return mosaik2_invalid(&args);
	} else if(strcmp( args.mode, "duplicates") == 0) {
		return mosaik2_duplicates(&args);
	} else if(strcmp( args.mode, "info") == 0) {
		return mosaik2_info(&args);
	} else if(strcmp( args.mode, "crop") == 0) {
		return mosaik2_crop(&args);
	}

	cleanup(&args);

	return 0;
}

void get_mosaik2_arguments(mosaik2_arguments *args, int argc, char **argv) {

	memset(args,0,sizeof(mosaik2_arguments));

	args->database_image_resolution = 16;
	args->color_stddev_ratio = 100;
	args->pixel_per_tile = 200;
	args->color_distance = MOSAIK2_ARGS_COLOR_DISTANCE_DEFAULT;
	args->has_num_tiles = 0;
	args->element_number = 1;
	args->has_element_identifier = 0;
	args->element_filename = NULL;
	args->element_number = 0;
	args->has_phash_distance = 0;
	args->phash_distance = 0;

	char *modes[] = {"init", "index", "gathering", "join", "duplicates", "invalid", "info","crop"};
	
	int modes_used[] = {0,0,0,0,0,0,0,0,0};

	char *all_options = "dD:he:ij:l:nqp:P:r:R:st:uUvVy?";

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
			case 'e':
				if(is_number(optarg)) {
					args->element_number = atoll(optarg);
					args->has_element_identifier = 1; // ELEMENT_NUMBER
				} else {
					args->element_filename = argv[optind-1];
					args->has_element_identifier = 2; //ELEMENT_FILENAME;
				}
				break; //no modes_used because it appears in several modes
			case 'h': print_usage(); print_help(); exit(EXIT_SUCCESS); break;
			case 'i': args->ignore_old_invalids = 1; 
								break; //no modes_used because it appears in several modes
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
			case 'P': args->phash_distance = atoi(optarg);
				  args->has_phash_distance = 1;
				  modes_used[MODE_DUPLICATES]++;
				  break;
			case 'q': args->quiet = 1;
				  break; // no modes_used because it appears int several modes
			case 'r': args->database_image_resolution = atoi(optarg);
								modes_used[MODE_INIT]++;
								break;
			case 'R': args->color_stddev_ratio = atoi(optarg);
								modes_used[MODE_GATHERING]++;
								break;
			case 's': args->symlink_cache = 1;
								modes_used[MODE_JOIN]++;
								break;
			case 't': args->num_tiles = atoi(optarg);// no modes_used because it appears in serveral modes
				args->has_num_tiles = 1;
								break;
			case 'u': args->unique = 1;
								modes_used[MODE_GATHERING]++;
								break;
			case 'U': args->fast_unique = 1;
								modes_used[MODE_GATHERING]++;
								break;
			case 'v': print_version(); exit(EXIT_SUCCESS); 
			case 'V': args->verbose = 1; break;// no modes_used because it appears in serveral modes 
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
		if(strncmp(args->mode, modes[i], strlen(modes[i]))==0) {
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
	if(mode == MODE_GATHERING)
	// special case, dry-run is valid in two modes, modes_used was not incremented for it
	if(args->dry_run == 1 && !( mode == MODE_INVALID || mode == MODE_DUPLICATES ) ) {
		print_usage();
		exit(EXIT_FAILURE);
	}
	if(args->ignore_old_invalids==1 && !(mode==MODE_INVALID || mode == MODE_DUPLICATES)) {
		print_usage();
		exit(EXIT_FAILURE);
	}
	if(args->has_num_tiles == 1 && !( mode == MODE_GATHERING || mode == MODE_CROP || mode == MODE_INFO ) ) {
		print_usage();
		exit(EXIT_FAILURE);
	}
	if(args->has_num_tiles == 0 && mode == MODE_GATHERING ) {
		args->num_tiles = 20;
	} else if( args->has_num_tiles == 0 && mode == MODE_CROP ) {
		// no default value!
		print_usage();
		exit(EXIT_FAILURE);
	}
	if( args->quiet == 1 && args->verbose == 1) {
		// there can be only one
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
	|| (mode == MODE_INFO       && (marg < 2 || marg > 3))
	|| (mode == MODE_CROP        && marg != 2);

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
			if(args->unique>0 && args->fast_unique>0) {
				print_usage();
				exit(EXIT_FAILURE);
			}
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
			if(args->has_element_identifier == 1 && ( args->ignore_old_invalids == 1 || args->dry_run == 1 || args->no_hash_cmp == 1)) {
				print_usage();
				exit(EXIT_FAILURE);
			}
			break;
		case MODE_INFO:
			if(marg==3) { // if src_image is specified -t num_tiles is required
				if( ! args->has_num_tiles ) {
					print_usage();
					exit(EXIT_FAILURE);
				}
				// more flexibility of the argument position of the file name
				if(is_file(argv[optind+1])) {
					args->src_image = argv[optind+1];
					args->mosaik2db = argv[optind+2];
				} else {
					args->src_image = argv[optind+2];
					args->mosaik2db = argv[optind+1];

				}
			} else {
				args->mosaik2db = argv[optind+1];
			}
			break;
		case MODE_CROP:
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
		fprintf (stderr,"options:\nverbose = %s\nquiet = %s\ndry-run = %s\ndatabase_image_resolution = %i\nmax_load = %i\nmax_jobs = %i\nunique = %s\nfast-unique = %s\ncolor_stddev_ratio = %i\npixel_per_tile = %i\nphash duplicate = %i\nreduction = %s\nsymlink_cache = %s\nignore_old_invalids = %s\nno_hash_cmp = %s\ncolor-distance = %s\nnum_tiles = %i\n",
              args->verbose ? "yes" : "no",
              args->quiet ? "yes" : "no",

              args->dry_run ? "yes" : "no",
              args->database_image_resolution,
              args->max_load,
              args->max_jobs,
              args->unique ? "yes" : "no",
              args->fast_unique ? "yes" : "no",
              args->color_stddev_ratio,
              args->pixel_per_tile,
              args->phash_distance,
              args->duplicate_reduction ? "yes" : "no",
              args->symlink_cache ? "yes" : "no",
              args->ignore_old_invalids ? "yes" : "no",
              args->no_hash_cmp ? "yes" : "no",
              args->color_distance == MOSAIK2_ARGS_COLOR_DISTANCE_MANHATTAN ? "manhattan" :
            		  args->color_distance == MOSAIK2_ARGS_COLOR_DISTANCE_EUCLIDIAN ? "euclidian" : "chevychev",
              args->num_tiles);
		if(args->has_element_identifier == 1 ) {
			fprintf(stderr, "element_number = %i\n", args->element_number);
		} else {
			fprintf(stderr, "element_number = none\n");
		}
		if(args->has_element_identifier == 2 ) {
			fprintf(stderr, "element_filename = %s", args->element_filename);
		} else {
			fprintf(stderr, "element_filename = none\n");
		}
		fprintf(stderr, "\n");
	}
}

void print_version() {
        puts(PACKAGE " version " VERSION);
        puts("Compile-time switches: "
#ifdef DEBUG
                "debug "
#endif
#ifdef HAVE_PHASH
		"phash "
#endif
#ifdef HAVE_LIBCURL
		"curl "
#endif
#ifdef HAVE_LIBEXIF
		"exif "
#endif
	);
}

void print_usage() {
	fprintf(stdout,
#include "usage.inc"
);
}

void print_help() {
	fprintf(stdout,
#include "help.inc"
);
}

int is_number(char *string) {

	int is_digit = 1;
	int j=0;
	size_t len = strlen(string);
	while(j<len && is_digit == 1){
		//fprintf(stderr, "j:%i c:%c:%i (%i %i) => ", j , string[j], string[j], '0', '9');
	 	if(string[j] <=57 && string[j] >=48) {
			//fprintf(stderr, "is digit\n");
			is_digit = 1;
		} else {
			return 0;
			//fprintf(stderr, "no digit\n");
	    		is_digit = 0;
		}
	  	j++;
	}
	//fprintf(stderr, "RETURN VALUE is_digit:%i\n", is_digit);
	return is_digit;
}


void cleanup(mosaik2_arguments *args) {
	if( args->has_element_identifier == 2) {
		free( args->element_filename );
	}
}
