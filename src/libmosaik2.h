
#ifndef _LIBMOSAIK2_H_
#define _LIBMOSAIK2_H_

#include <curl/curl.h>
#include <errno.h>
#include <err.h>
#include <inttypes.h>
#include <libgen.h>
#include <limits.h>
#include <math.h>
#include <openssl/md5.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/file.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <sys/sysinfo.h> // for meminfo external sort in duplicates
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>



#include <gd.h>
#include <libexif/exif-data.h>

#include "mosaik2.h"

#define MAX_FILENAME_LEN 1024
#define MAX_TEMP_FILENAME_LEN 100

extern const int FT_JPEG;
extern const int FT_PNG;
extern const int FT_ERR;

extern uint8_t ORIENTATION_TOP_LEFT;
extern uint8_t ORIENTATION_RIGHT_TOP;
extern uint8_t ORIENTATION_BOTTOM_RIGHT;
extern uint8_t ORIENTATION_LEFT_BOTTOM;

void init_mosaik2_context(mosaik2_context *);
void init_mosaik2_database(mosaik2_database *md, char *thumbs_db_name);
void init_mosaik2_project(mosaik2_project *mp, char *thumbs_db_name, char *dest_filename);

int mosaik2_indextask_read_image(mosaik2_indextask *);

int EndsWith(const char *str, const char *suffix);
int StartsWith(const char *pre, const char *str);
int is_file_local( const char *filename );
int is_file_wikimedia_commons( const char *filename );
void get_wikimedia_thumb_url(const char *url, char *thumb_pixel, char *dest, int dest_len);
void get_wikimedia_file_url(const char *url, char *dest, int dest_len);
off_t get_file_size(const char *filename);

int get_file_type(const char *dest_filename);
int get_file_type_from_buf(uint8_t *buf, size_t len);
uint64_t read_thumbs_db_count(mosaik2_database *md);
uint8_t read_thumbs_conf_tilecount(mosaik2_database *md);
void read_database_id(mosaik2_database *md);
void check_thumbs_db_name(char *thumbs_db_name);
void check_thumbs_db(mosaik2_database *md);
int check_dest_filename(char *dest_filename);
void check_thumbs_tile_count(uint32_t thumbs_tile_count);
void check_resolution(uint32_t resolution);
void remove_newline(char *str);

int cmpfunc (const void * a, const void * b);
int cmpfunc_back(const void *a, const void *b);

//for curl writing
int File_Copy(char FileSource[], char FileDestination[]);



uint8_t get_image_orientation(unsigned char *buffer, size_t buf_size);
gdImagePtr myLoadPng(char *filename, char *origin_name);
void trim_spaces(char *buf);
void show_tag(ExifData *d, ExifIfd ifd, ExifTag tag);
void show_mnote_tag(ExifData *d, unsigned tag);

gdImagePtr gdImageRotate90 (gdImagePtr src);
gdImagePtr gdImageRotate180 (gdImagePtr src);
gdImagePtr gdImageRotate270 (gdImagePtr src);

#endif

void print_usage(char *);
