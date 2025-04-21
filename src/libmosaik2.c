#include "libmosaik2.h"


const m2ftype FT_JPEG = 0;
const m2ftype FT_PNG = 1;
const m2ftype FT_ERR = -1;
m2orient ORIENTATION_TOP_LEFT=0;
m2orient ORIENTATION_RIGHT_TOP=1;
m2orient ORIENTATION_BOTTOM_RIGHT=2;
m2orient ORIENTATION_LEFT_BOTTOM=3;

#ifdef HAVE_PHASH
const int PHASHES_VALID = 1;
const int PHASHES_INVALID = 0;

const int IS_PHASH_DUPLICATE = 2;
const int HAS_PHASH = 4;
const int HAS_NO_PHASH = 8;
#endif

const int IS_DUPLICATE = 1;
const int IS_NO_DUPLICATE = 0;

void mosaik2_context_init(mosaik2_context *ctx) {

	memset(ctx, 0, sizeof(mosaik2_context));
	memset(ctx->pids, 0, 1024*sizeof(pid_t));
	char *env_mosaik2_debug = getenv("MOSAIK2_DEBUG");
	if(env_mosaik2_debug != NULL) {
		ctx->debug = strncmp("1", getenv("MOSAIK2_DEBUG"), 1) == 0;
	}
}

void mosaik2_create_cache_dir() {
	char *home = getenv("HOME");
	size_t sz = snprintf(NULL, 0, "%s/.mosaik2/mosaik2.hash",home);
	char mkdir_buf[sz+1];
	memset(mkdir_buf,0,sz+1);
	snprintf(mkdir_buf, sz+1, "%s/.mosaik2/mosaik2.hash",home);
	m2cname mkdir_path = dirname(mkdir_buf);
	if(access(mkdir_path, W_OK)!=0) {
		//not accessible or writeable, try to create dir
		if( mkdir(mkdir_path, S_IRWXU | S_IRGRP | S_IROTH ) != 0) {
			fprintf(stderr, "cache directory (%s) could not be created\n", mkdir_path);
			exit(EXIT_FAILURE);
		}
	}
}

void mosaik2_database_init(mosaik2_database *md, m2name thumbs_db_name) {

	memset( md->id, 0, 17);
	md->id_len = 16;

	concat(md->thumbs_db_name,thumbs_db_name);
	concat(md->imagecolors_filename,thumbs_db_name,"/imagecolors.bin");
	concat(md->imagedims_filename, thumbs_db_name,"/imagedims.bin");
	concat(md->image_index_filename, thumbs_db_name,"/image.idx");
	concat(md->filenames_filename, thumbs_db_name,"/filenames.txt");
	concat(md->filenames_index_filename, thumbs_db_name,"/filenames.idx");
	concat(md->filehashes_filename, thumbs_db_name,"/filehashes.bin");
	concat(md->filehashes_index_filename, thumbs_db_name,"/filehashes.idx");
	concat(md->timestamps_filename, thumbs_db_name,"/timestamps.bin");
	concat(md->filesizes_filename, thumbs_db_name,"/filesizes.bin");
	concat(md->tiledims_filename, thumbs_db_name,"/tiledims.bin");
	concat(md->invalid_filename, thumbs_db_name,"/invalid.bin");
	concat(md->duplicates_filename, thumbs_db_name,"/duplicates.bin");
	concat(md->database_image_resolution_filename, thumbs_db_name,"/database_image_resolution.bin");
	concat(md->id_filename, thumbs_db_name,"/id.txt");
	concat(md->version_filename, thumbs_db_name,"/dbversion.txt");
	concat(md->readme_filename, thumbs_db_name,"/README.txt");
	concat(md->pid_filename, thumbs_db_name,"/mosaik2.pid");
	concat(md->lock_database_filename, thumbs_db_name,"/.lock_database");
	concat(md->lock_index_filename, thumbs_db_name,"/.lock_indexmode");
	concat(md->lastmodified_filename, thumbs_db_name,"/.lastmodified");
	concat(md->tileoffsets_filename, thumbs_db_name,"/tileoffsets.bin");
	concat(md->lastindexed_filename, thumbs_db_name,"/.lastindexed");
	concat(md->createdat_filename, thumbs_db_name,"/.createdat");
	concat(md->phash_filename, thumbs_db_name,"/phash.bin");

	md->imagecolors_sizeof = 3;
	md->imagedims_sizeof = 2*sizeof(uint32_t);
	md->image_index_sizeof = sizeof(off_t);
	md->filenames_index_sizeof = sizeof(off_t);
	md->filehashes_sizeof = MD5_DIGEST_LENGTH;
	md->filehashes_index_sizeof = MD5_DIGEST_LENGTH + sizeof(size_t);
	md->timestamps_sizeof = sizeof(time_t);
	md->filesizes_sizeof = sizeof(size_t);
	md->tiledims_sizeof = 2*sizeof(char);
	md->invalid_sizeof = sizeof(char);
	md->databaseimageresolution_sizeof = sizeof(m2rezo);
	md->duplicates_sizeof = sizeof(char);
	md->tileoffsets_sizeof = 2*sizeof(char);
	md->lastmodified_sizeof = sizeof(time_t);
	md->lastindexed_sizeof = sizeof(time_t);
	md->createdat_sizeof = sizeof(time_t);
	md->phash_sizeof = sizeof(m2phash);//sizeof(unsigned long long) + sizeof(int);
}

void mosaik2_project_init(mosaik2_project *mp, m2text mosaik2_database_id, m2name dest_filename) {
	/*size_t mosaik2_database_id_len = strlen(mosaik2_database_id);
	size_t dest_filename_len = strlen(dest_filename);*/

	//strncpy(mp->dest_filename, dest_filename, FILENAME_LEN);
	concat(mp->dest_filename, dest_filename);

	/*m2text thumbs_db_ending=".mtileres";
	size_t thumbs_db_ending_len = strlen(thumbs_db_ending);*/
	/*memset(mp->dest_primarytiledims_filename, 0, 256);
	//strncpy(mp->dest_primarytiledims_filename, mp->dest_filename, dest_filename_len);
	strcat(mp->dest_primarytiledims_filename, ".");
	strncat(mp->dest_primarytiledims_filename, mosaik2_database_id, mosaik2_database_id_len);
	strncat(mp->dest_primarytiledims_filename, thumbs_db_ending, thumbs_db_ending_len);*/
 	
	/*memset(mp->dest_result_filename, 0, 256);
	thumbs_db_ending=".result";
	thumbs_db_ending_len = strlen(thumbs_db_ending);
	//strncpy(mp->dest_result_filename, mp->dest_filename, dest_filename_len);
	strcat(mp->dest_result_filename, ".");
	strncat(mp->dest_result_filename, mosaik2_database_id, mosaik2_database_id_len);
	strncat(mp->dest_result_filename, thumbs_db_ending, thumbs_db_ending_len);*/
	concat(mp->dest_result_filename, mp->dest_filename, ".", mosaik2_database_id, ".result");

	/*memset(mp->dest_tile_infos_filename, 0, 256);
	thumbs_db_ending=".tile_info";
	thumbs_db_ending_len = strlen(thumbs_db_ending);
	//strncpy(mp->dest_tile_infos_filename, mp->dest_filename, dest_filename_len);
	strcat(mp->dest_tile_infos_filename, ".");
	strncat(mp->dest_tile_infos_filename, mosaik2_database_id, mosaik2_database_id_len);
	strncat(mp->dest_tile_infos_filename, thumbs_db_ending, thumbs_db_ending_len);*/
	concat(mp->dest_tile_infos_filename, mp->dest_filename, ".", mosaik2_database_id, ".tile_info");

	/*memset(mp->dest_html_filename, 0, 256);
	//strncpy(mp->dest_html_filename, mp->dest_filename, dest_filename_len);
	strcat(mp->dest_html_filename, ".html");*/
	concat(mp->dest_html_filename, mp->dest_filename, ".html");

	/*memset(mp->dest_html2_filename, 0, 256);
	//strncpy(mp->dest_html2_filename, mp->dest_filename, dest_filename_len);
	strcat(mp->dest_html2_filename, ".cover.html");*/
	concat(mp->dest_html2_filename, mp->dest_filename, ".cover.html");

	/*memset(mp->dest_src_filename, 0, 256);
	//strncpy(mp->dest_src_filename, mp->dest_filename, dest_filename_len);
	strcat(mp->dest_src_filename, ".src");*/
	concat(mp->dest_src_filename, mp->dest_filename, ".src");

}

