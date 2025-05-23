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
#define OPTION_D_LO 0
#define OPTION_D_HI 1
#define OPTION_H 2
#define OPTION_E_LO 3
#define OPTION_E_HI 4
#define OPTION_I 5
#define OPTION_J 6
#define OPTION_L 7
#define OPTION_N 8
#define OPTION_Q 9
#define OPTION_P_LO 10
#define OPTION_P_HI 11
#define OPTION_R_LO 12
#define OPTION_R_HI 13
#define OPTION_S 14
#define OPTION_T 15
#define OPTION_U_LO 16
#define OPTION_U_HI 17
#define OPTION_V_LO 18
#define OPTION_V_HI 19
#define OPTION_Y 20
#define OPTION_COUNT 21



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
	args->pixel_per_tile = 200;
	args->color_distance = MOSAIK2_ARGS_COLOR_DISTANCE_DEFAULT;
	args->has_src_image_resolution = 0;
	args->has_element_identifier = 0;
	args->element_filename = NULL;
	args->element_number = 0;
	args->has_phash_distance = 0;
	args->phash_distance = 0;

	char *modes[] = {"init", "index", "gathering", "join", "duplicates", "invalid", "info","crop"};
	
	int modes_used[MODE_COUNT] = {0};
	// count usage of options, to prevent multiple occurances of the same option
	int options_used[OPTION_COUNT] = {0};

	char *all_options = "dD:he:E:ij:l:nqp:P:r:R:st:uUvVy?";
	char *all_options_array[] = {"d","D","h","e","E","i","j","l","n","q","p","P","r","R","s","t","u","U","v","V","y"};

	int opt;
	while((opt = getopt(argc, argv, all_options)) != -1 ) {
		switch(opt) {
			case 'd': options_used[OPTION_D_LO]++; break;
			case 'D': options_used[OPTION_D_HI]++; break;
			case 'e': options_used[OPTION_E_LO]++; break;
			case 'E': options_used[OPTION_E_HI]++; break;
			case 'h': options_used[OPTION_H]++; break;
			case 'i': options_used[OPTION_I]++; break;
			case 'j': options_used[OPTION_J]++; break;
			case 'l': options_used[OPTION_L]++; break;
			case 'n': options_used[OPTION_N]++;	break;
			case 'p': options_used[OPTION_P_LO]++; break;
			case 'P': options_used[OPTION_P_HI]++; break;
			case 'q': options_used[OPTION_Q]++; break;
			case 'r': options_used[OPTION_R_LO]++; break;
			case 'R': options_used[OPTION_R_HI]++; break;
			case 's': options_used[OPTION_S]++; break;
			case 't': options_used[OPTION_T]++; break;
			case 'u': options_used[OPTION_U_LO]++; break;
			case 'U': options_used[OPTION_U_HI]++; break;
			case 'v': options_used[OPTION_V_LO]++; break;
			case 'V': options_used[OPTION_V_HI]++; break;
			case 'y': options_used[OPTION_Y]++; break;
		}
	};

	for (int option_i = 0; option_i < OPTION_COUNT; option_i++) {
		if (option_i != OPTION_E_HI && options_used[option_i] > 1) {
			fprintf(stderr, "option %s is allowed only one time.\n\n",
					all_options_array[option_i]);
			print_usage();
			exit(EXIT_FAILURE);
		}
	}

	args->exclude_count = options_used[OPTION_E_HI];
	args->exclude_area = (char **)m_calloc(args->exclude_count, sizeof(char *));

	int exclude_index=0;
	optind=1; // reset getopt
	while((opt = getopt(argc, argv, all_options)) != -1 ) {
		switch(opt) {
			case 'd': args->duplicate_reduction = 1; 
								modes_used[MODE_JOIN]++;
								break;
			case 'D':
								if(strcmp("manhattan", optarg) == 0) {
									args->color_distance = MOSAIK2_ARGS_COLOR_DISTANCE_MANHATTAN;
								} else if(strcmp("euclidian", optarg) == 0) {
									args->color_distance = MOSAIK2_ARGS_COLOR_DISTANCE_EUCLIDIAN;
								} else if(strcmp("chebyshev", optarg) == 0) {
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
					long long element_number = atoll(optarg);
					if(element_number<1 || element_number>UINT32_MAX) { 
						// user input 1 means index 0 => lowest value allowed
						fprintf(stderr, "element number out of range\n"); 
						exit(EXIT_FAILURE);
					}
					args->element_number = (m2elem)element_number;
					args->has_element_identifier = ELEMENT_NUMBER; // ELEMENT_NUMBER
				} else {
					args->element_filename = argv[optind-1];
					args->has_element_identifier = ELEMENT_FILENAME; //ELEMENT_FILENAME;
				}
				break; //no modes_used because it appears in several modes
			case 'E':
				args->exclude_area[exclude_index] = argv[optind-1];
				//fprintf(stderr, "argparser %i:[%s]\n", e, args->exclude_area[e] );
				exclude_index++;
				modes_used[MODE_GATHERING]++;
				break;
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
			case 'r':
				if(is_number(optarg)) {
					int32_t database_image_resolution = atoi(optarg);
					check_resolution(database_image_resolution);
					args->database_image_resolution=(m2rezo)database_image_resolution;// no modes_used because it appears in serveral modes
				} else {
					fprintf(stderr, "-r must be a postivie number\n");
					exit(EXIT_FAILURE);
				} 
				modes_used[MODE_INIT]++;
				break;
			case 's': args->symlink_cache = 1;
								modes_used[MODE_JOIN]++;
								break;
			case 't': 
				if(is_number(optarg)) {
					int32_t src_image_resolution = atoi(optarg);
					check_resolution(src_image_resolution);
					args->src_image_resolution=(m2rezo)src_image_resolution;// no modes_used because it appears in serveral modes
					args->has_src_image_resolution=1; 
				} else {
					fprintf(stderr, "-t must be a positive number\n");
					exit(EXIT_FAILURE);
				}
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
	if(args->has_src_image_resolution == 1 && !( mode == MODE_GATHERING || mode == MODE_CROP || mode == MODE_INFO ) ) {
		print_usage();
		exit(EXIT_FAILURE);
	}
	if(args->has_src_image_resolution == 0 && mode == MODE_GATHERING ) {
		args->src_image_resolution = 20;
	} else if( args->has_src_image_resolution == 0 && mode == MODE_CROP ) {
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
	|| (mode == MODE_INDEX      && marg != 3)
	|| (mode == MODE_GATHERING  && marg != 4)
	|| (mode == MODE_JOIN       && marg < 3)
	|| (mode == MODE_DUPLICATES && (marg < 2 || marg > 3))
	|| (mode == MODE_INVALID    && marg != 2)
	|| (mode == MODE_INFO       && (marg < 2 || marg > 3))
	|| (mode == MODE_CROP        && marg != 2);

	if(invalid) {
		print_usage();
		exit(EXIT_FAILURE);
	}

	int _is_dir=0;
	int _is_src_file=0;
	int _is_dest_file=0;
	int c[3]={0};

	switch(mode) {
		case MODE_INIT: 
			args->mosaik2db = argv[optind+1]; 
			break;
		case MODE_INDEX:
			if(dir_exists(argv[optind+1])) {
				args->mosaik2db = argv[optind+1];
				args->index_filelist = argv[optind+2];
			} else {
				args->index_filelist = argv[optind+1];
				args->mosaik2db = argv[optind+2];
			}
			break;
		case MODE_GATHERING: 
			
			if(args->unique>0 && args->fast_unique>0) {
				print_usage();
				exit(EXIT_FAILURE);
			}

			for(int i=1;i<=3;i++) {
				if(M2DEBUG)fprintf(stderr, "  %i %s:\n",i, argv[ optind+i ] );
				if(dir_exists(argv[ optind+i ])){
					if(M2DEBUG)fprintf(stderr, "dir_exists ");
					c[0]++;
					_is_dir=i;
				}
				if(is_src_file(argv[optind+i])) {
					if(M2DEBUG)fprintf(stderr, "is_src_file ");
					c[1]++;
					_is_src_file = i;
				}
				if(is_dest_file(argv[optind+i])){
					if(M2DEBUG)fprintf(stderr, "is_dest_file ");
					c[2]++;
					_is_dest_file = i;
				}
					if(M2DEBUG)fprintf(stderr, "\n");
			}
			if(M2DEBUG)fprintf(stderr, "\n");

			if(c[0]==0)
				fprintf(stderr, "No mosaik2db specified.\n");
			else if(c[0]>1)
				fprintf(stderr, "Multiple mosaik2db specified.\n");
			else if(c[1] ==0 ) {
				fprintf(stderr, "No existing src-file specified.\n");
			}else if(c[1]>1) {
				fprintf(stderr, "Multiple src-file specified.\n");
			} else if(c[2]==0) {
				fprintf(stderr, "No dest-file specified.\n");
			} else if(c[2]>1) {
				fprintf(stderr, "Cannot distinguish between src-file and dest-file. Please use another (new) dest_file.\n");
			}
		
			if(c[0] != 1 || c[1]!=1 || c[2]!=1 ) {
				print_usage();
				exit(EXIT_FAILURE);
			}
			//check_resolution
			args->mosaik2db = argv[optind+_is_dir];
			args->src_image = argv[optind+_is_src_file];
			args->dest_image = argv[optind+_is_dest_file];

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
			if(marg==3) { // if src_image is specified -t src_image_resolution is required
				if( ! args->has_src_image_resolution ) {
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
		fprintf (stderr,"src-image = %s\n", args->src_image);
		fprintf (stderr,"options:\nverbose = %s\nquiet = %s\ndry-run = %s\n"
		"database_image_resolution = %i\nsrc_image_resolution = %i\nmax_load = %i\n"
		"max_jobs = %i\nunique = %s\nfast-unique = %s\n"
		"pixel_per_tile = %i\nphash duplicate = %i\nreduction = %s\n"
		"symlink_cache = %s\nignore_old_invalids = %s\nno_hash_cmp = %s\n"
		"color-distance = %s\n",
              args->verbose ? "yes" : "no",
              args->quiet ? "yes" : "no",
              args->dry_run ? "yes" : "no",
              args->database_image_resolution,
              args->src_image_resolution,
              args->max_load,
              args->max_jobs,
              args->unique ? "yes" : "no",
              args->fast_unique ? "yes" : "no",
              args->pixel_per_tile,
              args->phash_distance,
              args->duplicate_reduction ? "yes" : "no",
              args->symlink_cache ? "yes" : "no",
              args->ignore_old_invalids ? "yes" : "no",
              args->no_hash_cmp ? "yes" : "no",
              args->color_distance == MOSAIK2_ARGS_COLOR_DISTANCE_MANHATTAN ? "manhattan" :
            		  args->color_distance == MOSAIK2_ARGS_COLOR_DISTANCE_EUCLIDIAN ? "euclidian" : "chevychev");
		fprintf(stderr, "exclude_area = ");
		if(args->exclude_count==0) {
			fprintf(stderr, "no\n");
		} else {
			for(int i=0;i<args->exclude_count;i++) {
				fprintf(stderr, "%s ", args->exclude_area[i]);
			}
			fprintf(stderr, "\n");
		}
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
	for(size_t i=0,len=strlen(string);i<len&&is_digit;i++) {
	 	if(string[i] <=57 && string[i] >=48) {
			is_digit = 1;
		} else {
			is_digit = 0;
		}
	}
	return is_digit;
}

void cleanup(mosaik2_arguments *args) {
	
	if( args->exclude_count>0) {
		m_free((void **)&args->exclude_area);
	}

}
