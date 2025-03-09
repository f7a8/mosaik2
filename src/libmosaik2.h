
#ifndef _LIBMOSAIK2_H_
#define _LIBMOSAIK2_H_

#define _GNU_SOURCE // for activating memmem in string.h
#include <assert.h>
#include <errno.h>
#include <err.h>
#include <float.h>
#include <gd.h>
#include <inttypes.h>
#include <libgen.h>
#include <limits.h>
#include <math.h>
#include <openssl/evp.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/file.h>
#include <sys/mman.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <sys/sysinfo.h> // for meminfo external sort in duplicates
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#define MD5_DIGEST_LENGTH 16

#ifdef HAVE_CURL
#include <curl/curl.h>
#endif

#ifdef HAVE_EXIF
#include <libexif/exif-data.h>
#endif

#include "data_types.h"
#include "mosaik2.h"

#define MAX_FILENAME_LEN 1024
#define MAX_TEMP_FILENAME_LEN 100

#define ELEMENT_NUMBER 1
#define ELEMENT_FILENAME 2

#define R 0
#define G 1
#define B 2
#define RGB 3

#define INVALID_NONE 0
#define INVALID_AUTOMATIC 1
#define INVALID_MANUAL 2

#define DUPLICATE_NONE 0
#define DUPLICATE_MD5 1
#define DUPLICATE_PHASH 2

#define TILEOFFSET_UNSET 0xFF
#define TILEOFFSET_INVALID 0


extern const int FT_JPEG;
extern const int FT_PNG;
extern const int FT_ERR;

extern uint8_t ORIENTATION_TOP_LEFT;
extern uint8_t ORIENTATION_RIGHT_TOP;
extern uint8_t ORIENTATION_BOTTOM_RIGHT;
extern uint8_t ORIENTATION_LEFT_BOTTOM;

#ifdef HAVE_PHASH

extern const int PHASHES_VALID;
extern const int PHASHES_INVALID;
extern const int IS_PHASH_DUPLICATE;

extern int ph_dct_imagehash(const char *file, unsigned long long *hash);
#endif

extern const int IS_DUPLICATE;
extern const int IS_NO_DUPLICATE;

void mosaik2_context_init(mosaik2_context *ctx);
void mosaik2_database_check(mosaik2_database *md);
//void mosaik2_database_check_name(char *mosaik2_database_name);
float mosaik2_database_costs(mosaik2_database *md, mosaik2_database_element *mde);
int mosaik2_database_find_element_number(mosaik2_database *md, m2name filename, m2elem *found_element_number);
void mosaik2_database_init(mosaik2_database *md, m2name mosaik2_database_name);
#ifdef HAVE_PHASH
void mosaik2_database_phashes_build(mosaik2_database *md);
int  mosaik2_database_phashes_check(mosaik2_database *md);
#endif
void mosaik2_database_read_database_id(mosaik2_database *md);
void mosaik2_database_read_element(mosaik2_database *md, mosaik2_database_element *mde, m2elem element_number);
m2elem mosaik2_database_read_element_count(mosaik2_database *md);
m2name mosaik2_database_read_element_filename(mosaik2_database *md, int element_number, m2file filenames_index_file);
uint8_t mosaik2_database_read_image_resolution(mosaik2_database *md);
m2elem mosaik2_database_read_duplicates_count(mosaik2_database *md);
m2elem mosaik2_database_read_invalid_count(mosaik2_database *md);
m2elem mosaik2_database_read_valid_count(mosaik2_database *md);
m2elem mosaik2_database_read_tileoffset_count(mosaik2_database *md);
time_t mosaik2_database_read_createdat(mosaik2_database *md);
time_t mosaik2_database_read_lastindexed(mosaik2_database *md);
time_t mosaik2_database_read_lastmodified(mosaik2_database *md);
size_t mosaik2_database_read_size(mosaik2_database *md);
void mosaik2_database_read_histogram(mosaik2_database *md);
int mosaik2_indextask_read_image(mosaik2_indextask *);
void mosaik2_project_check(mosaik2_project *mp);
int  mosaik2_project_check_dest_filename(m2name dest_filename);
void mosaik2_project_init(mosaik2_project *mp, m2ctext mosaik2_database_name, m2name dest_filename);
void mosaik2_project_read_image_dims(mosaik2_project *mp);
void mosaik2_project_read_primary_tile_dims(mosaik2_project *mp);
mosaik2_project_result *mosaik2_project_read_result(mosaik2_project *mp, mosaik2_database *md, int total_primary_tile_count);
void mosaik2_tile_infos_init(mosaik2_tile_infos *ti, int database_image_resolution, int src_image_resolution, int image_width, int image_height);
void mosaik2_tile_image( mosaik2_tile_infos *ti, gdImagePtr *im, double *colors, double *stddev);
void mosaik2_tiler_infos_init(mosaik2_tile_infos *ti, int database_image_resolution, int image_width, int image_height);