void mosaik2_tile_infos_init(mosaik2_tile_infos *ti, m2rezo database_image_resolution, m2rezo src_image_resolution, uint32_t image_width, uint32_t image_height) {

	//assert(src_image_resolution > 0);
	//assert(database_image_resolution > 0 && database_image_resolution < UINT8_MAX);

	if(database_image_resolution*database_image_resolution*(2*RGB*UINT8_MAX) > UINT32_MAX) {
		fprintf(stderr, "database_image_resolution too high for internal data structure\n");
		exit(EXIT_FAILURE);
	}

	ti->image_width = image_width;
	ti->image_height = image_height;
	ti->short_dim = image_height > image_width ? image_width : image_height;

	ti->src_image_resolution = src_image_resolution; // old:num_tiles;
	ti->primary_tile_count = src_image_resolution;
	ti->database_image_resolution = database_image_resolution; // old:thumbs_tile_count

	ti->tile_count = src_image_resolution * database_image_resolution;
	assert(ti->tile_count != 0);

	if(image_width < ti->tile_count || image_height < ti->tile_count ) {
		fprintf(stderr,"image dimension is too small\n");
		exit(EXIT_FAILURE);
	}
 
	ti->pixel_per_tile = ti->short_dim / ti->tile_count; //automatically floored
	ti->total_pixel_per_tile = ti->pixel_per_tile * ti->pixel_per_tile;
  
  
  	ti->pixel_per_primary_tile = ti->pixel_per_tile * database_image_resolution; 
  	ti->total_pixel_per_primary_tile = ti->pixel_per_primary_tile * ti->pixel_per_primary_tile;
  
  	if(ti->short_dim == ti->image_width) {
  		//prevent primary_tile_x/y_count be greater than the src_image_resolution, (possible through floored pixel_per_tile)
  		ti->primary_tile_x_count = ti->src_image_resolution;
	} else {
		ti->primary_tile_x_count = image_width / ti->pixel_per_primary_tile;
	}
	if(ti->short_dim == ti->image_height) {
		ti->primary_tile_y_count = ti->src_image_resolution;
	} else {
		ti->primary_tile_y_count = image_height / ti->pixel_per_primary_tile;
	}
	ti->tile_x_count = ti->primary_tile_x_count * database_image_resolution;
	ti->tile_y_count = ti->primary_tile_y_count * database_image_resolution;
	ti->offset_x = (image_width -  ti->primary_tile_x_count * ti->pixel_per_primary_tile) / 2;
	ti->offset_y = (image_height - ti->primary_tile_y_count * ti->pixel_per_primary_tile) / 2;
	
	ti->total_tile_count = ti->tile_x_count * ti->tile_y_count;
	ti->total_primary_tile_count = ( ti->tile_x_count / database_image_resolution ) * ( ti->tile_y_count / database_image_resolution );

	ti->lx = ti->offset_x + ti->pixel_per_tile * ti->tile_x_count;
	ti->ly = ti->offset_y + ti->pixel_per_tile * ti->tile_y_count;
	
	ti->total_pixel_count = ti->pixel_per_tile * ti->tile_x_count * ti->pixel_per_tile * ti->tile_y_count;
	ti->ignored_pixel_count = (image_width * image_height) - ti->total_pixel_count;
}

void mosaik2_tiler_infos_init(mosaik2_tile_infos *ti, m2rezo database_image_resolution, uint32_t image_width, uint32_t image_height) {

	ti->database_image_resolution = database_image_resolution; // old:thumbs_tile_count

	//assert(database_image_resolution > 0 && database_image_resolution < UINT8_MAX);

	if(database_image_resolution*database_image_resolution*(6*256) > UINT32_MAX) {
		fprintf(stderr, "database_image_resolution too high for internal data structure\n");
		exit(EXIT_FAILURE);
	}

	ti->image_width = image_width;
	ti->image_height = image_height;

	if(ti->image_width  < (uint32_t)database_image_resolution 
	|| ti->image_height < (uint32_t)database_image_resolution) {
		fprintf(stderr,"image is too small, at least one dimension is smaller than the database_image_resolution\n");
		exit(EXIT_FAILURE);
	}
	
	ti->short_dim = ti->image_width<ti->image_height?ti->image_width:ti->image_height;
	ti->pixel_per_tile = ( ti->short_dim - (ti->short_dim % database_image_resolution) ) / database_image_resolution;
	ti->total_pixel_per_tile = ti->pixel_per_tile * ti->pixel_per_tile;


	if(ti->short_dim == ti->image_width){
			ti->tile_x_count = database_image_resolution;
			ti->tile_y_count = ti->image_height / ti->pixel_per_tile;
	} else {
			ti->tile_x_count = ti->image_width / ti->pixel_per_tile;
			ti->tile_y_count = database_image_resolution;
	}

	if( ti->tile_x_count >= UINT8_MAX || ti->tile_y_count >= UINT8_MAX ) {
		fprintf(stderr,"any tile dimension (x:%u, y:%u)must be < %u\n", ti->tile_x_count, ti->tile_y_count, UINT8_MAX);
		exit(EXIT_FAILURE);
	}

	ti->total_tile_count = ti->tile_x_count * ti->tile_y_count;

	ti->offset_x = ((ti->image_width - ti->tile_x_count * ti->pixel_per_tile)/2);
	ti->offset_y = ((ti->image_height - ti->tile_y_count * ti->pixel_per_tile)/2);

	ti->lx = ti->offset_x + ti->pixel_per_tile * ti->tile_x_count;
	ti->ly = ti->offset_y + ti->pixel_per_tile * ti->tile_y_count;

	ti->total_pixel_count = ti->pixel_per_tile * ti->tile_x_count * ti->pixel_per_tile * ti->tile_y_count;
	ti->ignored_pixel_count = (image_width * image_height) - ti->total_pixel_count;
}



int EndsWith(const char *str, const char *suffix) {
     if (!str || !suffix)
         return 0;
     size_t lenstr = strlen(str);
     size_t lensuffix = strlen(suffix);
     if (lensuffix >  lenstr)
         return 0;
     return strncasecmp(str + lenstr - lensuffix, suffix, lensuffix) == 0;
}

int StartsWith(const char *pre, const char *str) {
	size_t lenpre = strlen(pre),
	lenstr = strlen(str);
	return lenstr < lenpre ? 0 : memcmp(pre, str, lenpre) == 0;
}

int is_dest_file ( const char* pathname) {

	int exists_file = file_exists(pathname);
	int exists_dir = dir_exists(pathname);

	// destfile does not exists? fine its a new one
	if(!EndsWith(pathname, "/") && !exists_file && !exists_dir) {
		//fprintf(stderr, " is_dest_file:doesnotexist ");
		return 1;
		}


	// ok there is already a file, lets make sure if matched the file pattern of a destfile

	char dest_html_filename[MAX_FILENAME_LEN] = {0};
	char dest_src_filename[MAX_FILENAME_LEN] = {0};

	/*memset(dest_html_filename, 0, MAX_FILENAME_LEN);
	//strncpy(dest_html_filename, pathname, MAX_FILENAME_LEN-5);
	//strcat(dest_html_filename, ".html");*/
	concat(dest_html_filename, pathname, ".html");


	/*memset(dest_src_filename, 0, MAX_FILENAME_LEN);
	//strncpy(dest_src_filename, pathname, MAX_FILENAME_LEN-4);
	//strcat(dest_src_filename, ".src");*/
	concat(dest_src_filename, pathname, ".src");

	if( file_exists(dest_html_filename) &&
		file_exists(dest_src_filename) ) {
		return 1;
	}
	
	// if the destfile exists, but the extension files not, so this is not acceptable destfile to overwrite some unknown file
	return 0;

}

int is_src_file (const char* pathname ) {

	// ok there is already a file, lets make sure if matched the file pattern of a destfile

	char dest_html_filename[MAX_FILENAME_LEN] = {0};
	char dest_src_filename[MAX_FILENAME_LEN] = {0};

	/*memset(dest_html_filename, 0, MAX_FILENAME_LEN);
	//strncpy(dest_html_filename, pathname, MAX_FILENAME_LEN-5);
	//strcat(dest_html_filename, ".html");*/
	concat(dest_html_filename, pathname, ".html");

	/*memset(dest_src_filename, 0, MAX_FILENAME_LEN);
	//(dest_src_filename, pathname, MAX_FILENAME_LEN-4);
	//strcat(dest_src_filename, ".src");*/
	concat(dest_src_filename, pathname, ".src");

	 return file_exists(pathname) 
	 && get_file_type(pathname) == FT_JPEG 
	 && !file_exists(dest_html_filename) 
	 && !file_exists(dest_src_filename);
}

// must exist
int is_file ( const char* pathname ) {
	struct stat statbuf;
	m_stat(pathname, &statbuf);
	return statbuf.st_mode & S_IFREG;
}
 //does not fail if stat fails in an intended manor.
int file_exists(const char *pathname) {
	struct stat statbuf;
	int val = stat(pathname, &statbuf);
	if(val != 0) {
		//does not exists -> fine
		if(errno == ENOENT) {
			return 0;
		} 
		fprintf(stderr, "file_exists: error stat()ing file for existance\n");
		exit(EXIT_FAILURE);
	}
	return statbuf.st_mode & S_IFREG;
}

int dir_exists(const char* pathname) {
	struct stat statbuf;
	int val = stat(pathname, &statbuf);
	if(val != 0) {
		//does not exists -> fine
		if(errno == ENOENT) {
			return 0;
		} 
		fprintf(stderr, "dir_exists: error stat()ing file for existance\n");
		exit(EXIT_FAILURE);
	}
	return statbuf.st_mode & S_IFDIR;
}

int is_file_local(const m2name filename ) {
	if(!filename) {
		fprintf(stderr, "no filename\n");
		exit(EXIT_FAILURE);
	}
	//size_t len_filename = strlen(filename);
	if( StartsWith("https://", filename ) || StartsWith("http://", filename))
		return 0;
	return 1;
}

int is_same_file(const m2name filename0, const m2name filename1) {
	assert(filename0!=NULL);
	assert(filename1!=NULL);
	struct stat st;
	m_stat(filename0, &st);
	ino_t i0 = st.st_ino;
	m_stat(filename1, &st);
	ino_t i1 = st.st_ino;
	return i0 == i1;
}

off_t get_file_size(const m2name filename) {
	assert(filename != NULL );
	assert(strlen(filename)>0);
	struct stat st;
	m_stat(filename, &st);
	off_t size = st.st_size;
	return size;
}
 
m2ftype get_file_type(const char *dest_filename) {
	if(EndsWith(dest_filename, "jpg") || EndsWith(dest_filename, "jpeg") )
		return FT_JPEG;
	if(EndsWith(dest_filename, "png"))
		return FT_PNG;
	return FT_ERR;
}

m2ftype get_file_type_from_buf(uint8_t *buf, size_t len) {
	if(len < 2) {
		return FT_ERR;
	}
	if(buf[0]==0xFF && buf[1]==0xD8) {
		return FT_JPEG;
	}
	if(buf[0]==0x80 && buf[1]==0x50) {
		return FT_PNG; // just a guess... but thats save enough for me at this point
	}
	return FT_ERR;
}

