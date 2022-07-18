
#ifndef _MOSAIK2_H_
#define _MOSAIK2_H_

#define MAX_FILENAME_LEN 1024
#define MAX_TEMP_FILENAME_LEN 100

#define MOSAIK2_DATABASE_FORMAT_VERSION 5
#define MOSAIK2_VERSION "0.2"

extern const int FT_JPEG;
extern const int FT_PNG;
extern const int FT_ERR;

extern uint8_t ORIENTATION_TOP_LEFT;
extern uint8_t ORIENTATION_RIGHT_TOP;
extern uint8_t ORIENTATION_BOTTOM_RIGHT;
extern uint8_t ORIENTATION_LEFT_BOTTOM;

static int const MOSAIK2_CONTEXT_MAX_TILER_PROCESSES = 1024;

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
};

typedef struct mosaik2_context_struct mosaik2_context;

typedef enum {  INDEXTASK_STATE_INITIAL, INDEXTASK_STATE_LOADING, INDEXTASK_STATE_INDEXING, INDEXTASK_STATE_WRITING_INDEX, INDEXTASK_STATE_ENDING } TASK_STATE;

struct mosaik2_indextask_struct {
	time_t start;
	time_t end;
	TASK_STATE state;
	uint64_t idx;
	char filename[1024]; // trade off, dont want to malloc that much. hoping it fits
	FILE *file;
	size_t filesize;
	time_t lastmodified;
	uint8_t tile_count;
	unsigned char *image_data;


	int width;
	int height;
	int tile_x_count;
	int tile_y_count;
  unsigned char hash[MD5_DIGEST_LENGTH];

	uint32_t total_tile_count;	
	uint8_t *colors;
	uint8_t *colors_stddev;
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
	char tilecount_filename[256];
	char id_filename[256];
	char id[14];
	size_t id_len;
	char version_filename[256];
	char readme_filename[256];
	char pid_filename[256];
	char lock_filename[256];
	char lastmodified_filename[256];

	uint8_t tilecount;
};
typedef struct mosaik2_database_struct mosaik2_database;

struct mosaik2_project_struct {
	char dest_filename[256];
	char dest_primarytiledims_filename[256];
	char dest_result_filename[256];
	uint8_t ratio;
	uint8_t unique;
	size_t file_size;
	uint8_t primary_tile_count;
	char dest_html_filename[ 256 ]; 
	char dest_src_filename[ 256 ];
	mosaik2_database *mds;
	uint8_t mds_len;
};
typedef struct mosaik2_project_struct mosaik2_project;

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
	int max_load, max_jobs;
	int unique;
	int color_stddev_ratio;
	int pixel_per_tile;
	int duplicate_reduction;
	int symlink_cache;
	char *cache_path;
	int ignore_old_invalids;
	int no_hash_cmp;
};
typedef struct arguments_struct mosaik2_arguments;

int mosaik2_init(mosaik2_arguments*);
int mosaik2_index(mosaik2_arguments*);
int mosaik2_gathering(mosaik2_arguments*);
int mosaik2_join(mosaik2_arguments*);
int mosaik2_invalid(mosaik2_arguments*);
int mosaik2_duplicates(mosaik2_arguments*);

int mosaik2_clean(char *mosaik2_database_name);
int mosaik2_tiler(mosaik2_arguments *, mosaik2_database *, mosaik2_indextask *);
void mosaik2_index_write_to_disk(mosaik2_database *, mosaik2_indextask *);


#endif
