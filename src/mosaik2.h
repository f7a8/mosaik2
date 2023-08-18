
#ifndef _MOSAIK2_H_
#define _MOSAIK2_H_

#define MAX_FILENAME_LEN 1024
#define MAX_TEMP_FILENAME_LEN 100

#define MOSAIK2_DATABASE_FORMAT_VERSION 6
#define MOSAIK2_VERSION "0.3"

extern const int FT_JPEG;
extern const int FT_PNG;
extern const int FT_ERR;

extern uint8_t ORIENTATION_TOP_LEFT;
extern uint8_t ORIENTATION_RIGHT_TOP;
extern uint8_t ORIENTATION_BOTTOM_RIGHT;
extern uint8_t ORIENTATION_LEFT_BOTTOM;

static int const MOSAIK2_CONTEXT_MAX_TILER_PROCESSES = 1024;

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

	int image_width;
	int image_height;
	unsigned char src_image_resolution; // -t count of mosaic elements at the shorter src-image side
	unsigned char database_image_resolution;

	int short_dim;
	uint32_t pixel_per_tile;
	double total_pixel_per_tile;
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
	 * image_width 19
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
	char filename[1024]; // trade off, dont want to malloc that much. hoping it fits
	FILE *file;
	size_t filesize;
	time_t lastmodified;
	time_t lastindexed;
	uint8_t tile_count;
	unsigned char *image_data;

	int imagedims[2];
	uint8_t tiledims[2];
	unsigned char hash[MD5_DIGEST_LENGTH];

	uint32_t total_tile_count;	
	uint8_t *colors;
	uint8_t *stddev;
};

typedef struct mosaik2_indextask_struct mosaik2_indextask;

struct mosaik2_database_struct {
	char thumbs_db_name[256];
	char imagestddev_filename[256];
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
	char lock_filename[256];
	char tileoffsets_filename[256];
	char lastmodified_filename[256];
	char lastindexed_filename[256];
	char createdat_filename[256];
	char phash_filename[256];

	int imagestddev_sizeof;
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
	int tileoffsets_sizeof;
	int lastmodified_sizeof;
	int lastindexed_sizeof;
	int createdat_sizeof;
	int phash_sizeof;

	uint8_t database_image_resolution;
	float histogram_color[3]; // all valid entries
	float histogram_stddev[3];
	uint32_t element_count;
};
typedef struct mosaik2_database_struct mosaik2_database;

struct mosaik2_database_element_struct {
	mosaik2_database *md;
	uint32_t element_number;
	unsigned char hash[MD5_DIGEST_LENGTH];
	char *filename;
	ssize_t filesize;
	int imagedims[2];
	unsigned char tiledims[2];
	time_t timestamp;
	unsigned char duplicate;
	unsigned char invalid;
	unsigned char tileoffsets[2];
	float histogram_color[3];
	float histogram_stddev[3];
};
typedef struct mosaik2_database_element_struct mosaik2_database_element;

struct mosaik2_project_struct {
	char dest_filename[256];
	char dest_primarytiledims_filename[256];
	char dest_result_filename[256];
	char dest_imagedims_filename[256];
	uint8_t ratio;
	uint8_t unique;
	uint8_t fast_unique;
	size_t file_size;
	uint8_t primary_tile_count;
	char dest_html_filename[ 256 ]; 
	char dest_src_filename[ 256 ];
	mosaik2_database *mds;
	uint8_t mds_len;
	int image_height; // populated by read_image_dim( *mosaik2_project)
	int image_width;  // populated by read_image_dim( *mosaik2_project)
	int pixel_per_tile;
	uint8_t primary_tile_x_count; //populated by mosaik2_project_read_primary_tile_dims( *mosaik2_project)
	uint8_t primary_tile_y_count;//populated by mosaik2_project_read_primary_tile_dims( *mosaik2_project)
};
typedef struct mosaik2_project_struct          mosaik2_project;

struct mosaik2_project_result_struct {
	uint32_t sortorder;
	mosaik2_database *md;
	uint8_t hash[16];
	uint32_t index; // index in thumbs_db
	float costs;
	uint8_t off_x;
	uint8_t off_y;
	char thumbs_db_filenames[MAX_FILENAME_LEN];
	char temp_filename[MAX_TEMP_FILENAME_LEN];
	int size;
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
	char *mosaik2db;
	char **mosaik2dbs;
	int mosaik2dbs_count;
	char *dest_image;
  int verbose;
	int dry_run;
	int database_image_resolution;
	int num_tiles;
	int has_num_tiles;
	int max_load, max_jobs;
	int unique;
	int fast_unique;
	int color_stddev_ratio;
	int pixel_per_tile;
	int duplicate_reduction;
	int symlink_cache;
	char *cache_path;
	int ignore_old_invalids;
	int no_hash_cmp;
	int color_distance;
	uint32_t element_number;
	int has_element_identifier;
	char *src_image;
	int quiet;
	char *element_filename;
	int phash_distance;
	int has_phash_distance;
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