m2elem mosaik2_database_read_element_count(mosaik2_database *md) {

	off_t db_filesizes_size = get_file_size(md->filesizes_filename);
	if(db_filesizes_size == 0)
		return 0;
	// the *.db.filesizes is a binary file that includes the original 
	// filesizes of the indexed images and its saved as 4 byte integers
	return (m2elem)(db_filesizes_size / sizeof(size_t));
}

m2rezo mosaik2_database_read_image_resolution(mosaik2_database *md) {
	m2file database_image_resolution_file = m_fopen(md->database_image_resolution_filename, "rb");
	m2rezo database_image_resolution=0;
	m_fread(&database_image_resolution, sizeof(database_image_resolution), database_image_resolution_file);
	m_fclose( database_image_resolution_file );
	assert(database_image_resolution != 0);
	return database_image_resolution;
}

m2elem mosaik2_database_read_duplicates_count(mosaik2_database *md) {
	m2file file = m_fopen(md->duplicates_filename, "rb");
	
	m2elem duplicates_count = 0;
	uint8_t buf[BUFSIZ];
	while( feof(file) == 0) {
		size_t s = fread(&buf, 1, BUFSIZ, file);
		for(size_t i=0;i<s;i++) {
			if(buf[i]!=0)
				duplicates_count++;
		}
	}
	m_fclose( file );
	return duplicates_count;
}

m2elem mosaik2_database_read_invalid_count(mosaik2_database *md) {
	m2file file = m_fopen(md->invalid_filename, "rb");
	m2elem count = 0;
	uint8_t buf[BUFSIZ];
	while( feof(file) == 0) {
		size_t s = fread(&buf, 1, BUFSIZ, file);
		for(size_t i=0;i<s;i++) {
			if(buf[i]!=0) // only 0 marks a valid entry
				count++;
		}
	}
	m_fclose( file );
	return count;
}

m2elem mosaik2_database_read_valid_count(mosaik2_database *md) {
	m2file file  = m_fopen(md->invalid_filename, "rb");
	m2file file1 = m_fopen(md->duplicates_filename, "rb");

	size_t file_size = get_file_size(md->invalid_filename);
	size_t file1_size = get_file_size(md->duplicates_filename);

	assert(file_size == file1_size);

	m2elem count = 0;
	unsigned char buf[BUFSIZ];
	unsigned char buf1[BUFSIZ];

	size_t s = 0, s1 = 0;
	while( (s = fread(&buf, 1, BUFSIZ, file)) != 0 &&
		  (s1 = fread(&buf1, 1, BUFSIZ,file1))!= 0 ) {
	for(size_t i=0;i<s;i++) {
		if(!(buf[i]!=0 || buf1[i]!=0)) // only 0s marks a valid entries
			count++;
		}
	}

	m_fclose( file );
	m_fclose( file1 );

	return count;
}

uint32_t mosaik2_database_read_tileoffset_count(mosaik2_database *md) {
	m2file file = m_fopen(md->tileoffsets_filename, "rb");
	
	uint32_t count = 0;
	uint8_t buf[BUFSIZ];
	while( feof(file) == 0) {
		size_t s = fread(&buf, 1, BUFSIZ, file);
		for(size_t i=0;i<s;i+=2) {
			if(!( buf[i]==0xFF && buf[i+1]==0xFF)) // 2x 0xFF marks unset, everything else should have a custom value
				count++;
		}
	}
	m_fclose( file );
	return count;
}

void mosaik2_database_read_database_id(mosaik2_database *md) {

	m2file file = m_fopen(md->id_filename,"r");
	m_fgets( md->id, md->id_len, file);
	m_fclose( file );
}

//filename has to be freed in the end
//md.database_image_resolution must be set before
void mosaik2_database_read_element(mosaik2_database *md, mosaik2_database_element *mde, m2elem element_number) {
	mde->md = md;
	mde->element_number = element_number;
	mde->filename = mosaik2_database_read_element_filename(md, element_number, NULL, NULL);
	read_entry( md->filehashes_filename,  mde->hash,         md->filehashes_sizeof, element_number*md->filehashes_sizeof);
	read_entry(  md->filesizes_filename, &mde->filesize,      md->filesizes_sizeof, element_number*md->filesizes_sizeof);
	read_entry(  md->imagedims_filename, &mde->imagedims,     md->imagedims_sizeof, element_number*md->imagedims_sizeof);
	read_entry(   md->tiledims_filename, &mde->tiledims,       md->tiledims_sizeof, element_number*md->tiledims_sizeof);
	read_entry( md->timestamps_filename, &mde->timestamp,    md->timestamps_sizeof, element_number*md->timestamps_sizeof);
	read_entry( md->duplicates_filename, &mde->duplicate,    md->duplicates_sizeof, element_number*md->duplicates_sizeof);
	read_entry(    md->invalid_filename, &mde->invalid,         md->invalid_sizeof, element_number*md->invalid_sizeof);
	read_entry(md->tileoffsets_filename, &mde->tileoffsets, md->tileoffsets_sizeof, element_number*md->tileoffsets_sizeof);
	#ifdef HAVE_PHASH
	//TODO save phash_filesize
	off_t phash_filesize = get_file_size(md->phash_filename);
	if(phash_filesize>=1*md->phash_sizeof&&phash_filesize>=element_number*md->phash_sizeof) {
		read_entry(md->phash_filename, &mde->phash, sizeof(mde->phash), element_number*md->phash_sizeof);
		mde->has_phash=HAS_PHASH;
	} else {
		mde->has_phash=HAS_NO_PHASH;
	}
	#endif
	off_t imagecolors_offset;
	read_entry(md->image_index_filename, &imagecolors_offset, sizeof(imagecolors_offset), element_number*sizeof(imagecolors_offset));
	//int short_dim = mde->tiledims[0] > mde->tiledims[1] ? mde->tiledims[1] : mde->tiledims[0];


	int x0,y0,xl,yl,total_tile_count; 
	float total_tile_count_f;
	if(mde->tileoffsets[0] == 0xFF && mde->tileoffsets[1] == 0xFF ) { // then undefined
		x0 = 0;
		y0 = 0;
		xl = mde->tiledims[0];
		yl = mde->tiledims[1];
		total_tile_count = xl * yl;
		total_tile_count_f = (float ) total_tile_count;
	} else { // recognize only cropped area
		x0 = mde->tileoffsets[0];
		y0 = mde->tileoffsets[1];
		xl = x0 + md->database_image_resolution;
		yl = y0 + md->database_image_resolution;
		total_tile_count = md->database_image_resolution * md->database_image_resolution;
		total_tile_count_f = (float) total_tile_count;
	}

	unsigned char imagecolors[total_tile_count*RGB];
	read_entry(md->imagecolors_filename, &imagecolors, total_tile_count*RGB, imagecolors_offset);

	//fprintf(stderr, "###x0:%i y0:%i xl:%i yl:%i tiledims:%i %i tile_count_f:%f\n", x0, y0, xl, yl, mde->tiledims[0], mde->tiledims[1], total_tile_count_f);
	for(int x=x0;x<xl;x++) {
		for(int y=y0;y<yl;y++) {
			int idx = (x * mde->tiledims[1] + y) * RGB;
			mde->histogram_color[R] += imagecolors[idx+R] / total_tile_count_f;
			mde->histogram_color[G] += imagecolors[idx+G] / total_tile_count_f;
			mde->histogram_color[B] += imagecolors[idx+B] / total_tile_count_f;
		}
	}
}
float mosaik2_database_costs(mosaik2_database *md, mosaik2_database_element *mde) {
	double diff_c = sqrt(
			  pow(md->histogram_color[R] - mde->histogram_color[R], 2)
			+ pow(md->histogram_color[G] - mde->histogram_color[G], 2)
			+ pow(md->histogram_color[B] - mde->histogram_color[B], 2)
			);

	return diff_c;
}

static int cmp_off_t(const void *p1, const void *p2) {
	off_t *v1 = (off_t *)p1;
	off_t *v2 = (off_t *)p2;

	if(*v1<*v2)
		return -1;
	if(*v1>*v2)
		return 1;
	return 0;
}

/** returns 0 on success, -1 when no filename was found and -2 if the found string position is a substring ,
 *
 * searches with memmem in a memory mapped file for the filenames, rechecks the found offset with saved offsets in filenames_index 
 * if theres a match a filename was found from its beginning.
 * the recheck search in the filenames_index file uses also a memory mapped file and bsearch, because the offsets in filenames_index due to its nature are saved in order. 
 */
int mosaik2_database_find_element_number(mosaik2_database *md, m2name filename, m2elem *found_element_number) {

	m2fd fd = m_open(md->filenames_filename, O_RDONLY, 0);
	struct stat st;
	m_fstat(fd, &st);
	void *ptr = m_mmap(NULL, st.st_size, PROT_READ, MAP_SHARED, fd, 0);

	const void *haystack = (const void*) ptr;
	size_t haystacklen = (size_t) st.st_size;
	const void *needle = (const void *)filename;
	size_t needlelen = strlen(filename);

	void *found = memmem(haystack, haystacklen, needle, needlelen);

	m_munmap(ptr, (size_t)st.st_size );
	m_close(fd);

	if(found==NULL)
		return -1;

	size_t found_character_position = (void *)found- (void *)ptr;

	fd = m_open(md->filenames_index_filename, O_RDONLY, 0);
	m_stat(md->filenames_index_filename, &st);

	ptr = m_mmap(NULL, st.st_size, PROT_READ, MAP_SHARED, fd, 0);
	size_t nmemb = st.st_size / md->filenames_index_sizeof;

	found = bsearch( &found_character_position, ptr, nmemb, md->filenames_index_sizeof, cmp_off_t);
	if(found == NULL) {
		return -2;
	}
	m2elem element_number = (found - ptr)/md->filenames_index_sizeof;
	m_munmap(ptr, (size_t)st.st_size);
	m_close(fd);

	*found_element_number = element_number;
	return 0;
}

/**
 * reads the filename at element_number from the mosaik2_database.
 * if FILE*s are null they are opend and closed
 * the returned char pointer has to freed manually.
 */
