/*
    _        _ __  _  _            _
 __| | _  _ | '_ \| |(_) __  __ _ | |_  ___  ___               __
/ _` || || || .__/| || |/ _|/ _` ||  _|/ -_)(_-/         _    / _|
\__/_| \_._||_|   |_||_|\__|\__/_| \__|\___|/__/ _phash (_)   \__|
*/

#include "libmosaik2.h"

int mosaik2_database_phashes_check(mosaik2_database *md) {
	struct stat lastmodified_file, phashes_file;

	m_stat(md->lastmodified_filename, &lastmodified_file);
	m_stat(md->phash_filename, &phashes_file);

	if( phashes_file.st_ctim.tv_sec > lastmodified_file.st_ctim.tv_sec ||
			( phashes_file.st_ctim.tv_sec == lastmodified_file.st_ctim.tv_sec &&
				phashes_file.st_ctim.tv_nsec > lastmodified_file.st_ctim.tv_nsec )) {
		//filehashes index was created after last modified timestamp of that mosaik2_database => still fine
		fprintf(stderr, "phashes file was modified after last modified of the database, has to be rebuild\n");
		return PHASHES_INVALID;
	}

	if(get_file_size(md->phash_filename) != md->element_count * md->phash_sizeof) {
		fprintf(stderr, "phashes file has not that much elements than it could be, has to be rebuild\n");
		return PHASHES_INVALID;
	}

	return PHASHES_VALID;
}

void mosaik2_database_phashes_build(mosaik2_database *md) {

	int old_phash_element_count = get_file_size(md->phash_filename);
	int element_count = md->element_count;
	FILE *filename_index_file = m_fopen(md->filenames_index_filename, "r");

	FILE *phash_file = m_fopen(md->phash_filename, "a");
	fprintf(stderr, "building phashes\n");

	int next_phash_element = 0;
	if(old_phash_element_count > 0)
		next_phash_element=1;
	for(uint32_t element = old_phash_element_count + next_phash_element; element<element_count; element++) {

		char *filename = mosaik2_database_read_element_filename( md, element, filename_index_file);
		unsigned long long hasha=0;
	//	fprintf(stderr, "phashing file %s\n", filename);
		int val = ph_dct_imagehash(filename, &hasha);
		free(filename);
		fwrite(&hasha,sizeof(unsigned long long), 1, phash_file);
		fwrite(&val, sizeof(int), 1, phash_file);
	}
	fprintf(stderr, "building phashes done\n");

	m_fclose(filename_index_file);
	m_fclose(phash_file);
}