unsigned char* read_stdin(size_t *);

int EndsWith(const char *str, const char *suffix);
int StartsWith(const char *pre, const char *str);
int is_file ( const char* pathname );
int is_file_local( const m2name filename );
int is_file_wikimedia_commons( const m2name filename );
void get_wikimedia_thumb_url(const char *url, char *thumb_pixel, char *dest, int dest_len);
void get_wikimedia_file_url(const char *url, char *dest, int dest_len);
/**
 * check via inode equalness
 */
int is_same_file(const m2name filename0, const m2name filename1);
off_t get_file_size(const m2name filename);

int get_file_type(const m2name dest_filename);
int get_file_type_from_buf(uint8_t *buf, size_t len);

//void check_thumbs_tile_count(uint32_t thumbs_tile_count);
void check_resolution(uint32_t resolution);
void remove_newline(char *str);

int cmpfunc (const void * a, const void * b);
int cmpfunc_back(const void *a, const void *b);

//for curl writing
int File_Copy(char FileSource[], char FileDestination[]);



uint8_t get_image_orientation(unsigned char *buffer, size_t buf_size);
gdImagePtr read_image_from_file(m2name filename);
gdImagePtr read_image_from_buf(unsigned char *buf, size_t file_size);
void trim_spaces(char *buf);
//void show_tag(ExifData *d, ExifIfd ifd, ExifTag tag);
//void show_mnote_tag(ExifData *d, unsigned tag);

gdImagePtr gdImageRotate90 (gdImagePtr src);
gdImagePtr gdImageRotate180 (gdImagePtr src);
gdImagePtr gdImageRotate270 (gdImagePtr src);

#endif

//void print_usage(char *);

void read_entry(m2name filename, void *val, size_t len, off_t offset); //read single values from a single database file.
void write_entry(m2name filename, void *val, size_t len, off_t offset); //read single values from a single database file.
void read_file_entry(m2file file, void *val, size_t len, off_t offset); 
void write_file_entry(m2file file, void *val, size_t len, off_t offset);

m2file m_fopen(m2name filename, char *mode); // wrapper fpr fopen
void m_fclose(m2file file); //wrapper for flose

int m_open(const char *pathname, int flags, mode_t mode);
void m_close(int fd);

m2rtext m_fgets(m2rtext s, int size, m2file stream);
void m_fread(void *ptr, size_t nmemb, m2file stream);
void m_fwrite(const void *ptr, size_t nmemb, m2file stream);
void *m_malloc(size_t size);
void *m_calloc(size_t nmemb, size_t size);
int m_fseeko(m2file stream, off_t offset, int whence);
int m_fflush(m2file stream);
void m_stat(const char *pathname, struct stat *statbuf);
void m_fstat(int fd, struct stat *statbuf);
void *m_mmap(void *addr, size_t length, int prot, int flags, int fd, off_t offset);
void m_munmap(void *addr, size_t length);
m2file m_tmpfile(void);


int m_sysinfo(struct sysinfo *info);
void m_access(m2ctext pathname, int mode);


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
char *get_file_name(m2file file);
char *get_fd_name(int fd);
