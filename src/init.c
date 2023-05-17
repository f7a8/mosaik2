
#include "libmosaik2.h"


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

void create_mosaik2_database_file_id(mosaik2_database *md, int print) {
	FILE *file = create_mosaik2_database_file(md->id_filename, 0, 0);
	FILE *rand_file = m_fopen("/dev/random", "r");

	uint8_t buf[8];
	m_fread(buf, 8, rand_file);
	snprintf(md->id, md->id_len, "%02x%02x%02x%02x%02x%02x%02x%02x", buf[0], buf[1], buf[2], buf[3], buf[4], buf[5], buf[6], buf[7]);
	m_fwrite(md->id, md->id_len, file);

	m_fclose( file );
	m_fclose( rand_file );
	if(print) printf( "mosaik2 database file %s created\n", md->id_filename);
}

void create_mosaik2_database_file_readme(char *filename, int print) {
	FILE *file = create_mosaik2_database_file(filename, 0, print);
	fprintf(file, "This is a mosaik2 database directory.\n\nmosaik2 creates real photo mosaics especially like from large datasets.\nView the projects website at https://f7a8.github.io/mosaik2/\n");
	m_fclose( file);
}

void create_mosaik2_database_file_val(char *filename, void *ptr, size_t len, int print) {
	FILE *file = create_mosaik2_database_file(filename, 0, print);
	m_fwrite(ptr, len, file);
	m_fclose(file);
}

int mosaik2_init(mosaik2_arguments *args) {

	char *mosaik2_database_name = args->mosaik2db;
	int database_image_resolution = args->database_image_resolution;
	time_t n = time(NULL);
	int mdfv = MOSAIK2_DATABASE_FORMAT_VERSION;


	mosaik2_database md;
	init_mosaik2_database(&md, mosaik2_database_name);

	check_resolution(database_image_resolution);

	if( mkdir(mosaik2_database_name, S_IRWXU | S_IRGRP | S_IROTH ) != 0) {
		fprintf(stderr,"mosaik2 database directory (%s) could not be created: %s\n", mosaik2_database_name, strerror(errno));
		exit(EXIT_FAILURE);
	}
	create_mosaik2_database_file_id(&md, args->verbose);
	printf("mosaik2 database directory (%s) created (id:%s)\n", mosaik2_database_name, md.id);

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
	create_mosaik2_database_file(md.lastindexed_filename, 1, args->verbose);

	create_mosaik2_database_file_val(md.version_filename, &mdfv, sizeof(mdfv),  args->verbose);
	create_mosaik2_database_file_val(md.database_image_resolution_filename, &database_image_resolution, sizeof(database_image_resolution), args->verbose);
	create_mosaik2_database_file_val(md.createdat_filename, &n, md.createdat_sizeof, args->verbose);
	create_mosaik2_database_file_readme(md.readme_filename, args->verbose);

	return 0;
}