__attribute__((malloc)) m2name mosaik2_database_read_element_filename(mosaik2_database *md, m2elem element_number, m2file filenames_index_file,m2file filenames_file) {
	off_t offsets[2];
	int fileidx_was_null = filenames_index_file == NULL;
	int filename_was_null = filenames_file == NULL;
	// find starting offset from filenames index file for real filenames file
	if(fileidx_was_null)
		filenames_index_file = m_fopen(md->filenames_index_filename, "r");
	if(filename_was_null)
		filenames_file = m_fopen(md->filenames_filename, "r");

	m_fseeko(filenames_index_file, element_number*sizeof(off_t), SEEK_SET);
	size_t read = fread(offsets, sizeof(off_t), 2, filenames_index_file);
	if(read == 0) {
		fprintf(stderr, "error: could not read filenames offset from filenames index file (element:%u)\n", element_number);
		exit(EXIT_FAILURE);
	}
	if(read == 1) { // this was the last entry in the file. so there is no next entry
		offsets[1] = get_file_size(md->filenames_filename);
		//fprintf(stderr, "next offset from filesize\n");
	}
	if(fileidx_was_null)
		m_fclose(filenames_index_file);

	offsets[1]--;
	off_t filename_len = offsets[1]-offsets[0];

	m2name filename = m_calloc(1,filename_len+1);
	read_file_entry(filenames_file, filename, filename_len, offsets[0]);
	if(filename_was_null)
		m_fclose(filenames_file);
	return filename;
}

void mosaik2_project_read_primary_tile_dims(mosaik2_project *mp) {
	mosaik2_tile_infos ti;
	m2file ti_file = m_fopen(mp->dest_tile_infos_filename, "r");
	m_fread(&ti, sizeof(ti), ti_file);
	m_fclose(ti_file);

	mp->primary_tile_x_count = ti.primary_tile_x_count;
	mp->primary_tile_y_count = ti.primary_tile_y_count;
}

size_t mosaik2_database_read_size(mosaik2_database *md) {
	return (size_t)
    ( get_file_size( md->imagecolors_filename)
		+ get_file_size( md->imagedims_filename)
		+ get_file_size( md->image_index_filename)
		+ get_file_size( md->filenames_filename)
		+ get_file_size( md->filenames_index_filename)
		+ get_file_size( md->filehashes_filename)
		+ get_file_size( md->filehashes_index_filename)
		+ get_file_size( md->timestamps_filename)
		+ get_file_size( md->filesizes_filename)
		+ get_file_size( md->tiledims_filename)
		+ get_file_size( md->invalid_filename)
		+ get_file_size( md->duplicates_filename)
		+ get_file_size( md->database_image_resolution_filename)
		+ get_file_size( md->id_filename)
		+ get_file_size( md->version_filename)
		+ get_file_size( md->readme_filename)
		+ get_file_size( md->lastmodified_filename)
		+ get_file_size( md->tileoffsets_filename));
}

time_t mosaik2_database_read_createdat(mosaik2_database *md) {
	m2file file = m_fopen(md->createdat_filename, "rb");
	time_t t=0;
	m_fread(&t, sizeof(t), file);
	m_fclose( file );
	return t;
}
time_t mosaik2_database_read_lastindexed(mosaik2_database *md) {
	time_t t=0;

	if(get_file_size(md->lastindexed_filename)== sizeof(t) ) {
		m2file file = m_fopen(md->lastindexed_filename, "rb");
		m_fread(&t, sizeof(t), file);
		m_fclose( file );
	}
	return t;
}
time_t mosaik2_database_read_lastmodified(mosaik2_database *md) {
	time_t t=0;
	if(get_file_size(md->lastmodified_filename)==sizeof(t)) {
		m2file file = m_fopen(md->lastmodified_filename, "rb");
		m_fread(&t, sizeof(t), file);
		m_fclose( file );
	}
	return t;
}
/**
void check_thumbs_db_name(char *thumbs_db_name) {

	uint32_t thumbs_db_name_len = strlen( thumbs_db_name );
	if(thumbs_db_name_len > 100) {
		fprintf(stderr,"thumbs_db_name is too long. Only 100 characters allowed");
		exit(EXIT_FAILURE);
	}

	char valid_signs[] = {"abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_-"};
	uint8_t valid_signs_count = strlen( valid_signs );
	uint8_t valid = 0;
	//thumbs_db_name must have has at least 100 characters here
	for(uint8_t i = 0; i < thumbs_db_name_len; i++) {
		valid = 0;
		for(uint8_t j = 0; j < valid_signs_count; j++) {
			if(thumbs_db_name[i] == valid_signs[j] ) {
				valid = 1;
				break;
			}
		}
		if( valid == 0) {
			fprintf(stderr, "thumbs_db_name (%s) has illegal characters. Allowed are a..z, A..Z, 0..9, underscore and dash\n", thumbs_db_name);
			exit(EXIT_FAILURE);
		}
	}	

} */
void mosaik2_project_check(mosaik2_project *mp) {
	m_access( mp->dest_filename, F_OK);
	m_access( mp->dest_tile_infos_filename, F_OK);
	m_access( mp->dest_result_filename, F_OK);
}

void mosaik2_database_check(mosaik2_database *md) {

	//check_thumbs_db_name( md->thumbs_db_name );

	m_access( md->thumbs_db_name, F_OK );
	m_access( md->imagecolors_filename, F_OK );
	m_access( md->imagedims_filename, F_OK );
	m_access( md->image_index_filename, F_OK );
	m_access( md->filenames_filename, F_OK );
	m_access( md->filenames_index_filename, F_OK );
	m_access( md->filesizes_filename, F_OK );
	m_access( md->filehashes_filename, F_OK );
	m_access( md->filehashes_index_filename, F_OK );
	m_access( md->tiledims_filename, F_OK );
	m_access( md->invalid_filename, F_OK );
	m_access( md->duplicates_filename, F_OK );
	m_access( md->database_image_resolution_filename, F_OK );
	m_access( md->lock_database_filename, F_OK);
	m_access( md->lock_index_filename, F_OK);
	m_access( md->lastmodified_filename, F_OK) ;
	m_access( md->tileoffsets_filename, F_OK);

	// TODO make more plause checks
	m2elem element_count = mosaik2_database_read_element_count(md);
	md->element_count = element_count;

	assert(get_file_size(md->database_image_resolution_filename) == md->databaseimageresolution_sizeof);

	m2rezo database_image_resolution = mosaik2_database_read_image_resolution(md);
	
	assert(get_file_size(md->imagecolors_filename)     >= element_count * md->imagecolors_sizeof*database_image_resolution*database_image_resolution);

	assert(get_file_size(md->imagecolors_filename)     <= element_count * md->imagecolors_sizeof*256*256);

	assert(get_file_size(md->imagedims_filename)       == element_count * md->imagedims_sizeof);
	assert(get_file_size(md->image_index_filename)     == element_count * md->image_index_sizeof);
	assert(get_file_size(md->filenames_filename)       >= element_count * 2); // at least one character + newline
	assert(get_file_size(md->filenames_index_filename) == element_count * md->filenames_index_sizeof);
	assert(get_file_size(md->filesizes_filename)       == element_count * md->filesizes_sizeof);
	assert(get_file_size(md->filehashes_filename)      == element_count * md->filehashes_sizeof);
//TODO	assert(get_file_size(md->filehashes_index_filename)== element_count * md->filehashes_index_sizeof
	   // || get_file_size(md->filehashes_index_filename)== 0);
	assert(get_file_size(md->tiledims_filename)        == element_count * md->tiledims_sizeof);
	assert(get_file_size(md->invalid_filename)         == element_count * md->invalid_sizeof);
	assert(get_file_size(md->duplicates_filename)      == element_count * md->duplicates_sizeof);
	
	//m_access( md->database_image_resolution_filename, F_OK );
	assert(get_file_size(md->lastindexed_filename)    == (element_count > 0 ? md->lastindexed_sizeof : 0));
	assert(get_file_size(md->tileoffsets_filename)     == element_count * md->tileoffsets_sizeof);
}

void mosaik2_database_check_pid_file(mosaik2_database *md) {
	if( access(md->pid_filename, F_OK) == 0) {
		fprintf(stderr, "The pid file (%s) exists, either an indexing process is"
		 " running or the program did not exit correctly. In the first case, the"
		 " pid file can be deleted.\n", md->pid_filename);
		exit(EXIT_FAILURE);
	}
}

void mosaik2_database_lock_writer(mosaik2_database *md) {
	m2fd lockfile = m_open(md->lock_database_filename, O_RDONLY, 0);
	int val = flock(lockfile, LOCK_EX|LOCK_NB);
	
	if(val ==0) {
		//exclusive lock applyed until process termination
	} else if(errno == EWOULDBLOCK) {
		fprintf(stderr, "Cannot lock database (%s) for write operation yet.\n"
		"There should be an active mosaik2 instance, which does a read "
		"operation.\n", md->thumbs_db_name);
		exit(EXIT_FAILURE);
	}
}

void mosaik2_database_lock_reader(mosaik2_database *md) {

	m2fd lockfile = m_open(md->lock_database_filename, O_RDONLY, 0);
	int val = flock(lockfile, LOCK_SH|LOCK_NB);
	
	if(val ==0) {
		//shared lock applyed until process termination
	} else if(errno == EWOULDBLOCK) {
		fprintf(stderr, "Cannot lock database (%s) for read operation yet.\n"
		"There should be an active mosaik2 instance, which does a write "
		"operation.\n", md->thumbs_db_name);
		exit(EXIT_FAILURE);
	}
}

