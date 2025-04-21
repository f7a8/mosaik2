
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

#ifdef HAVE_LIBCURL
#include <curl/curl.h>
#endif

#ifdef HAVE_LIBEXIF
#include <libexif/exif-data.h>
#endif

#ifdef HAVE_AVX
#include <immintrin.h>
#endif

#ifdef M2DBG
#define M2DEBUG 1
#else
#define M2DEBUG 0
#endif

#include "data_types.h"
#include "mosaik2.h"

#define MAX_FILENAME_LEN 1024
#define FILENAME_LEN 256
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

extern m2orient ORIENTATION_TOP_LEFT;
extern m2orient ORIENTATION_RIGHT_TOP;
extern m2orient ORIENTATION_BOTTOM_RIGHT;
extern m2orient ORIENTATION_LEFT_BOTTOM;

#ifdef HAVE_PHASH

extern const int PHASHES_INVALID;
extern const int PHASHES_VALID;
extern const int IS_PHASH_DUPLICATE;
extern const int HAS_PHASH;
extern const int HAS_NO_PHASH;

extern int ph_dct_imagehash(const char *file, unsigned long long *hash);

#endif

extern const int IS_DUPLICATE;
extern const int IS_NO_DUPLICATE;

void mosaik2_context_init(mosaik2_context *ctx);
void mosaik2_create_cache_dir();
void mosaik2_database_check(mosaik2_database *md);
void mosaik2_database_lock_writer(mosaik2_database *md);// os exclusive filelock on file in db dir
void mosaik2_database_lock_reader(mosaik2_database *md);// os shared filelock on file in db dir

float mosaik2_database_costs(mosaik2_database *md, mosaik2_database_element *mde);
int mosaik2_database_find_element_number(mosaik2_database *md, m2name filename, m2elem *found_element_number);
void mosaik2_database_init(mosaik2_database *md, m2name mosaik2_database_name);
#ifdef HAVE_PHASH
void mosaik2_database_phashes_build(mosaik2_database *md, mosaik2_arguments *args);
int  mosaik2_database_phashes_check(mosaik2_database *md);
#endif
void mosaik2_database_read_database_id(mosaik2_database *md);
void mosaik2_database_read_element(mosaik2_database *md, mosaik2_database_element *mde, m2elem element_number);
m2elem mosaik2_database_read_element_count(mosaik2_database *md);
m2name mosaik2_database_read_element_filename(mosaik2_database *md, m2elem element_number, m2file filenames_index_file, m2file filenames_file);
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
void mosaik2_database_touch_lastmodified(mosaik2_database *md);
int mosaik2_indextask_read_image(/*mosaik2_database *md, */mosaik2_indextask *);
void mosaik2_project_check(mosaik2_project *mp);
m2ftype  mosaik2_project_check_dest_filename(m2name dest_filename);
void mosaik2_project_init(mosaik2_project *mp, m2text mosaik2_database_id, m2name dest_filename);
void mosaik2_project_read_primary_tile_dims(mosaik2_project *mp);
void mosaik2_project_read_exclude_area(mosaik2_project *mp, mosaik2_tile_infos *ti, mosaik2_arguments *args);
mosaik2_project_result *mosaik2_project_read_result(mosaik2_project *mp, mosaik2_database *md, int total_primary_tile_count);
void mosaik2_tile_infos_init(mosaik2_tile_infos *ti, m2rezo database_image_resolution, m2rezo src_image_resolution, uint32_t image_width, uint32_t image_height);
void mosaik2_tiler_infos_init(mosaik2_tile_infos *ti, m2rezo database_image_resolution, uint32_t image_width, uint32_t image_height);

unsigned char* read_stdin(size_t *);


int EndsWith(const char *str, const char *suffix);
int StartsWith(const char *pre, const char *str);
#define concat(buf, ...) \
               _Static_assert(!__builtin_types_compatible_p(__typeof__(buf), char *), \
               "concat() only allow with arrays, use concatn instead?"); \
               _concat(buf, sizeof(buf), __VA_ARGS__, NULL)
#define concatn(buf, len, ...) \
               _concat(buf, len, __VA_ARGS__, NULL)

void _concat(char *dest, size_t len, ...);
int is_dest_file ( const char* pathname);
int is_src_file ( const char* pathname );
int is_file ( const char* pathname );
int file_exists(const char *pathname);
int dir_exists(const char *pathname);
int file_exists(const char *pathname);

