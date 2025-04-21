/*
    _        _ __  _  _            _
 __| | _  _ | '_ \| |(_) __  __ _ | |_  ___  ___               __
/ _` || || || .__/| || |/ _|/ _` ||  _|/ -_)(_-/         _    / _|
\__/_| \_._||_|   |_||_|\__|\__/_| \__|\___|/__/ _phash (_)   \__|
*/
#ifdef HAVE_PHASH
#include "libmosaik2.h"

int mosaik2_database_phashes_check(mosaik2_database *md) {
	struct stat lastindexed_file, phashes_file;

	m_stat(md->lastindexed_filename, &lastindexed_file);
	m_stat(md->phash_filename, &phashes_file);

	if( phashes_file.st_ctim.tv_sec < lastindexed_file.st_ctim.tv_sec ||
			( phashes_file.st_ctim.tv_sec == lastindexed_file.st_ctim.tv_sec &&
				phashes_file.st_ctim.tv_nsec < lastindexed_file.st_ctim.tv_nsec )) {
		//filehashes index was created after last modified timestamp of that mosaik2_database => still fine
		//fprintf(stderr, "phashes file was modified before last index mode of the database, has to be rebuild\n");
		return PHASHES_INVALID;
	}

	if(get_file_size(md->phash_filename) != md->element_count * md->phash_sizeof) {
		//fprintf(stderr, "phashes file has not that much elements than it could be, has to be rebuild\n");
		return PHASHES_INVALID;
	}

	return PHASHES_VALID;
}

void mosaik2_database_phashes_build(mosaik2_database *md, mosaik2_arguments *args) {
	if(args->verbose)fprintf(stderr, "create/update phash file\n");
	unsigned char invalid = 0;
	int old_phash_element_count = get_file_size(md->phash_filename)/md->phash_sizeof;
	m2elem element_count = md->element_count;
	m2file filename_index_file = m_fopen(md->filenames_index_filename, "r");
	m2file invalid_file = m_fopen(md->invalid_filename,"r");
	m2file phash_file = m_fopen(md->phash_filename, "a");

	int next_phash_element = 0;
	if(old_phash_element_count > 0)
		next_phash_element=1;

	if(!args->quiet) {
		if(old_phash_element_count==0)
		fprintf(stderr, "building %u phashes for %s\n", element_count, md->thumbs_db_name);
		else
		fprintf(stderr, "append %u phashes for %s\n", element_count-old_phash_element_count, md->thumbs_db_name);
		
	}
	for(m2elem element = old_phash_element_count + next_phash_element; element<element_count; element++) {

		//void read_file_entry(m2file file, void *val, size_t len, off_t offset); 
		read_file_entry(invalid_file, &invalid, md->invalid_sizeof, element*md->invalid_sizeof);
		m2name filename = mosaik2_database_read_element_filename( md, element, filename_index_file, NULL);

		m2phash my_phash = {0};
		my_phash.val = PHASHES_INVALID;

		//phash only valid images
		if(invalid == INVALID_NONE) {
			if(args->verbose)
				fprintf(stderr, "phashing %u/%u file %s\n", element, element_count, filename);
			my_phash.val = ph_dct_imagehash(filename, &my_phash.hash);
		} else {
			if(args->verbose)
				fprintf(stderr, "ignore %u/%u invalid file %s\n", element, element_count, filename);
		}
		m_free((void**)&filename);

		m_fwrite(&my_phash, md->phash_sizeof, phash_file);
	}
	if(!args->quiet)
		fprintf(stderr, "building phashes done\n");

	m_fclose(filename_index_file);
	m_fclose(phash_file);
}
#endif
