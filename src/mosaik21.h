
#include <inttypes.h>
#include <math.h>
#include <openssl/md5.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <gd.h>
#include <libexif/exif-data.h>

#ifndef _MOSAIK21_H_
#define _MOSAIK21_H_
#define MAX_FILENAME_LEN 1024
#define MAX_TEMP_FILENAME_LEN 100

extern const int FT_JPEG;
extern const int FT_PNG;
extern const int FT_ERR;

extern uint8_t ORIENTATION_TOP_LEFT;
extern uint8_t ORIENTATION_RIGHT_TOP;
extern uint8_t ORIENTATION_BOTTOM_RIGHT;
extern uint8_t ORIENTATION_LEFT_BOTTOM;

struct mosaik2_database_struct {
	char thumbs_db_name[256];
	char imagestddev_filename[256];
	char imagecolors_filename[256];
	char imagedims_filename[256];
	char filenames_filename[256];
	char filenames_index_filename[256];
	char filehashes_filename[256];
	char timestamps_filename[256];
	char filesizes_filename[256];
	char tiledims_filename[256];
	char invalid_filename[256];
	char duplicates_filename[256];
	char temporary_duplicates_filename[256];
	char tilecount_filename[256];
};// = {NULL, "imagestddev.bin", "imagecolors.bin", "imagedims.bin", "filenames.txt", "filehashes.bin", "filesizes.bin", "tiledims.bin", "invalid.bin", "tilecount.conf"};

struct mosaik2_project_struct {
	char dest_filename[256];
	char dest_mastertiledims_filename[256];
	char dest_result_filename[256];
	uint8_t ratio;
	uint8_t unique;
	uint32_t file_size;
	uint8_t master_tile_count;
	char dest_html_filename[ 256 ]; 
	char dest_src_filename[ 256 ];
	struct mosaik2_database_struct *mds;
	uint8_t mds_len;
};

struct result {
	uint32_t sortorder;
	char *thumbs_db_name;
	uint8_t hash[16];
	uint64_t index; // index in thumbs_db
	uint64_t score;
	uint8_t off_x;
	uint8_t off_y;
	char thumbs_db_filenames[MAX_FILENAME_LEN];
	char temp_filename[MAX_TEMP_FILENAME_LEN];
	int size;
};

 void init_mosaik2_database_struct(struct mosaik2_database_struct *md, char *thumbs_db_name);
 void init_mosaik2_project_struct(struct mosaik2_project_struct *mp, char *thumbs_db_name, char *dest_filename);
 int EndsWith(const char *str, const char *suffix);
 int StartsWith(const char *pre, const char *str);
 int is_file_local( const char *filename );
 int is_file_wikimedia_commons( const char *filename );
 void get_wikimedia_thumb_url(const char *url, char *thumb_pixel, char *dest, int dest_len);
 void get_wikimedia_file_url(const char *url, char *dest, int dest_len);
 off_t get_file_size(const char *filename);

 int get_file_type(const char *dest_filename);
 int get_file_type_from_buf(uint8_t *buf, size_t len);
 uint64_t read_thumbs_db_count(struct mosaik2_database_struct *md);
 uint8_t read_thumbs_conf_tilecount(struct mosaik2_database_struct *md);
 void check_thumbs_db_name(char *thumbs_db_name);
 void check_thumbs_db(struct mosaik2_database_struct *md);
 int check_dest_filename(char *dest_filename);
 void check_thumbs_tile_count(uint32_t thumbs_tile_count);
 void remove_newline(char *str);

 int cmpfunc (const void * a, const void * b);
 int cmpfunc_back(const void *a, const void *b);

//for curl writing
 static size_t write_data(void *ptr, size_t size, size_t nmemb, void *stream);
 int File_Copy(char FileSource[], char FileDestination[]);



 uint8_t get_image_orientation(unsigned char *buffer, size_t buf_size);
 gdImagePtr myLoadPng(char *filename, char *origin_name);
 static void trim_spaces(char *buf);
 static void show_tag(ExifData *d, ExifIfd ifd, ExifTag tag);
 static void show_mnote_tag(ExifData *d, unsigned tag);

 gdImagePtr gdImageRotate90 (gdImagePtr src);
 gdImagePtr gdImageRotate180 (gdImagePtr src);
 gdImagePtr gdImageRotate270 (gdImagePtr src);

#endif