int is_file_local( const m2name filename );
//int is_file_wikimedia_commons( const m2name filename );
//void get_wikimedia_thumb_url(const char *url, char *thumb_pixel, char *dest, int dest_len);
//void get_wikimedia_file_url(const char *url, char *dest, int dest_len);
/**
 * check via inode equalness
 */
int is_same_file(const m2name filename0, const m2name filename1);
off_t get_file_size(const m2name filename);

m2ftype get_file_type(const char *dest_filename);
m2ftype get_file_type_from_buf(uint8_t *buf, size_t len);

//void check_thumbs_tile_count(uint32_t thumbs_tile_count);
void check_resolution(int32_t resolution);
void check_mosaik2database_name(char *name);
void remove_newline(char *str);

int cmpfunc (const void * a, const void * b);
int cmpfunc_back(const void *a, const void *b);

//for curl writing
int File_Copy(char FileSource[], char FileDestination[]);

void u8_to_f32(const uint8_t* src, float* dst, size_t count);

uint8_t get_image_orientation(void *buf, size_t buf_size);
gdImagePtr read_image_from_file(m2name filename);
gdImagePtr read_image_from_buf(void *buf, size_t file_size);
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

m2fd m_open(const char *pathname, int flags, mode_t mode);
void m_close(m2fd fd);

m2text m_fgets(m2rtext s, int size, m2file stream);
void m_fread(void *ptr, size_t nmemb, m2file stream);
void m_fwrite(const void *ptr, size_t nmemb, m2file stream);
void *m_malloc(size_t size);
void *m_calloc(size_t nmemb, size_t size);
void m_free(void **ptr);
int m_fseeko(m2file stream, off_t offset, int whence);
m2off m_ftello(FILE *stream);
int m_fflush(m2file stream);
void m_stat(const char *pathname, struct stat *statbuf);
void m_fstat(m2fd fd, struct stat *statbuf);
void *m_mmap(void *addr, size_t length, int prot, int flags, int fd, off_t offset);
void m_munmap(void *addr, size_t length);
m2file m_tmpfile(void);
void m_flock(m2fd fd, int operation);


int m_sysinfo(struct sysinfo *info);
void m_access(const char* pathname, int mode);

m2fd m_fileno(m2file file);
char *get_file_name(m2file file);//ptr has to be freed
char *get_fd_name(m2fd fd);//ptr has to be freed
void quote_string(const char* input, int output_len, char* output);
void unset_file_buf(FILE *stream);
int m_setvbuf(FILE *stream, char *buf, int mode, size_t size);
//void m_exit(int status);


/* Max-Heap and Min-Heap from https://de.wikibmooks.org/wiki/Algorithmen_und_Datenstrukturen_in_C/_Heaps under CC BY-SA 3.0 */
/* Adaption: Element types are changed from int to mosaik2_database_candidate -------------------------------------------- */
typedef struct {
   uint32_t last;
   uint32_t count;
   mosaik2_database_candidate* keys;
} Heap;
void swap(Heap* h, uint32_t n, uint32_t m);
void heap_init(Heap* h, mosaik2_database_candidate* storage);
void heap_dump(Heap *h);
void mdc_dump(mosaik2_database_candidate *mdc0);
void max_heap_bubble_up(Heap* h, uint32_t n);
void max_heap_insert(Heap* h, mosaik2_database_candidate *key);
void max_heap_sift_down(Heap* h, uint32_t n);
int max_heap_delete(Heap* h, uint32_t n, mosaik2_database_candidate *key);
int max_heap_pop(Heap* h, mosaik2_database_candidate *key);
int max_heap_peek(Heap *h, mosaik2_database_candidate *key);
void min_heap_bubble_up(Heap* h, uint32_t n);
void min_heap_insert(Heap* h, mosaik2_database_candidate *key);
void min_heap_sift_down(Heap* h, uint32_t n);
int min_heap_delete(Heap* h, uint32_t n, mosaik2_database_candidate *key);
int min_heap_pop(Heap* h, mosaik2_database_candidate *k);
int min_heap_peek(Heap *h, mosaik2_database_candidate *k);
/* END: Max-Heap and Min-Heap from https://de.wikibooks.org/wiki/Algorithmen_und_Datenstrukturen_in_C/_Heaps under CC BY-SA 3.0 */