m2ftype mosaik2_project_check_dest_filename(m2name dest_filename) {
	
	uint32_t dest_filename_len = strlen(dest_filename);
	if( dest_filename_len > 100 || dest_filename_len < 1 ) {
		fprintf(stderr, "dest_filename should have 1 to 100 characters\n");
		exit(EXIT_FAILURE);
	}

	/*if(dest_filename[0] == '.') {
		fprintf(stderr, "dest_filename (%s) must not start with a dot\n", dest_filename);
		exit(EXIT_FAILURE);
	}*/
/*
	char valid_signs[] = {"abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_-."};
	uint8_t valid_signs_count = strlen( valid_signs );
	uint8_t valid = 0;

	//thumbs_db_name must have has at least 100 characters here
	for(uint8_t i = 0; i < dest_filename_len; i++) {
		valid = 0;
		for(uint8_t j = 0; j < valid_signs_count; j++) {
			if(dest_filename[i] == valid_signs[j] ) {
				valid = 1;
				break;
			}
		}
		if( valid == 0) {
			fprintf(stderr, "dest_filename (%s) has illegal characters. Allowed are a..z, A..Z, 0..9, underscores, dashes and dots.\n", dest_filename);
			exit(EXIT_FAILURE);
		}
	}
*/
	m2ftype ft;
	if( (ft = get_file_type(dest_filename)) == FT_ERR) {
		fprintf(stderr, "illegal file type for destination file\n");
		exit(EXIT_FAILURE);
	}
	return ft;
}

/*void check_thumbs_tile_count(uint32_t thumbs_tile_count) {
  if(thumbs_tile_count*thumbs_tile_count*(6*256)>UINT32_MAX) {
    //can candidates_costs contain the badest possible costs?
    fprintf(stderr, "thumb tile size too high for internal data structure\n");
    exit(EXIT_FAILURE);
  }
}*/

void remove_newline(char *str) {
	size_t l = strlen(str);
	if(l>0 && str[l-1]=='\n')
		str[l-1]='\0';
}

int cmpfunc (const void * a, const void * b) {
	//fprintf(stderr,".");
	mosaik2_project_result *a0 = (mosaik2_project_result *)a;
	mosaik2_project_result *b0 = (mosaik2_project_result *)b;

	//fprintf(stderr,"(%s)(%s).",a0->thumbs_db_name,b0->thumbs_db_name);
	int strcmp0 = a0->md < b0->md;
	//fprintf(stderr,"%i.",strcmp0);
	if(strcmp0 < 0)
		return -1;
	if(strcmp0 > 0)
		return 1;

	if(a0->index > b0->index)
		return 1;
	if(a0->index < b0->index)
		return -1;
	return 0;
}

int cmpfunc_back(const void *a, const void *b) {
	mosaik2_project_result *a0 = (mosaik2_project_result *)a;
	mosaik2_project_result *b0 = (mosaik2_project_result *)b;
	if(a0->sortorder > b0->sortorder)
		return 1;
	if(a0->sortorder < b0->sortorder)
		return -1;
	return 0;
}

int File_Copy(char FileSource[], char FileDestination[])
{
    char    c[BUFSIZ]; // or any other constant you like
    FILE    *stream_R = m_fopen(FileSource, "r");
    FILE    *stream_W = m_fopen(FileDestination, "w");   //create and write to file
    while (!feof(stream_R)) {
        size_t bytes = fread(c, 1, sizeof(c), stream_R);
        if (bytes) {
            fwrite(c, 1, bytes, stream_W);
        }
    }

    //close streams
    m_fclose(stream_R);
    m_fclose(stream_W);

    return 0;
}





#ifdef HAVE_LIBEXIF
m2orient get_image_orientation(void *buf, size_t buf_size) {

	ExifData *ed;
	ExifEntry *entry;

  /* Load an ExifData object from an EXIF m2file */
	ed = exif_data_new_from_data(buf, buf_size);
	if (ed==NULL) {
		printf("unable to create exif data\n");
		exit(EXIT_FAILURE);
	}

	entry = exif_content_get_entry(ed->ifd[EXIF_IFD_0], EXIF_TAG_ORIENTATION);
	if (entry) {
		char buf0[64] = {0};
		if (exif_entry_get_value(entry, buf0, sizeof(buf0))) {
			trim_spaces(buf);

			if (strcmp(buf0, "Right-top")==0) {
				return ORIENTATION_RIGHT_TOP;
			} else if(strcmp(buf0, "Bottom-right")==0) {
				return ORIENTATION_BOTTOM_RIGHT;
			} else if(strcmp(buf0, "Left-Bottom")==0) {
				return ORIENTATION_LEFT_BOTTOM;
			}
    	}
	}
	return ORIENTATION_TOP_LEFT;
}
#else
m2orient get_image_orientation(void *buf, size_t buf_size) {
	return ORIENTATION_TOP_LEFT;
}
#endif


gdImagePtr read_image_from_file(m2name filename) {
	m2file in = m_fopen(filename, "rb");
	size_t file_size = get_file_size(filename);
	void *buf = m_calloc(1, file_size);
	m_fread(buf, file_size, in);
	gdImagePtr im = read_image_from_buf(buf, file_size);
	m_free((void**)&buf);
	m_fclose(in);
	return im;
}

gdImagePtr read_image_from_buf(void *buf, size_t file_size) {
   gdImagePtr im;
   m2ftype file_type = get_file_type_from_buf(buf, file_size);
   if( file_type == FT_JPEG ) {
	   im = gdImageCreateFromJpegPtr( file_size, buf);
   } else {
	fprintf(stderr, "wrong image type, only jpegs accepted\n");
   	exit(EXIT_FAILURE);
   }
   if(im ==NULL){
	   fprintf(stderr,"image could not be instanciated\n");
	   exit(EXIT_FAILURE);
   }
	
	m2orient orientation = get_image_orientation(buf, file_size);
//uint8_t ORIENTATION_TOP_LEFT=0;
//uint8_t ORIENTATION_RIGHT_TOP=1; 270
//uint8_t ORIENTATION_BOTTOM_RIGHT=2; 180
//uint8_t ORIENTATION_LEFT_BOTTOM=3; 90
	if(orientation == ORIENTATION_BOTTOM_RIGHT ) {
		gdImagePtr im2 = gdImageRotate180(im);
		gdImageDestroy(im);
		im=im2;
	} else if(orientation == ORIENTATION_RIGHT_TOP) { 
  		gdImagePtr im2;
		im2 = gdImageRotate270(im); // 270
		gdImageDestroy(im);
		im = im2;
	//	case ORIENTATION_BOTTOM_RIGHT: im = gdImageRotate90(im,0); break;//180
	//	case ORIENTATION_LEFT_BOTTOM: im = gdImageRotate90(im,0); break;
	} else if(orientation == ORIENTATION_LEFT_BOTTOM ) {
		gdImagePtr im2 = gdImageRotate90(im);
		gdImageDestroy(im);
		im = im2;
	}

   return im;
 }

/* Remove spaces on the right of the string */
//static void trim_spaces(char *buf) {
void trim_spaces(char *buf) {
    char *s = buf-1;
    for (; *buf; ++buf) {
        if (*buf != ' ')
            s = buf;
    }
    *++s = 0; /* nul terminate the string on the first of the final spaces */
}



/* Show the tag name and contents if the tag exists */
//void show_tag(ExifData *d, ExifIfd ifd, ExifTag tag)
//{
//    /* See if this tag exists */
//    ExifEntry *entry = exif_content_get_entry(d->ifd[ifd],tag);
//    if (entry) {
//        char buf[1024];
//
//        /* Get the contents of the tag in human-readable form */
//        exif_entry_get_value(entry, buf, sizeof(buf));
//
//        /* Don't bother printing it if it's entirely blank */
//        trim_spaces(buf);
//        if (*buf) {
//            printf("%s\t%s\t%02X\n", exif_tag_get_name_in_ifd(tag,ifd), buf, entry->data[0]);
//        }
//    }
//}

/* Show the given MakerNote tag if it exists */
//void show_mnote_tag(ExifData *d, unsigned tag)
//{
//    ExifMnoteData *mn = exif_data_get_mnote_data(d);
//    if (mn) {
//        int num = exif_mnote_data_count(mn);
//        int i;
//
//        /* Loop through all MakerNote tags, searching for the desired one */
//        for (i=0; i < num; ++i) {
//            char buf[1024];
//            if (exif_mnote_data_get_id(mn, i) == tag) {
//                if (exif_mnote_data_get_value(mn, i, buf, sizeof(buf))) {
//                    /* Don't bother printing it if it's entirely blank */
//                    trim_spaces(buf);
//                    if (*buf) {
//                        printf("%s: %s\n", exif_mnote_data_get_title(mn, i),
//                            buf);
//                    }
//                }
//            }
//        }
//    }
//}

/* Rotates an image by 90 degrees (counter clockwise) */
gdImagePtr gdImageRotate90 (gdImagePtr src) {
	int uY, uX;
	int c;
	gdImagePtr dst;
	
	dst = gdImageCreateTrueColor(src->sy, src->sx);
	if (dst != NULL) {
		for (uY = 0; uY<src->sy; uY++) {
			for (uX = 0; uX<src->sx; uX++) {
				c =  src->tpixels[uY][uX];
				gdImageSetPixel(dst, uY, (dst->sy - uX - 1), c);
			}
		}
	}

	return dst;
}
/* Rotates an image by 180 degrees (counter clockwise) */
gdImagePtr gdImageRotate180 (gdImagePtr src) {
	int uY, uX;
	int c;
	gdImagePtr dst;
 
	dst = gdImageCreateTrueColor(src->sx, src->sy);
	if (dst != NULL) {
		for (uY = 0; uY<src->sy; uY++) {
			for (uX = 0; uX<src->sx; uX++) {
				c = src->tpixels[uY][uX];
				
			//	fprintf(stderr, "c:%i @ uX:%i uY:%i sx:%i sy:%i dx:%i dy:%i tx:%i ty:%i\n",c, uX, uY, src->sx, src->sy, dst->sx, dst->sy,(dst->sx - uX-1),(dst->sy-uY-1));
				gdImageSetPixel(dst, (dst->sx - uX - 1), (dst->sy - uY - 1), c);
			}
		}
	}
	return dst;
}	
/* Rotates an image by 90 degrees (counter clockwise) */
gdImagePtr gdImageRotate270 (gdImagePtr src) {
	int uY, uX;
	int c;
	gdImagePtr dst;

	dst = gdImageCreateTrueColor (src->sy, src->sx);

	if (dst != NULL) {
		for (uY = 0; uY<src->sy; uY++) {
			for (uX = 0; uX<src->sx; uX++) {
				c = src->tpixels[uY][uX];
			//	fprintf(stderr, "c:%i @ %i:%i sx:%i sy:%i\n",c, uX, uY, src->sy, src->sx);
				gdImageSetPixel(dst, (dst->sx - uY - 1), uX, c);
			}
		}
	}
	return dst;
}	

