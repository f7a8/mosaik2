

#define MAX_FILENAME_LEN 1024
#define MAX_TEMP_FILENAME_LEN 100

#define MOSAIK2_DATABASE_FORMAT_VERSION 5

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
	char dest_mastertiledims_filename[256];
	char dest_result_filename[256];
	uint8_t ratio;
	uint8_t unique;
	uint32_t file_size;
	uint8_t master_tile_count;
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


int mosaik2_init(char *mosaik2_database_name, uint32_t tilecount);
int mosaik2_clean(char *mosaik2_database_name);
int mosaik2_index(char *mosaik2_database_name,  uint32_t max_tiler_processes, uint32_t max_loadavg);

/* usage param 1=> tile_count, 2=> file_size of the image in bytes. Image data is only accepted via stdin stream */
int mosaik2_tiler(mosaik2_database *, mosaik2_indextask *);
void mosaik2_index_write_to_disk(mosaik2_database *, mosaik2_indextask *);

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

