
#ifndef _LIBMOSAIK2_H_
#define _LIBMOSAIK2_H_

#include <assert.h>
#include <curl/curl.h>
#include <errno.h>
#include <err.h>
#include <float.h>
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

#define R 0
#define G 1
#define B 2
#define RGB 3

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
void mosaik2_tile_infos_init(mosaik2_tile_infos *ti, int database_image_resolution, int src_image_resolution, int image_width, int image_height);
void mosaik2_tiler_infos_init(mosaik2_tile_infos *ti, int database_image_resolution, int image_width, int image_height);
void mosaik2_database_read_database_id(mosaik2_database *md);
void mosaik2_database_read_element(mosaik2_database *md, mosaik2_database_element *mde, uint32_t element_number);
char *mosaik2_database_read_element_filename(mosaik2_database *md, int element_number, FILE *filenames_index_file);
void mosaik2_project_read_primary_tile_dims(mosaik2_project *mp);
mosaik2_project_result *mosaik2_project_read_result(mosaik2_project *mp, mosaik2_database *md, int total_primary_tile_count);

int mosaik2_indextask_read_image(mosaik2_indextask *);
void mosaik2_tile_image( mosaik2_tile_infos *ti, gdImagePtr *im, double *colors, double *stddev);
unsigned char* read_stdin(size_t *);
void mosaik2_project_read_image_dims(mosaik2_project *mp);

int EndsWith(const char *str, const char *suffix);
int StartsWith(const char *pre, const char *str);
int is_file_local( const char *filename );
int is_file_wikimedia_commons( const char *filename );
void get_wikimedia_thumb_url(const char *url, char *thumb_pixel, char *dest, int dest_len);
void get_wikimedia_file_url(const char *url, char *dest, int dest_len);
/**
 * check via inode equalness
 */
int is_same_file(const char *filename0, const char *filename1);
off_t get_file_size(const char *filename);

int get_file_type(const char *dest_filename);
int get_file_type_from_buf(uint8_t *buf, size_t len);
uint32_t read_thumbs_db_count(mosaik2_database *md);
uint8_t read_database_image_resolution(mosaik2_database *md);
uint32_t read_thumbs_db_duplicates_count(mosaik2_database *md);
uint32_t read_thumbs_db_invalid_count(mosaik2_database *md);
uint32_t read_thumbs_db_valid_count(mosaik2_database *md);
uint32_t read_thumbs_db_tileoffset_count(mosaik2_database *md);
time_t read_thumbs_db_createdat(mosaik2_database *md);
time_t read_thumbs_db_lastindexed(mosaik2_database *md);
time_t read_thumbs_db_lastmodified(mosaik2_database *md);

size_t read_thumbs_db_size(mosaik2_database *md);
void read_thumbs_db_histogram(mosaik2_database *md);

void check_thumbs_db_name(char *thumbs_db_name);
void check_thumbs_db(mosaik2_database *md);
void mosaik2_project_check(mosaik2_project *mp);
int check_dest_filename(char *dest_filename);
void check_thumbs_tile_count(uint32_t thumbs_tile_count);
void check_resolution(uint32_t resolution);
void remove_newline(char *str);

int cmpfunc (const void * a, const void * b);
int cmpfunc_back(const void *a, const void *b);

//for curl writing
int File_Copy(char FileSource[], char FileDestination[]);



uint8_t get_image_orientation(unsigned char *buffer, size_t buf_size);
gdImagePtr read_image_from_file(char *filename);
gdImagePtr read_image_from_buf(unsigned char *buf, size_t file_size);
void trim_spaces(char *buf);
void show_tag(ExifData *d, ExifIfd ifd, ExifTag tag);
void show_mnote_tag(ExifData *d, unsigned tag);

gdImagePtr gdImageRotate90 (gdImagePtr src);
gdImagePtr gdImageRotate180 (gdImagePtr src);
gdImagePtr gdImageRotate270 (gdImagePtr src);

#endif

//void print_usage(char *);

void read_entry(char *filename, void *val, size_t len, off_t offset); //read single values from a single database file. 
void write_entry(char *filename, void *val, size_t len, off_t offset); //read single values from a single database file. 

FILE *m_fopen(char *filename, char *mode); // wrapper fpr fopen
void m_fclose(FILE *file); //wrapper for flose
void m_fread(void *ptr, size_t nmemb, FILE *stream);
void m_fwrite(const void *ptr, size_t nmemb, FILE *stream);
void *m_malloc(size_t size);
void *m_calloc(size_t nmemb, size_t size);
int m_fseeko(FILE *stream, off_t offset, int whence);
int m_fflush(FILE *stream);
int m_stat(const char *pathname, struct stat *statbuf);
int m_sysinfo(struct sysinfo *info);
void m_access(const char *pathname, int mode);



/* Max-Heap and Min-Heap from https://de.wikibooks.org/wiki/Algorithmen_und_Datenstrukturen_in_C/_Heaps under CC BY-SA 3.0 */
/* Adaption: Element types are changed from int to mosaik2_database_candidate -------------------------------------------- */
typedef struct {
   int last;
   uint32_t count;
   mosaik2_database_candidate* keys;
} Heap;
void swap(Heap* h, int n, int m);
void heap_init(Heap* h, mosaik2_database_candidate* storage);
void heap_dump(Heap *h);
void mdc_dump(mosaik2_database_candidate *mdc0);
void max_heap_bubble_up(Heap* h, int n);
void max_heap_insert(Heap* h, mosaik2_database_candidate *key);
void max_heap_sift_down(Heap* h, int n);
int max_heap_delete(Heap* h, int n, mosaik2_database_candidate *key);
int max_heap_pop(Heap* h, mosaik2_database_candidate *key);
int max_heap_peek(Heap *h, mosaik2_database_candidate *key);
void min_heap_bubble_up(Heap* h, int n);
void min_heap_insert(Heap* h, mosaik2_database_candidate *key);
void min_heap_sift_down(Heap* h, int n);
int min_heap_delete(Heap* h, int n, mosaik2_database_candidate *key);
int min_heap_pop(Heap* h, mosaik2_database_candidate *k);
int min_heap_peek(Heap *h, mosaik2_database_candidate *k);
/* END: Max-Heap and Min-Heap from https://de.wikibooks.org/wiki/Algorithmen_und_Datenstrukturen_in_C/_Heaps under CC BY-SA 3.0 */