void check_resolution(int32_t resolution) {
	if(resolution<1 || resolution >= 256) {
		fprintf(stderr, "illegal resolution (%i): accepted range is 0 < resolution < 256\n", resolution);
		exit(EXIT_FAILURE);
	}
}

void check_mosaik2database_name(char *name) {
	if(strlen(name)<1) {
		fprintf(stderr, "mosaik2 database name too short\n");
		exit(EXIT_FAILURE);
	}
	if( strlen(name)>=128) {
		fprintf(stderr, "mosaik2 database name too long\n");
		exit(EXIT_FAILURE);
	}
	
}

size_t write_data(void *ptr, size_t size, size_t nmemb, m2file stream) {
    size_t written;
    written = fwrite(ptr, size, nmemb, stream);
    return written;
}

int mosaik2_indextask_read_image(/*mosaik2_database *md,*/ mosaik2_indextask *task) {
	if(is_file_local( task->filename )) {
		struct stat st;
		m_stat(task->filename, &st);
		task->filesize = st.st_size;
		task->lastmodified = st.st_mtim.tv_sec;
		m2file file = m_fopen( task->filename, "rb");
		unsigned char *buf = m_malloc(task->filesize);
		m_fread(buf,task->filesize,file);
		task->image_data = buf;
		m_fclose(file);
		
	} else {
#ifdef HAVE_LIBCURL
		CURL *curl;
		CURLcode res;
		//TODO the use of `tmpnam' is dangerous, better use `mkstemp'
		m2name tmpfilename = tmpnam(NULL); 
		m2file tmpfile = m_fopen(tmpfilename,"w+b");

		curl = curl_easy_init();
		char errbuf[CURL_ERROR_SIZE];
		errbuf[0] = 0;
		if(curl) {
			curl_easy_setopt(curl, CURLOPT_USERAGENT, "mosaik2");
			curl_easy_setopt(curl, CURLOPT_URL, task->filename);
			curl_easy_setopt(curl, CURLOPT_VERBOSE, 0L);
			curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 1L);
			curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
			curl_easy_setopt(curl, CURLOPT_WRITEDATA, tmpfile);
			curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, errbuf);

			res = curl_easy_perform(curl);
			if(res != CURLE_OK) {
				size_t len = strlen(errbuf);
				fprintf(stderr, "libcurl: (%d) ", res);
				if(len)
					fprintf(stderr, "%s%s", errbuf,
							((errbuf[len - 1] != '\n') ? "\n" : ""));
				else
					fprintf(stderr, "%s\n", curl_easy_strerror(res));
				exit(EXIT_FAILURE);
  			}
			struct stat st;
			char* filename = get_file_name(tmpfile);
			m_stat(filename, &st);
			m_free((void**)&filename);
			task->filesize = st.st_size;
			task->lastmodified = st.st_mtim.tv_sec;
			m_fseeko(tmpfile, 0, SEEK_SET);
			task->image_data = m_malloc(task->filesize);
			m_fread(task->image_data, task->filesize, tmpfile);
			m_fclose(tmpfile);
			curl_easy_cleanup(curl);
		}
#else
		fprintf(stderr, "mosaik2 was compiled without curl support, no downloads are possible, only loading images from the local filesystem\n");
		exit(EXIT_FAILURE);
#endif
		
	}
	return 0;
}
/*void mosai2_indextask_deconst(mosaik2_indextask *task) {
	
}*/


//returned pointer must be freed
__attribute__((malloc)) unsigned char* read_stdin( size_t *file_size) {

	unsigned char *buf0[BUFSIZ];
	unsigned char *buf = malloc(BUFSIZ);
	(*file_size) = 0;

	if(!buf) {
    	fprintf(stderr,"memory could not be allocated for primary image data\n");
    	exit(EXIT_FAILURE);
  	}
	int i=1;
	while(1) {
  	size_t bytes_read = fread(buf0, 1, BUFSIZ, stdin);
		size_t new_file_size = (*file_size) + bytes_read;
		if((buf = realloc(buf, new_file_size))==NULL || errno == ENOMEM) {
  			m_free((void**)&buf);
    		fprintf(stderr, "image could not be loaded bytes_should:%li, bytes_read:%li\n", (*file_size), bytes_read);
    		exit(EXIT_FAILURE);
		}
		i++;
		
		memcpy(buf+(*file_size),buf0,bytes_read);
		(*file_size) = new_file_size;
		if(bytes_read<BUFSIZ) 
			break;
	}
	return buf;
}

void mosaik2_project_read_exclude_area(mosaik2_project *mp, mosaik2_tile_infos *ti, mosaik2_arguments *args) {

	mp->exclude_count = args->exclude_count;
	m2area *areas = (m2area *) m_malloc(mp->exclude_count *sizeof(m2area));
	mp->exclude_area = areas;
	char **area_string = args->exclude_area;


	for(m2elem i=0;i<mp->exclude_count;i++ ) {

		if (sscanf(area_string[i], "%u,%u,%u,%u", &(areas->start_x), &(areas->start_y),
				&(areas->end_x), &(areas->end_y)) == 4) {

			if (areas->start_x > areas->end_x || areas->start_y > areas->end_y) {
				fprintf(stderr, "invalid exclude area range[%s], have a look at the man page\n",area_string[i]);
				exit(EXIT_FAILURE);
			}
			if(	       areas->end_x < areas->start_x
					|| areas->end_y < areas->start_y
					|| areas->start_x > ti->primary_tile_x_count
					|| areas->start_y > ti->primary_tile_y_count
					|| areas->end_x > ti->primary_tile_x_count
					|| areas->end_y > ti->primary_tile_y_count ) {
				fprintf(stderr, "exclude area is out of range [%s] "
						"( possible exclude range x between 0 and %u and y between 0 and %u )\n",
						area_string[i], ti->primary_tile_x_count,ti->primary_tile_y_count);
				/*fprintf(stderr, "%i %i %i %i %i %i %i %i\n", areas->start_x < 0
					, areas->start_y < 0
					, areas->end_x < areas->start_x
					, areas->end_y < areas->start_y
					, areas->start_x > ti->primary_tile_x_count
					, areas->start_y > ti->primary_tile_y_count
					, areas->end_x > ti->primary_tile_x_count
					, areas->end_y > ti->primary_tile_y_count);*/
				exit(EXIT_FAILURE);
			}
			/*fprintf(stderr, "exlucde %i:%i (%i-%i)*(%i-%i)\n", i,(areas->end_x-areas->start_x)*(areas->end_y-areas->start_y),
					areas->end_x,areas->start_x,areas->end_y,areas->start_y);*/
		} else {
			fprintf(stderr, "invalid exclude area format [%s]\n",
					area_string[i]);
			exit(EXIT_FAILURE);
		}
		areas++;
	}
}


mosaik2_project_result *mosaik2_project_read_result(mosaik2_project *mp, mosaik2_database *md, int total_primary_tile_count) {

      
	mosaik2_project_result *results = m_calloc(total_primary_tile_count, sizeof( mosaik2_project_result ));

	for(int i=0;i<total_primary_tile_count;i++) {
		results[i].costs=FLT_MAX;
	}

	fprintf(stderr,"load result file %s\n", mp->dest_result_filename);

	m2file file = m_fopen(mp->dest_result_filename, "rb");
		
	off_t filesize = get_file_size(mp->dest_result_filename);
	char buf[filesize+1];
	memset(buf, 0, filesize+1);

	m_fread(buf, filesize, file);
	
	char *ptr = NULL;
	ptr = strtok(buf, "\n\t"); // ignore first column with line index
	assert(ptr!=NULL);

	for(int j=0;j<total_primary_tile_count;j++) {	
		ptr = strtok(NULL, "\n\t"); assert(ptr!=NULL); // ignore first column with line index
		results[j].index = atoll( ptr ); 	ptr = strtok(NULL, "\n\t"); assert(ptr != NULL);
		results[j].costs = (float) atof(ptr);	ptr = strtok(NULL, "\n\t"); assert(ptr != NULL);
		results[j].off_x = atoi( ptr );		ptr = strtok(NULL, "\n\t"); assert(ptr != NULL);
		results[j].off_y = atoi( ptr );		ptr = strtok(NULL, "\n\t"); assert(ptr != NULL || (ptr == NULL && j == total_primary_tile_count - 1));
		results[j].md = md;		
	}

	m_fclose(file);
	return results;

}

