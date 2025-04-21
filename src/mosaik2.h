
#ifndef _MOSAIK2_H_
#define _MOSAIK2_H_

#define MAX_FILENAME_LEN 1024
#define MAX_TEMP_FILENAME_LEN 100

#define MOSAIK2_DATABASE_FORMAT_VERSION 7
#define MOSAIK2_VERSION "0.31"

extern const int FT_JPEG;
extern const int FT_PNG;
extern const int FT_ERR;

extern m2orient ORIENTATION_TOP_LEFT;
extern m2orient ORIENTATION_RIGHT_TOP;
extern m2orient ORIENTATION_BOTTOM_RIGHT;
extern m2orient ORIENTATION_LEFT_BOTTOM;

static uint32_t const MOSAIK2_CONTEXT_MAX_TILER_PROCESSES = 1024;

static int const MOSAIK2_ARGS_COLOR_DISTANCE_MANHATTAN = 1;
static int const MOSAIK2_ARGS_COLOR_DISTANCE_EUCLIDIAN = 2;
static int const MOSAIK2_ARGS_COLOR_DISTANCE_CHEBYSHEV = 3;
static int const MOSAIK2_ARGS_COLOR_DISTANCE_DEFAULT = 1;

struct mosaik2_context_struct {
	int debug;
	int debug1;
	int html;
	int out;

	uint8_t exiting;
	uint32_t max_tiler_processes;
	double max_load_avg;
	
	uint32_t current_tiler_processes;
	pid_t pids[1024];

	time_t start_t;

	// starts every index process at 0, important for image per second info in index mode
	uint32_t new_indexed_element;
	
};

typedef struct mosaik2_context_struct mosaik2_context;

struct mosaik2_tile_infos_struct {

	uint32_t image_width;
	uint32_t image_height;
	m2rezo src_image_resolution; // -t count of mosaic elements at the shorter src-image side
	m2rezo database_image_resolution;

	uint32_t short_dim;
	uint32_t pixel_per_tile;
	float total_pixel_per_tile;
	uint32_t total_pixel_per_primary_tile;
	uint32_t pixel_per_primary_tile;
	uint32_t total_pixel_count; // only recognized pixel count
	uint32_t ignored_pixel_count; // none recognized pixels
	uint32_t tile_count; 
	uint32_t tile_x_count;
	uint32_t tile_y_count;
	uint32_t primary_tile_count;
	uint32_t primary_tile_x_count;
	uint32_t primary_tile_y_count;
	uint32_t total_tile_count;
	uint32_t total_primary_tile_count;
	uint32_t offset_x, offset_y; // pixel offset to cut a maybe margin
	uint32_t lx, ly; // absolute pixel length where the regognized image area ends
	/*
	 * initial values
	 * database_image_resolution 3
	 * src_image_resolution 2
	 * image_width 19_init
	 * image_height 6
	 
	 * are controlling this values

	 primary_tile_count 2
	 primary_tile_x_count 5
	 primary_tile_y_count 2
	 tile_count 6
	 tile_x_count

	   ------------------- <- 0                 pixel_per_tile 3
	   | ++#++#++#++#++# | <- offset_y 1        total_pixel_per_tile 9
	   | ++#++#++#++#++# |                      
	   | ############### |   +# total_pixel_count 14*5=70
	   | ++#++#++#++#++# |
	   | ++#++#++#++#++# | 
	   | ############### | <- ly 6
	   ------------------- <- image_height 8 
	   ^ ^             ^ ^
	   | |             | |
	   0 offset_x     lx image_width 
	     2            16 19

	*/
};
typedef struct mosaik2_tile_infos_struct mosaik2_tile_infos;

typedef enum {  INDEXTASK_STATE_INITIAL, INDEXTASK_STATE_LOADING, INDEXTASK_STATE_INDEXING, INDEXTASK_STATE_WRITING_INDEX, INDEXTASK_STATE_ENDING } TASK_STATE;

struct mosaik2_indextask_struct {
	time_t start;
	time_t end;
	TASK_STATE state;
	uint32_t idx;
	char filename[MAX_FILENAME_LEN]; // trade off, dont want to malloc that much. hoping it fits
	m2file file;
	size_t filesize;
	time_t lastmodified;
	time_t lastindexed;
	uint8_t tile_count;
	void *image_data;

	int imagedims[2];
	uint8_t tiledims[2];
	unsigned char hash[MD5_DIGEST_LENGTH];

	uint32_t total_tile_count;	
	uint8_t *colors;
};

typedef struct mosaik2_indextask_struct mosaik2_indextask;

struct mosaik2_database_struct {
	char thumbs_db_name[256];
	char imagecolors_filename[256];
	char imagedims_filename[256];
	char image_index_filename[256];
	char filenames_filename[256];
	char filenames_index_filename[256];
	char filehashes_filename[256];
	char filehashes_index_filename[256];
	char timestamps_filename[256];
	char filesizes_filename[256];
	char tiledims_filename[256];
	char invalid_filename[256];
	char duplicates_filename[256];
	char database_image_resolution_filename[256];
	char id_filename[256];
	char id[17];
	size_t id_len;
	char version_filename[256];
	char readme_filename[256];
	char pid_filename[256];
	char lock_index_filename[256]; // relevant to synchronize in indexing mode
	char lock_database_filename[256]; // for data integrity, to prevent write and read operations simultaneously
	char tileoffsets_filename[256];
	char lastmodified_filename[256];
	char lastindexed_filename[256];
	char createdat_filename[256];
	char phash_filename[256];

