int mosaik2_init(char *mosaik2_database_name, uint32_t tilecount);
int mosaik2_clean(char *mosaik2_database_name);
int mosaik2_index(char *mosaik2_database_name,  uint32_t max_tiler_processes, uint32_t max_loadavg);

/* usage param 1=> tile_count, 2=> file_size of the image in bytes. Image data is only accepted via stdin stream */
int mosaik2_tiler(uint32_t tile_count, uint32_t filesize);

/**  	1 => master_tile_count (only approx. depends on the input image, can be slightly more)
	2 => file_size in bytes
	3 => dest_filename (including jpeg or png suffix)
	4 => ratio (0<=ratio<=100) of the weightning between image color and image standard deviation of the color (100 could be a good starting value)
	5 => unique (0 or 1) use a tile at least one time
	6 => pathname to mosaik2_thumb_db
*/
int mosaik2_gathering(int master_tile_count, size_t filesize, char *dest_filename, int ratio, int unique, char *mosaik2_db_name);

/*	1=> dest_filename (including jpeg or png suffix)
	2=> image width in per master tile in px
	3=> unique_tiles ( 1 or 0 ) duplicate tiles can be supressed as much as thumbs_db are involved
	4 => local_cache ( 1 copy files into ~/.mosaik2/, 0 creates symbolic links),
	5 => thumbs_db_name_1
*/
int mosaik2_join(char *dest_filename, int image_width, int unique_tiles, int local_cache, int argc, char **argv);

/* 1=> mosaik2_db_dir, 2=> mosaik2_db_dir, 3=> dry_run (0 or 1) */
int mosaik2_duplicates(char *mosaik2_db_name_1, char *mosaik2_db_name_2, int dry_run);

/* 1=> mosaik2_db_dir, 2=> ignore_old_ivalids ( 0 or 1; if 1 already as invalid marked files are not checked again ), 3=> dry_run (0 or 1; if 1, then nothing i save to the invalid file) */
int mosaik2_invalid(char *mosaik2_db_name, int ignore_old_invalids, int dry_run);