//over all valid and croppd elements
void mosaik2_database_read_histogram(mosaik2_database *md) {
	m2elem element_count = mosaik2_database_read_element_count(md);
	uint32_t valid_count = mosaik2_database_read_valid_count(md);
	float valid_count_f = (float)valid_count;

	memset(md->histogram_color, 0, sizeof(md->histogram_color));

	if(valid_count == 0)
	{
		return; // histogram_colors set to 0
	}


	m2file tiledims_file = m_fopen(md->tiledims_filename, "r");
	m2file imagecolors_file = m_fopen(md->imagecolors_filename, "r");
	m2file tileoffsets_file = m_fopen(md->tileoffsets_filename, "r");
	m2file duplicates_file = m_fopen(md->duplicates_filename, "r");
	m2file invalid_file = m_fopen(md->invalid_filename, "r");

	m2rezo database_image_resolution = mosaik2_database_read_image_resolution(md);
	unsigned char tiledims[] = {0,0};
	
	float histogram_color0[RGB];

	unsigned char duplicates0 = 0;
	unsigned char invalid0 = 0;

	memset(histogram_color0, 0, sizeof(histogram_color0));

	unsigned char imagecolors[256*256*RGB];
	unsigned char tileoffsets[2];
	
	for(uint32_t i=0;i<element_count;i++) {

		memset(histogram_color0, 0, sizeof(histogram_color0));

		m_fread(&tiledims, sizeof(tiledims), tiledims_file);
		float tilesize = (float) tiledims[0]*tiledims[1];

		m_fread(&duplicates0, sizeof(char), duplicates_file);
		if(duplicates0 != 0) {
			m_fseeko(imagecolors_file, tilesize*RGB, SEEK_CUR);
			m_fseeko(tileoffsets_file, 2*sizeof(char), SEEK_CUR);
			m_fseeko(invalid_file, sizeof(char), SEEK_CUR);
			continue;
		}
		m_fread(&invalid0, sizeof(char), invalid_file);
		if(invalid0 != 0) {
			m_fseeko(imagecolors_file, tilesize*RGB, SEEK_CUR);
			m_fseeko(tileoffsets_file, 2*sizeof(char), SEEK_CUR);
			continue;
		}

		m_fread(imagecolors, tilesize*RGB, imagecolors_file);
		m_fread(tileoffsets, 2*sizeof(char), tileoffsets_file);

		int x0,y0,xl,yl;
		if(tileoffsets[0] == 0xFF && tileoffsets[1] == 0xFF ) { // then undefined
			x0 = 0;
			y0 = 0;
			xl = tiledims[0];
			yl = tiledims[1];
		} else { // recognize only cropped area
			x0 = tileoffsets[0];
			y0 = tileoffsets[1];
			xl = x0 + database_image_resolution;
			yl = y0 + database_image_resolution;
			tilesize = database_image_resolution * database_image_resolution; // only the cropped square
		}
		for(int x=x0;x<xl;x++) {
			for(int y=y0;y<yl;y++) {
				int idx = (x * tiledims[1] + y) * RGB;
				
				histogram_color0[R] += imagecolors[idx+R] / tilesize;
				histogram_color0[G] += imagecolors[idx+G] / tilesize;
				histogram_color0[B] += imagecolors[idx+B] / tilesize;

			}
		}
		md->histogram_color[R] += histogram_color0[R] / valid_count_f;
		md->histogram_color[G] += histogram_color0[G] / valid_count_f;
		md->histogram_color[B] += histogram_color0[B] / valid_count_f;

	}
	
	m_fclose( imagecolors_file );
	m_fclose( tileoffsets_file );
	m_fclose( duplicates_file );
	m_fclose( invalid_file );
       
}

void mosaik2_database_touch_lastmodified(mosaik2_database *md) {
	m2file lastmodified_file = m_fopen(md->lastmodified_filename, "w");
	m2time now = time(NULL);
	m_fwrite(&now, md->lastmodified_sizeof, lastmodified_file);
	m_fclose(lastmodified_file);
}

//include <sys/timeb.h>
/*void print_usage(char *m) {
	struct timeb tb;
	ftime(&tb);
    struct rusage resuage;
      getrusage( RUSAGE_SELF, &resuage);
    fprintf(stderr, "%i %li.%03i %-10s usert:%li.%06li syst:%li.%06li  max:%6li ix:%li, id:%li, is:%li\n",
      getpid(), tb.time, tb.millitm,
			m, resuage.ru_utime.tv_sec, resuage.ru_utime.tv_usec,
      resuage.ru_stime.tv_sec, resuage.ru_stime.tv_usec,
      resuage.ru_maxrss, resuage.ru_ixrss, resuage.ru_idrss, resuage.ru_isrss
  );
}*/


void read_entry(m2name filename, void *val, size_t val_len, off_t file_offset)  {
	m2file f = m_fopen(filename, "r");
	m_fseeko(f, file_offset, SEEK_SET);
	m_fread(val, val_len, f);
	m_fclose(f);
}

// assumes, that file is opened in the right mode
void read_file_entry(m2file file, void *val, size_t val_len, off_t file_offset) {
	m_fseeko(file, file_offset, SEEK_SET);
	m_fread(val, val_len, file);	
}

void write_entry(m2name filename, void *val, size_t val_len, off_t file_offset)  {
	m2file f = m_fopen(filename, "r+");
	m_fseeko(f, file_offset, SEEK_SET);
	m_fwrite(val, val_len, f);
	m_fclose(f);
}

// assumes that file is open in the right mode
void write_file_entry(m2file file, void *val, size_t val_len, off_t file_offset) {
	m_fseeko(file, file_offset, SEEK_SET);
	m_fwrite(val, val_len, file);
}

m2file m_fopen(m2name filename, char *mode) {
	m2file file = NULL;
	file = fopen(filename, mode);
	if( file == NULL) {
		fprintf(stderr, "m_fopen: file (%s) could not be opened in mode (%s)\n", filename,mode);
		perror(NULL);
		exit(EXIT_FAILURE);
	}
	return file;
}

m2fd m_open(const char *pathname, int flags, mode_t mode) {
	m2fd fd = open(pathname, flags, mode);
	if(fd == -1) {
		fprintf(stderr, "m_open: file (%s) could not be opened\n", pathname);
		perror("error:");
		exit(EXIT_FAILURE);
	}
	return fd;
}

void m_close(m2fd fd) {
	int val = close(fd);
	if(val == -1 )  {
		fprintf(stderr, "fd (%i) could not be closed\n", fd);
		perror("error:");
	}
}

void m_fclose(m2file file) {
	int val = fclose(file);
	if(val != 0) {
		fprintf(stderr, "could not close file\n");
		perror(NULL);
		exit(EXIT_FAILURE);
	}
}
void m_fread(void *buf, size_t nmemb, m2file stream) {
	m2off tell = m_ftello(stream);

	//size_t fread(void *ptr, size_t size, size_t nmemb, FILE *stream);
	size_t nmemb_read = fread(buf, 1, nmemb, stream);
	if(nmemb != nmemb_read) {
		fprintf(stderr, "m_fread: read (%li) bytes did not match the (%li) amount of data at position %li \n", nmemb_read, nmemb, tell);
		char* filename = get_file_name(stream);
		fprintf(stderr, "filename: %s\n", filename);
		m_free((void**)&filename);
		exit(EXIT_FAILURE);
	}
}

void m_fwrite(const void *ptr, size_t nmemb, m2file stream) {
	//TODO correct in error case
	m2off tell = m_ftello(stream);
	size_t nmemb_written = fwrite(ptr, 1, nmemb, stream);
	if(nmemb != nmemb_written) {
		fprintf(stderr, "m_fwrite: could not (%li) write the expected (%li) amount of data at position %li\n", nmemb_written, nmemb, tell);
		exit(EXIT_FAILURE);
	}
}

int m_fseeko(m2file stream, off_t offset, int whence) {
	int val = fseeko(stream, offset, whence);
	if(val!=0) {
		fprintf(stderr, "m_fseeko: cannot seek to %li\n", offset);
		perror(NULL);
		exit(EXIT_FAILURE);
	}
	return val;
}

m2off m_ftello(FILE *stream) {
	m2off val = ftello(stream);
	if(val==-1) {
		fprintf(stderr, "m_ftello failed\n");
		perror(NULL);
		exit(EXIT_FAILURE);
	}
	return val;
}

__attribute__((malloc)) void *m_malloc(size_t size) {
	void *buf = malloc(size);
	if(buf == NULL) {
		fprintf(stderr, "could not allocate memory\n");
		exit(EXIT_FAILURE);
	}
	return buf;
}

__attribute__((malloc)) void *m_calloc(size_t nmemb, size_t size) {
	void *buf = calloc(nmemb, size);
	if(buf == NULL) {
		fprintf(stderr, "could not allocate memory: %lix %li bytes\n", nmemb, size);
		exit(EXIT_FAILURE);
	}
	return buf;
}

void m_free(void **ptr) {
	free(*ptr);
	*ptr = NULL; // null out possible use after free
}

int m_fflush(m2file stream) {
	int val = fflush(stream);
	if(val!=0) {
		fprintf(stderr, "m_fflush: could not flush\n");
		perror(NULL);
		exit(EXIT_FAILURE);
	}
	return val;
}

void m_stat(const char *pathname, struct stat *statbuf) {
	int val = stat(pathname, statbuf);
	if(val != 0) {
		fprintf(stderr, "m_stat: could not stat file (%s)\n", pathname);
		perror(NULL);
		exit(EXIT_FAILURE);
	}
}
void m_fstat(m2fd fd, struct stat *statbuf) {
	int val = fstat(fd, statbuf);
	if(val != 0) {
		char *filename = get_fd_name(fd);
		fprintf(stderr, "m_fstat: could not stat fd");
		fprintf(stderr, " (%s)\n", filename);
		perror(NULL);
		m_free((void**)&filename);
		exit(EXIT_FAILURE);
	}
}

void *m_mmap(void *addr, size_t length, int prot, int flags, int fd, off_t offset) {
	void *ptr = mmap(addr, length, prot, flags, fd, offset);
	if(ptr == MAP_FAILED ) {
		char *filename = get_fd_name(fd);
		fprintf(stderr, "memory mapped file (%s) failed\n", filename);
		perror(NULL);
		m_free((void**)&filename);
		exit(EXIT_FAILURE);
	}
	return ptr;
}

void m_munmap(void *addr, size_t length) {
	int val = munmap(addr, length);
	if(val==-1) {
		perror("unmap memory mapped file failed");
		exit(EXIT_FAILURE);
	}
}

m2file m_tmpfile(void) {
	m2file f = tmpfile();
	if(f == NULL) {
		fprintf(stderr, "m_tmpfile: could not create temporary file\n");
		perror(NULL);
		exit(EXIT_FAILURE);
	}
	return f;
}