	int imagecolors_sizeof;
	int imagedims_sizeof;
	int image_index_sizeof;
	int filenames_index_sizeof;
	int filehashes_sizeof;
	int filehashes_index_sizeof;
	int timestamps_sizeof;
	int filesizes_sizeof;
	int tiledims_sizeof;
	int invalid_sizeof;
	int duplicates_sizeof;
	int databaseimageresolution_sizeof;
	int tileoffsets_sizeof;
	int lastmodified_sizeof;
	int lastindexed_sizeof;
	int createdat_sizeof;
	int phash_sizeof;

	m2rezo database_image_resolution;
	float histogram_color[3]; // all valid entries
	m2elem element_count;
	m2elem valid_element_count;
	m2elem invalid_element_count;
	m2elem duplicates_element_count;
	m2elem tileoffset_element_count;
	m2size db_size;
	m2time createdat;
	m2time lastmodified;
	m2time lastindexed;
};
typedef struct mosaik2_database_struct mosaik2_database;

struct mosaik2_database_element_struct {
	mosaik2_database *md;
	m2elem element_number;
	unsigned char hash[MD5_DIGEST_LENGTH];
	#ifdef HAVE_PHASH
	int has_phash;
	m2phash phash;
	#endif
	m2name filename;
	size_t filesize;
	uint32_t imagedims[2];
	unsigned char tiledims[2];
	time_t timestamp;
	unsigned char duplicate;
	unsigned char invalid;
	unsigned char tileoffsets[2];
	float histogram_color[3];
};
typedef struct mosaik2_database_element_struct mosaik2_database_element;

struct mosaik2_project_struct {
	char dest_filename[256];
	//char dest_primarytiledims_filename[256];
	char dest_result_filename[256];
	char src_filename[MAX_FILENAME_LEN+1];
	uint8_t unique;
	uint8_t fast_unique;
	size_t file_size;
	m2rezo primary_tile_count; 
	char dest_html_filename[256]; 
	char dest_html2_filename[256];
	char dest_src_filename[256];
	char dest_tile_infos_filename[256];
	mosaik2_database *mds;
	uint8_t mds_len;
	int image_height; // populated by read_image_dim( *mosaik2_project)
	int image_width;  // populated by read_image_dim( *mosaik2_project)
	int pixel_per_tile;
	uint8_t primary_tile_x_count; //populated by mosaik2_project_read_primary_tile_dims( *mosaik2_project)
	uint8_t primary_tile_y_count; //populated by mosaik2_project_read_primary_tile_dims( *mosaik2_project)
	m2area *exclude_area;
	m2elem exclude_count; // area elements
};
typedef struct mosaik2_project_struct mosaik2_project;

struct mosaik2_project_result_struct {
	uint32_t sortorder;
	mosaik2_database *md;
	m2hash hash;
	m2elem index; // index in thumbs_db
	float costs;
	uint8_t off_x;
	uint8_t off_y;
	__attribute__((deprecated)) \
	char temp_filename[MAX_TEMP_FILENAME_LEN]; // DEPRECATED
	char cache_filename[MAX_TEMP_FILENAME_LEN];
	char origin_filename[MAX_FILENAME_LEN];
	int is_file_local;
	int size;
	int exclude;
};
typedef struct mosaik2_project_result_struct    mosaik2_project_result;

typedef struct mosaik2_database_candidate_struct {
	uint32_t primary_tile_idx;
	uint32_t candidate_idx;
	float costs;
	uint8_t off_x;
	uint8_t off_y;
} mosaik2_database_candidate;


/* Used by main to communicate with parse_opt. */
struct arguments_struct {
  char *mode;
	m2name mosaik2db;
	m2name *mosaik2dbs;
	int mosaik2dbs_count;
	char *dest_image;
  int verbose;
	int dry_run;
	int32_t database_image_resolution;
	m2rezo src_image_resolution; // src_image_resolution
	int has_src_image_resolution;
	int max_load, max_jobs;
	int unique;
	int fast_unique;
	int pixel_per_tile;
	int duplicate_reduction;
	int symlink_cache;
	char *cache_path;
	int ignore_old_invalids;
	int no_hash_cmp;
	int color_distance;
	m2elem element_number;
	int has_element_identifier;
	char *src_image;
	char *index_filelist;
	int quiet;
	m2name element_filename;
	int phash_distance;
	int has_phash_distance;
	char **exclude_area;
	int exclude_count; // elements of exclude_area options
};
typedef struct arguments_struct mosaik2_arguments;

int mosaik2_init(mosaik2_arguments*);
int mosaik2_index(mosaik2_arguments*);
int mosaik2_gathering(mosaik2_arguments*);
int mosaik2_join(mosaik2_arguments*);
int mosaik2_invalid(mosaik2_arguments*);
int mosaik2_duplicates(mosaik2_arguments*);
int mosaik2_info(mosaik2_arguments*);
int mosaik2_crop(mosaik2_arguments*);

int mosaik2_clean(char *mosaik2_database_name);
int mosaik2_tiler(mosaik2_arguments *, mosaik2_database *, mosaik2_indextask *);
void mosaik2_index_write_to_disk(mosaik2_database *, mosaik2_indextask *);


#endif
