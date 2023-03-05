
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
    char *s = m_malloc(2*n + 1);       /* 2*n hex digits, plus NUL. */
    size_t k;

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
	FILE *f = m_fopen(filename,"w");
	if( close == 1 )
		m_fclose( f );
	if( print == 1 ) fprintf(stdout, "mosaik2 database file %s created\n", filename);
	return f;
}

void create_mosaik2_database_file_id(char *filename, int print) {
	FILE *file = create_mosaik2_database_file(filename, 0, 0);
	FILE *rand_file = m_fopen("/dev/random", "r");

	unsigned char buf[8];
	unsigned char buf2[17];
	memset( buf, 0, 8 );
	memset( buf2, 0, 17);

	m_fread(buf, 8, rand_file);
	char *hexbuf = phex( buf, 8);
	m_fwrite(hexbuf, 16, file);
	free(hexbuf);

	m_fclose( file );
	m_fclose( rand_file );
	if(print) printf( "mosaik2 database file %s created\n", filename);
}

void create_mosaik2_database_file_readme(char *filename, int print) {
	FILE *file = create_mosaik2_database_file(filename, 0, print);
	fprintf(file, "This is a mosaik2 database directory.\n\nmosaik2 creates real photo mosaics especially like from large datasets.\nView the projects website at https://f7a8.github.io/mosaik2/\n");
	m_fclose( file);
}

void create_mosaik2_database_file_int(char *filename, int value, int print) {
	FILE *file = create_mosaik2_database_file(filename, 0, print);
	if ( fprintf(file, "%i", value) < 1 ) {
		fprintf(stderr, "could not write mosaik2 database file (%s)\n", filename);
		exit(EXIT_FAILURE);
	}
	m_fclose(file);
}

int mosaik2_init(mosaik2_arguments *args) {
	
	char *mosaik2_database_name = args->mosaik2db;
	int database_image_resolution = args->database_image_resolution;


	mosaik2_database md;
	init_mosaik2_database(&md, mosaik2_database_name);

	check_resolution(database_image_resolution);

	if( mkdir(mosaik2_database_name, S_IRWXU | S_IRGRP | S_IROTH ) != 0) {
		fprintf(stderr,"mosaik2 database directory (%s) could not be created: %s\n", mosaik2_database_name, strerror(errno));
		exit(EXIT_FAILURE);
	}
	printf("mosaik2 database directory (%s) created\n", mosaik2_database_name);

	create_mosaik2_database_file(md.filesizes_filename, 1, args->verbose);
	create_mosaik2_database_file(md.filenames_filename, 1, args->verbose);
	create_mosaik2_database_file(md.filenames_index_filename, 1, args->verbose);
	create_mosaik2_database_file(md.filehashes_filename, 1, args->verbose);
	create_mosaik2_database_file(md.filehashes_index_filename, 1, args->verbose);
	create_mosaik2_database_file(md.timestamps_filename, 1, args->verbose);
	create_mosaik2_database_file(md.imagedims_filename, 1, args->verbose);
	create_mosaik2_database_file(md.tiledims_filename, 1, args->verbose);
	create_mosaik2_database_file(md.imagecolors_filename, 1, args->verbose);
	create_mosaik2_database_file(md.imagestddev_filename, 1, args->verbose);
	create_mosaik2_database_file(md.image_index_filename, 1, args->verbose);
	create_mosaik2_database_file(md.invalid_filename, 1, args->verbose);
	create_mosaik2_database_file(md.duplicates_filename, 1, args->verbose);
	create_mosaik2_database_file(md.database_image_resolution_filename, 1, args->verbose);
	create_mosaik2_database_file(md.lock_filename, 1, args->verbose);
	create_mosaik2_database_file(md.lastmodified_filename, 1, args->verbose);
	create_mosaik2_database_file(md.tileoffsets_filename, 1, args->verbose);

	create_mosaik2_database_file_id(md.id_filename, args->verbose);
	create_mosaik2_database_file_int(md.version_filename, MOSAIK2_DATABASE_FORMAT_VERSION, args->verbose);
	create_mosaik2_database_file_int(md.database_image_resolution_filename, database_image_resolution, args->verbose);
	create_mosaik2_database_file_readme(md.readme_filename, args->verbose);

	return 0;
}

