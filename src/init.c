
#include "libmosaik2.h"


/*
 * Return a hex string representing the data pointed to by `p`,
 * converting `n` bytes.
 *
 * The string should be deallocated using `free()` by the caller.
 */
char *phex(const void *p, size_t n)
{
    const unsigned char *cp = p;              /* Access as bytes. */
    char *s = malloc(2*n + 1);       /* 2*n hex digits, plus NUL. */
    size_t k;

    /*
     * Just in case - if allocation failed.
     */
    if (s == NULL)
        return s;

    for (k = 0; k < n; ++k) {
        /*
         * Convert one byte of data into two hex-digit characters.
         */
        sprintf(s + 2*k, "%02x", cp[k]);
    }

    /*
     * Terminate the string with a NUL character.
     */
    s[2*n] = '\0';

    return s;
}

/**
	* create as new empty mosaik2 database file. In case of an error it exits the program.
  */
FILE* create_mosaik2_database_file(char *filename, uint8_t close, uint8_t print) {
	FILE *f =	fopen(filename,"w");
	if( f == NULL ) { fprintf( stderr, "could not create mosaik2 database file (%s)\n", filename); exit(EXIT_FAILURE); }
	if( close == 1 && fclose( f ) != 0 ) {
		fprintf( stderr, "could not close mosaik2 database file (%s)\n", filename); exit(EXIT_FAILURE);
	}
	if( print == 1 ) fprintf(stdout, "mosaik2 database file %s created\n", filename);
	return f;
}

void create_mosaik2_database_file_id(char *filename) {
	FILE *file = create_mosaik2_database_file(filename, 0, 0);

	FILE *rand_file = fopen("/dev/random", "r");
	if( rand_file == NULL ) { fprintf( stderr, "could not open random file for id generation"); exit(EXIT_FAILURE); }
	unsigned char buf[8];
	unsigned char buf2[17];
	memset( buf, 0, 8 );
	memset( buf2, 0, 17);
	if( 8 == fread(buf, 1, 8, rand_file) ) {
		unsigned char *hexbuf = phex( buf, 8);
		if(16 != fwrite(hexbuf, 1, 16, file)) {
			free(hexbuf);
			
			fprintf(stderr, "could not write random value to id file\n");
			exit(EXIT_FAILURE);
		}
		free(hexbuf);
	} else {
		fprintf(stderr, "could not read random value from /dev/random\n");
		exit(EXIT_FAILURE);
	}
	if( fclose( file ) != 0 ) { fprintf( stderr, "could not close id file\n"); exit(EXIT_FAILURE); }
	if( fclose( rand_file ) != 0 ) { fprintf( stderr, "could not close /dev/random file\n"); exit(EXIT_FAILURE); }
	printf( "mosaik2 database file %s created\n", filename);
}

void create_mosaik2_database_file_version(char *filename) {
	FILE *file = create_mosaik2_database_file(filename, 0, 1);
	if( fprintf(file, "%i", MOSAIK2_DATABASE_FORMAT_VERSION) < 1 ) {
		fprintf(stderr, "could not write mosaik2 database version file\n");
		exit(EXIT_FAILURE);
	}
	if( fclose(file) != 0 ) {
		fprintf(stderr, "could not close mosaik2 database version file\n");
		exit(EXIT_FAILURE);
	}
}

void create_mosaik2_database_file_tilecount(char *filename, uint8_t tilecount) {
	FILE *file = create_mosaik2_database_file(filename, 0, 1);
	if ( fprintf(file, "%i", tilecount) < 1 ) {
		fprintf(stderr, "could not write mosaik2 database tilecount file\n");
		exit(EXIT_FAILURE);
	}
	if( fclose(file) != 0 ) {
		fprintf(stderr, "could not close mosaik2 database version file\n");
		exit(EXIT_FAILURE);
	}
}

void create_mosaik2_database_file_readme(char *filename) {
	FILE *file = create_mosaik2_database_file(filename, 0, 1);
	fprintf(file, "This is a mosaik2 database directory.\n\nmosaik2 creates real photo mosaics especially like from large datasets.\nView the projects website at https://f7a8.github.io/mosaik2/\n");
	if( fclose( file) != 0 ) {
		fprintf(stderr, "could not close mosaik2 database id file\n");
		exit(EXIT_FAILURE);
	}
}

int mosaik2_init(char *mosaik2_database_name, uint32_t tilecount) {
	
	int debug=0;

	mosaik2_database md;
	init_mosaik2_database(&md, mosaik2_database_name);

	check_resolution(tilecount);

	if( mkdir(mosaik2_database_name, S_IRWXU | S_IRGRP | S_IROTH ) != 0) {	
		fprintf(stderr,"mosaik2 database directory (%s) could not be created: %s\n", mosaik2_database_name, strerror(errno));
		exit(EXIT_FAILURE);
	}
	printf("mosaik2 database directory (%s) created\n", mosaik2_database_name);

	create_mosaik2_database_file(md.filesizes_filename, 1, 1);
	create_mosaik2_database_file(md.filenames_filename, 1, 1);
	create_mosaik2_database_file(md.filenames_index_filename, 1, 1);
	create_mosaik2_database_file(md.filehashes_filename, 1, 1);
	create_mosaik2_database_file(md.filehashes_index_filename, 1, 1);
	create_mosaik2_database_file(md.timestamps_filename, 1, 1);
	create_mosaik2_database_file(md.imagedims_filename, 1, 1);
	create_mosaik2_database_file(md.tiledims_filename, 1, 1);
	create_mosaik2_database_file(md.imagecolors_filename, 1, 1);
	create_mosaik2_database_file(md.imagestddev_filename, 1, 1);
	create_mosaik2_database_file(md.image_index_filename, 1, 1);
	create_mosaik2_database_file(md.invalid_filename, 1, 1);
	create_mosaik2_database_file(md.duplicates_filename, 1, 1);
	create_mosaik2_database_file(md.tilecount_filename, 1, 1);
	create_mosaik2_database_file(md.lock_filename, 1, 1);
	create_mosaik2_database_file(md.lastmodified_filename, 1, 1);

	create_mosaik2_database_file_id(md.id_filename);
	create_mosaik2_database_file_version(md.version_filename);
	create_mosaik2_database_file_tilecount(md.tilecount_filename, tilecount);
	create_mosaik2_database_file_readme(md.readme_filename);

	return 0;
}