void m_flock(m2fd fd, int operation) {
	int val = flock(fd, operation);
	if(val!=0) {
		fprintf(stderr, "error applying (%i) file lock for fd (%i)\n", operation, fd);
		perror(NULL);
		exit(EXIT_FAILURE);
	}
}

m2file m_fdopen(int fd, const char *mode) {
	m2file f = fdopen(fd, mode);
	if( f == NULL) {
		perror("fdopen failed\n");
		exit(EXIT_FAILURE);
	}
	return f;
}


int m_sysinfo(struct sysinfo *info) {
	int val = sysinfo(info);
	if( val != 0) {
		fprintf(stderr, "m_sysinfo: could not call sysinfo\n");
		perror(NULL);
		exit(EXIT_FAILURE);
	}
	return val;
}

void m_access(const char* pathname, int mode) {
	int val = access(pathname, mode);
	if(val != 0) {
		fprintf(stderr, "m_access: cannot access file (%s) with mode: %i\n", pathname, mode);
		perror(NULL);
		exit(EXIT_FAILURE);
	}
}
/* like fgets, but removes newline from string */
m2text m_fgets(m2rtext s, int size, m2file stream) {
	m2text val = fgets(s, size, stream);
	if(val == NULL) {
		fprintf(stderr, "m_fgets: read less data from stream than expected (%i)", size);
		exit(EXIT_FAILURE);
	}
	size_t buflen = strlen(s);
	if(buflen>0 && s[buflen-1]=='\n') {
		s[buflen-1]=0;
	}
	return s;
}


/* Max-Heap and Min-Heap from https://de.wikibooks.org/wiki/Algorithmen_und_Datenstrukturen_in_C/_Heaps under CC BY-SA 3.0 */
/* Adaption: Element types are changed from int to mosaik2_database_candidate -------------------------------------------- */

void swap(Heap* h, uint32_t n, uint32_t m) {
   mosaik2_database_candidate tmp;
  memcpy(&tmp, &h->keys[n], sizeof(mosaik2_database_candidate));
  memcpy(&h->keys[n], &h->keys[m], sizeof(mosaik2_database_candidate));
  memcpy(&h->keys[m], &tmp, sizeof(mosaik2_database_candidate));
}

void heap_init(Heap* h, mosaik2_database_candidate* storage) {
   h->last = 0;
   h->keys = storage - 1;
}

void heap_dump(Heap *h) {
	printf("dump heap count:%i\n", h->last-1);
	for(uint32_t i=1;i<h->last+1;i++) {
		printf(" %i tidx:%u cidx:%u c:%f off:%i %i\n", i-1, h->keys[i].primary_tile_idx, h->keys[i].candidate_idx, h->keys[i].costs, h->keys[i].off_x, h->keys[i].off_y);
	}
}
void mdc_dump(mosaik2_database_candidate *mdc0) {
	printf("   tidx:%u cidx:%u c:%f off:%i %i\n", mdc0->primary_tile_idx, mdc0->candidate_idx, mdc0->costs, mdc0->off_x, mdc0->off_y);
}

void max_heap_bubble_up(Heap* h, uint32_t n) {
   uint32_t parent;

   while(n > 1) {
      parent = n/2;
      if (h->keys[parent].costs > h->keys[n].costs)
         break;
      swap(h, parent, n);
      n = parent;
   }
}

void max_heap_insert(Heap* h, mosaik2_database_candidate *key) {
   h->last += 1;
   h->count++;
   memcpy(&(h->keys[h->last]), key, sizeof(mosaik2_database_candidate));


   max_heap_bubble_up(h, h->last);
}

void max_heap_sift_down(Heap* h, uint32_t n) {
   uint32_t last = h->last;

   while (1) {
      uint32_t max   = n;
      uint32_t left  = n * 2;
      uint32_t right = left + 1;

      if (left <= last && h->keys[left].costs > h->keys[max].costs)
         max = left;
      if (right <= last && h->keys[right].costs > h->keys[max].costs)
         max = right;

      if (n == max)
         break;

      swap(h, max, n);
      n = max;
   }
}

int max_heap_delete(Heap* h, uint32_t n, mosaik2_database_candidate *d) {
   h->count--;

   if(d!=NULL)
	memcpy(d, &h->keys[n], sizeof(mosaik2_database_candidate));
   h->keys[n] = h->keys[h->last];
   h->last -= 1;

   if (n >= h->last)
	   return 0;

   if (n > 1 && h->keys[n].costs > h->keys[n/2].costs)
      max_heap_bubble_up(h, n);
   else
      max_heap_sift_down(h, n);

   return 0;
}

int max_heap_pop(Heap* h, mosaik2_database_candidate *d) {
   return max_heap_delete(h, 1, d);
}
int max_heap_peek(Heap *h, mosaik2_database_candidate *d){
	if(h->last > 0) {
		memcpy(d, &h->keys[1], sizeof(mosaik2_database_candidate));
		return 0;
	} 
	return 1;
}

void min_heap_bubble_up(Heap* h, uint32_t n) {
   uint32_t parent;

   while(n > 1) {
      parent = n/2;
      if (h->keys[parent].costs < h->keys[n].costs)
         break;
      swap(h, parent, n);
      n = parent;
   }
}

void min_heap_insert(Heap* h, mosaik2_database_candidate *key) {
   h->last += 1;
   h->count++;
   memcpy(&h->keys[h->last], key, sizeof(mosaik2_database_candidate));

   min_heap_bubble_up(h, h->last);
}

void min_heap_sift_down(Heap* h, uint32_t n) {
   uint32_t last = h->last;

   while (1) {
      uint32_t min   = n;
      uint32_t left  = n * 2;
      uint32_t right = left + 1;

      if (left <= last && h->keys[left].costs < h->keys[min].costs)
         min = left;
      if (right <= last && h->keys[right].costs < h->keys[min].costs)
         min = right;

      if (n == min)
         break;

      swap(h, min, n);
      n = min;
   }
}

int min_heap_delete(Heap* h, uint32_t n, mosaik2_database_candidate *d) {
   h->count--;

   memcpy(d, &(h->keys[n]), sizeof(mosaik2_database_candidate));
   memcpy(&(h->keys[n]), &(h->keys[h->last]), sizeof(mosaik2_database_candidate));
   h->last -= 1;

   if (n >= h->last) {
   	return 0;
   }


   if (n > 1 && h->keys[n].costs < h->keys[n/2].costs) {
      min_heap_bubble_up(h, n); 
   } else {
      min_heap_sift_down(h, n);
   }

   return 0;
}

int min_heap_pop(Heap* h, mosaik2_database_candidate *d) {
   return min_heap_delete(h, 1, d);
}
int min_heap_peek(Heap *h, mosaik2_database_candidate *d) {
	if(h->last>0) {
		memcpy(d, &h->keys[1], sizeof(mosaik2_database_candidate));
		return 0;
	}
	return 1;
}
/* END: Max-Heap and Min-Heap from https://de.wikibooks.org/wiki/Algorithmen_und_Datenstrukturen_in_C/_Heaps under CC BY-SA 3.0 */

m2fd m_fileno(m2file file) {
	int fno = fileno(file);
	if(fno == -1) {
		fprintf(stderr, "could not retrieve fd from FILE*\n");
		perror(NULL);
		exit(EXIT_FAILURE);
	}
	return fno;
}
/* pointer has to be freed*/
char* get_file_name(m2file file) {
	m2fd fd = m_fileno(file);
	return get_fd_name(fd);
}
/* pointer has to be freed*/
__attribute__((malloc)) char * get_fd_name(m2fd fd) {
	char* filename =  m_malloc(0xFFF);
	int MAXSIZE = 0xFFF;
	ssize_t r;
	char proclnk[0xFFF];
	sprintf(proclnk, "/proc/self/fd/%d", fd);

	r = readlink(proclnk, filename, MAXSIZE);
	if (r < 0) {
		perror("failed to readlink");

		exit(EXIT_FAILURE);
	}
	filename[r] = '\0';
	return filename;
}

//make sure, output is large enough, will raise an error if its to small
void quote_string(const char* input, int output_len, char* output) {
    int len = strlen(input);
    int quote_count = 0;

    for (int i = 0; i < len; i++) {
        if (input[i] == '"') {
            quote_count++;
        }
    }

	if (strchr(input, ',') || strchr(input, '\n')) {

        int new_len = len + 2 + quote_count;
		if(new_len > output_len) {
			fprintf(stderr,"quote_string: output string is too small\n");
			exit(EXIT_FAILURE);
		}
        int j = 0;

        output[j++] = '"';

        for (int i = 0; i < len; i++) {
            if (input[i] == '"') {
                output[j++] = '"';
                output[j++] = '"';
            } else {
                output[j++] = input[i];
            }
        }

        output[j++] = '"';
        output[j] = '\0';
    } else {
        strcpy(output, input);
    }
}

void unset_file_buf(FILE *stream) {
	m_setvbuf(stream, NULL, _IONBF, 0); // disable bufing 
}
int m_setvbuf(FILE *stream, char *buf, int mode, size_t size) {
	int val = setvbuf(stream, buf, mode, size);
	if( val != 0 ) {
		perror("setvbuf failed");
	}
	return val;
}

void _concat(char *dest, size_t dlen, ...) {
	assert(dlen > 0);

    va_list args;
    va_start(args, dlen);

    dest[0] = '\0';  
    size_t used = 0;

    const char *s;
    while ((s = va_arg(args, const char *)) != NULL) {
        size_t slen = strlen(s); //exnull

        if (used + slen >= dlen) { // >= because strcat has to add a nullbyte after string of slen
            fprintf(stderr,
                    "concat: string buffer too small\n");
            exit(EXIT_FAILURE);
        }

        strcat(dest, s); //cat s = nullbyte
        used += slen;
    }

    va_end(args);
}

void u8_to_f32(const uint8_t* src, float* dst, size_t count) {
    for (size_t i = 0; i < count; ++i) {
        dst[i] = (float)src[i];
    }
}