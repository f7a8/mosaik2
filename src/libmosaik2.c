#include "libmosaik2.h"


const int FT_JPEG = 0;
const int FT_PNG = 1;
const int FT_ERR = -1;
uint8_t ORIENTATION_TOP_LEFT=0;
uint8_t ORIENTATION_RIGHT_TOP=1;
uint8_t ORIENTATION_BOTTOM_RIGHT=2;
uint8_t ORIENTATION_LEFT_BOTTOM=3;

#ifdef HAVE_PHASH
const int PHASHES_VALID = 1;
const int PHASHES_INVALID = 0;

const int IS_PHASH_DUPLICATE = 2;
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

void mosaik2_database_init(mosaik2_database *md, m2name thumbs_db_name) {

	memset( (*md).thumbs_db_name,0,256);
	memset( (*md).imagecolors_filename,0,256);
	memset( (*md).imagestddev_filename,0,256);
	memset( (*md).imagedims_filename,0,256);
	memset( md->image_index_filename,0,256);
	memset( (*md).filenames_filename,0,256);
	memset( (*md).filenames_index_filename,0,256);
	memset( (*md).filehashes_filename,0,256);
	memset( (*md).filehashes_index_filename,0,256);
	memset( (*md).timestamps_filename,0,256);
	memset( (*md).filesizes_filename,0,256);
	memset( (*md).tiledims_filename,0,256);
	memset( (*md).invalid_filename,0,256);
	memset( (*md).duplicates_filename,0,256);
	memset( (*md).database_image_resolution_filename,0,256);
	memset( md->database_image_resolution_filename, 0, 256);
	memset( md->id_filename, 0, 256);
	memset( md->id, 0, 17);
	memset( md->version_filename, 0, 256);
	memset( md->readme_filename, 0, 256);
	memset( md->pid_filename, 0, 256);
	memset( md->lock_filename, 0, 256);
	memset( md->lastmodified_filename, 0, 256);
	memset( md->tileoffsets_filename, 0, 256);
	memset( md->lastindexed_filename, 0, 256);
	memset( md->createdat_filename, 0,256);
	memset( md->phash_filename, 0,256);

	size_t l = strlen(thumbs_db_name);
	strncpy( (*md).thumbs_db_name,thumbs_db_name,l);

	strncpy( (*md).imagecolors_filename,thumbs_db_name,l);
	strcat( (*md).imagecolors_filename, "/imagecolors.bin");

	strncpy( (*md).imagestddev_filename,thumbs_db_name,l);
	strcat( (*md).imagestddev_filename,"/imagestddev.bin");

	strncpy( (*md).imagedims_filename,thumbs_db_name,l);
	strcat( (*md).imagedims_filename,"/imagedims.bin");

	strncpy( (*md).image_index_filename,thumbs_db_name,l);
	strcat( (*md).image_index_filename,"/image.idx");

	strncpy( (*md).filenames_filename,thumbs_db_name,l);
	strcat( (*md).filenames_filename,"/filenames.txt");

	strncpy( (*md).filenames_index_filename,thumbs_db_name,l);
	strcat( (*md).filenames_index_filename,"/filenames.idx");

	strncpy( (*md).filehashes_filename,thumbs_db_name,l);
	strcat( (*md).filehashes_filename,"/filehashes.bin");

	strncpy( (*md).filehashes_index_filename,thumbs_db_name,l);
	strcat( (*md).filehashes_index_filename,"/filehashes.idx");

	strncpy( (*md).timestamps_filename,thumbs_db_name,l);
	strcat( (*md).timestamps_filename,"/timestamps.bin");

	strncpy( (*md).filesizes_filename,thumbs_db_name,l);
	strcat( (*md).filesizes_filename,"/filesizes.bin");

	strncpy( (*md).tiledims_filename,thumbs_db_name,l);
	strcat( (*md).tiledims_filename,"/tiledims.bin");

	strncpy( (*md).invalid_filename,thumbs_db_name,l);
	strcat( (*md).invalid_filename,"/invalid.bin");

	strncpy( (*md).duplicates_filename,thumbs_db_name,l);
	strcat( (*md).duplicates_filename,"/duplicates.bin");

	strncpy( (*md).database_image_resolution_filename,thumbs_db_name,l);
	strcat( (*md).database_image_resolution_filename,"/database_image_resolution.bin");

	strncpy( (*md).id_filename,thumbs_db_name,l);
	strcat( (*md).id_filename,"/id.txt");

	(*md).id_len = 16;

	strncpy( md->version_filename,thumbs_db_name,l);
	strcat( md->version_filename,"/dbversion.txt");

	strncpy( md->readme_filename,thumbs_db_name,l);
	strcat( md->readme_filename,"/README.txt");

	strncpy( md->pid_filename,thumbs_db_name,l);
	strcat( md->pid_filename,"/mosaik2.pid");

	strncpy( md->lock_filename, thumbs_db_name, l);
	strcat( md->lock_filename, "/.lock");

	strncpy( md->lastmodified_filename, thumbs_db_name, l);
	strcat( md->lastmodified_filename, "/.lastmodified");

	strncpy( md->tileoffsets_filename, thumbs_db_name, l);
	strcat( md->tileoffsets_filename, "/tileoffsets.bin");

	strncpy( md->lastindexed_filename, thumbs_db_name, l);
	strcat( md->lastindexed_filename, "/.lastindexed");

	strncpy( md->createdat_filename, thumbs_db_name, l);
	strcat( md->createdat_filename, "/.createdat");

	strncpy( md->phash_filename, thumbs_db_name, l);
	strcat( md->phash_filename, "/phash.bin");

	md->imagestddev_sizeof = 3;
	md->imagecolors_sizeof = 3;
	md->imagedims_sizeof = 2*sizeof(int);
	md->image_index_sizeof = sizeof(off_t);
	md->filenames_index_sizeof = sizeof(off_t);
	md->filehashes_sizeof = MD5_DIGEST_LENGTH;
	md->filehashes_index_sizeof = MD5_DIGEST_LENGTH + sizeof(size_t);
	md->timestamps_sizeof = sizeof(time_t);
	md->filesizes_sizeof = sizeof(size_t);
	md->tiledims_sizeof = 2*sizeof(char);
	md->invalid_sizeof = sizeof(char);
	md->duplicates_sizeof = sizeof(char);
	md->tileoffsets_sizeof = 2*sizeof(char);
	md->lastmodified_sizeof = sizeof(time_t);
	md->lastindexed_sizeof = sizeof(time_t);
	md->createdat_sizeof = sizeof(time_t);
	md->phash_sizeof = sizeof(unsigned long long) + sizeof(int);
}

void mosaik2_project_init(mosaik2_project *mp, m2ctext mosaik2_database_id, m2name dest_filename) {
	size_t mosaik2_database_id_len = strlen(mosaik2_database_id);
	size_t dest_filename_len = strlen(dest_filename);


	strncpy(mp->dest_filename, dest_filename, dest_filename_len);

	char *thumbs_db_ending=".mtileres";
	size_t thumbs_db_ending_len = strlen(thumbs_db_ending);

	memset(mp->dest_primarytiledims_filename, 0, 256);
	strncpy(mp->dest_primarytiledims_filename, mp->dest_filename, dest_filename_len);
	strcat(mp->dest_primarytiledims_filename, ".");
	strncat(mp->dest_primarytiledims_filename, mosaik2_database_id, mosaik2_database_id_len);
	strncat(mp->dest_primarytiledims_filename, thumbs_db_ending, thumbs_db_ending_len);

	memset(mp->dest_imagedims_filename, 0, 256);
	strncpy(mp->dest_imagedims_filename, mp->dest_filename, dest_filename_len);
	strcat(mp->dest_imagedims_filename, ".imagedims.txt");

 	memset(mp->dest_result_filename, 0, 256);
	thumbs_db_ending=".result";
	thumbs_db_ending_len = strlen(thumbs_db_ending);
	strncpy(mp->dest_result_filename, mp->dest_filename, dest_filename_len);
	strcat(mp->dest_result_filename, ".");
	strncat(mp->dest_result_filename, mosaik2_database_id, mosaik2_database_id_len);
	strncat(mp->dest_result_filename, thumbs_db_ending, thumbs_db_ending_len);

	memset(mp->dest_html_filename, 0, 256);
	strncpy(mp->dest_html_filename, mp->dest_filename, dest_filename_len);
	strcat(mp->dest_html_filename, ".html");

	memset(mp->dest_src_filename, 0, 256);
	strncpy(mp->dest_src_filename, mp->dest_filename, dest_filename_len);
	strcat(mp->dest_src_filename, ".src");
}

void mosaik2_tile_infos_init(mosaik2_tile_infos *ti, int database_image_resolution, int src_image_resolution, int image_width, int image_height) {

	assert(src_image_resolution > 0);
	assert(database_image_resolution > 0 && database_image_resolution < UINT8_MAX);

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

void mosaik2_tiler_infos_init(mosaik2_tile_infos *ti, int database_image_resolution, int image_width, int image_height) {

	ti->database_image_resolution = database_image_resolution; // old:thumbs_tile_count

	assert(database_image_resolution > 0 && database_image_resolution < UINT8_MAX);

	if(database_image_resolution*database_image_resolution*(6*256) > UINT32_MAX) {
		fprintf(stderr, "database_image_resolution too high for internal data structure\n");
		exit(EXIT_FAILURE);
	}

	ti->image_width = image_width;
	ti->image_height = image_height;

	if(ti->image_width < database_image_resolution || ti->image_height < database_image_resolution) {
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

int is_file_local( const m2name filename ) {
	if(!filename) {
		fprintf(stderr, "no filename\n");
		exit(EXIT_FAILURE);
	}
	//size_t len_filename = strlen(filename);
	if( StartsWith("https://", filename ) || StartsWith("http://", filename))
		return 0;
	return 1;
}
int is_file_wikimedia_commons( const m2name filename ) {
	if(!filename) {
		fprintf(stderr, "no filename\n");
		exit(EXIT_FAILURE);
	}
	if( StartsWith("https://upload.wikimedia.org/", filename )) {
		return 1;
	}
	return 0;
}

void get_wikimedia_thumb_url(const char *url, char *thumb_pixel, char *dest, int dest_len) {
	if(!url) {
		fprintf(stderr, "no url\n");
		exit(EXIT_FAILURE);
	}

	

	size_t url_len = strlen( url );
	m2name filename = strrchr(url, '/');
	size_t filename_len = strlen(filename);
	if(filename_len < 2) {
		fprintf(stderr,"nothing found behind SLASH in url/\n");
		exit(EXIT_FAILURE);
	}

	filename++;
	filename_len--;

	memset(dest, '\0', dest_len);
	char *destr = dest;
	//sprintf(dest, "%s/%dpx-%s", url, thumb_pixel, filename);
	strncpy(destr, url, 47);
	destr += 47; // rolling

	strncpy(destr, "thumb", 5);
	destr+=5;
	
	strncpy(destr, url+46, url_len-46);
	destr+=url_len-46;

	strncpy(destr, "/", 1);
	destr++;

	strncpy(destr, thumb_pixel, strlen(thumb_pixel));
	destr+= strlen(thumb_pixel);

	strncpy(destr, "px-", 3);
	destr += 3;

	strncpy(destr, filename, filename_len);

//	fprintf(stderr, "get_wikimedia_thumb_url %s @ %ipx %i %i %i\n", dest, thumb_pixel, strlen(dest), dest_len);	
}

void get_wikimedia_file_url(const char *url, char *dest, int dest_len) {

	if(!url) {
		fprintf(stderr, "no url\n");
		exit(EXIT_FAILURE);
	}

					fprintf(stderr, "1\n");
	//size_t url_len = strlen(url);
	m2name filename = strrchr(url, '/');
	size_t filename_len = strlen(filename);

					fprintf(stderr, "2\n");
	if(filename_len < 2) {
		fprintf(stderr,"nothing found behind SLASH in url\n");
	}

					fprintf(stderr, "3\n");
	filename++;//ignore leading slash
	filename_len--;

					fprintf(stderr, "4\n");
	const char *commons_prefix="https://commons.wikimedia.org/wiki/File:";
	const int commons_prefix_len = strlen( commons_prefix );
					fprintf(stderr, "5\n");
	memset(dest, '\0', dest_len);
					fprintf(stderr, "6\n");
	//strncpy(dest, commons_prefix, commons_prefix_len);
	strcpy(dest,"[[File:");
					fprintf(stderr, "7\n");
					fprintf(stderr, "dest %s common_prefix_len %i filename %s filename_len %lu dest_len %i \n", dest, commons_prefix_len, filename, filename_len, dest_len);
//	sprintf(dest + commons_prefix_len, filename, filename_len); 
	strncpy(dest + 7, filename, filename_len);
	strcpy(dest + 7 + filename_len, "|50px]]");
					fprintf(stderr, "8\n");
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
 
int get_file_type(const m2name dest_filename) {
	if(EndsWith(dest_filename, "jpeg") || EndsWith(dest_filename, "jpg") )
		return FT_JPEG;
	if(EndsWith(dest_filename, "png"))
		return FT_PNG;
	return FT_ERR;
}

int get_file_type_from_buf(uint8_t *buf, size_t len) {
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

uint8_t mosaik2_database_read_image_resolution(mosaik2_database *md) {
	m2file database_image_resolution_file = m_fopen(md->database_image_resolution_filename, "rb");
	int32_t database_image_resolution=0;
	m_fread(&database_image_resolution, sizeof(database_image_resolution), database_image_resolution_file);

	if(database_image_resolution <= 0 || database_image_resolution > UINT8_MAX) {
		fprintf(stderr, "illegal image resolution\n");
		exit(EXIT_FAILURE);
	}
	m_fclose( database_image_resolution_file );
	return database_image_resolution;
}

m2elem mosaik2_database_read_duplicates_count(mosaik2_database *md) {
	m2file file = m_fopen(md->duplicates_filename, "rb");
	
	m2elem duplicates_count = 0;
	uint8_t buf[BUFSIZ];
	while( feof(file) == 0) {
		size_t s = fread(&buf, 1, BUFSIZ, file);
		for(int i=0;i<s;i++) {
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
		for(int i=0;i<s;i++) {
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
	for(int i=0;i<s;i++) {
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
		for(int i=0;i<s;i+=2) {
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
	read_entry(md->filehashes_filename, mde->hash, MD5_DIGEST_LENGTH, element_number*MD5_DIGEST_LENGTH);
	mde->filename = mosaik2_database_read_element_filename(md, element_number, NULL);
	read_entry(md->filesizes_filename, &mde->filesize, sizeof(ssize_t), element_number*sizeof(ssize_t));
	read_entry(md->imagedims_filename, &mde->imagedims, sizeof(int)*2, element_number*2*sizeof(int));
	read_entry(md->tiledims_filename, &mde->tiledims, sizeof(unsigned char)*2, element_number*2*sizeof(unsigned char));
	read_entry(md->timestamps_filename, &mde->timestamp, sizeof(time_t), element_number*sizeof(time_t));
	read_entry(md->duplicates_filename, &mde->duplicate, 1, element_number * 1);
	read_entry(md->invalid_filename, &mde->invalid, 1, element_number);
	read_entry(md->tileoffsets_filename, &mde->tileoffsets, sizeof(unsigned char)*2, element_number*2*sizeof(unsigned char));

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
	unsigned char imagestddev[total_tile_count*RGB];
	read_entry(md->imagecolors_filename, &imagecolors, total_tile_count*RGB, imagecolors_offset);
	read_entry(md->imagestddev_filename, &imagestddev, total_tile_count*RGB, imagecolors_offset);

	//fprintf(stderr, "###x0:%i y0:%i xl:%i yl:%i tiledims:%i %i tile_count_f:%f\n", x0, y0, xl, yl, mde->tiledims[0], mde->tiledims[1], total_tile_count_f);
	for(int x=x0;x<xl;x++) {
		for(int y=y0;y<yl;y++) {
			int idx = (x * mde->tiledims[1] + y) * RGB;
			mde->histogram_color[R] += imagecolors[idx+R] / total_tile_count_f;
			mde->histogram_color[G] += imagecolors[idx+G] / total_tile_count_f;
			mde->histogram_color[B] += imagecolors[idx+B] / total_tile_count_f;
			mde->histogram_stddev[R] += imagestddev[idx+R] / total_tile_count_f;
			mde->histogram_stddev[G] += imagestddev[idx+G] / total_tile_count_f;
			mde->histogram_stddev[B] += imagestddev[idx+B] / total_tile_count_f;
		}
	}
}
float mosaik2_database_costs(mosaik2_database *md, mosaik2_database_element *mde) {
	double diff_c = sqrt(
			  pow(md->histogram_color[R] - mde->histogram_color[R], 2)
			+ pow(md->histogram_color[G] - mde->histogram_color[G], 2)
			+ pow(md->histogram_color[B] - mde->histogram_color[B], 2)
			);
	double diff_s = sqrt(
			  pow(md->histogram_stddev[R] - mde->histogram_stddev[R], 2)
			+ pow(md->histogram_stddev[G] - mde->histogram_stddev[G], 2)
			+ pow(md->histogram_stddev[B] - mde->histogram_stddev[B], 2)
			);
	float image_ratio = 1.0;
	float stddev_ratio = 0.0;

	return image_ratio * diff_c + stddev_ratio * diff_s;
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

	int fd = m_open(md->filenames_filename, O_RDONLY, 0);
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
 * if FILE* is null its opend and closed
 * the returned char pointer has to freed manually.
 */
char *mosaik2_database_read_element_filename(mosaik2_database *md, int element_number, m2file filenames_index_file) {
	off_t offsets[2];
	int fileobject_was_null = filenames_index_file == NULL;
	// find starting offset from filenames index file for real filenames file
	if(fileobject_was_null)
		filenames_index_file = m_fopen(md->filenames_index_filename, "r");

	m_fseeko(filenames_index_file, element_number*sizeof(off_t), SEEK_SET);
	size_t read = fread(offsets, sizeof(off_t), 2, filenames_index_file);
	if(read == 0) {
		fprintf(stderr, "could not read filenames offset from filenames index file\n");
		exit(EXIT_FAILURE);
	}
	if(read == 1) { // this was the last entry in the file. so there is no next entry
		offsets[1] = get_file_size(md->filenames_filename);
		fprintf(stderr, "next offset from filesize\n");
	}
	if(fileobject_was_null)
		m_fclose(filenames_index_file);

	offsets[1]--;
	off_t filename_len = offsets[1]-offsets[0];

	m2name filename = m_calloc(1,filename_len+1);
	read_entry(md->filenames_filename, filename, filename_len, offsets[0]);
	return filename;
}

void mosaik2_project_read_primary_tile_dims(mosaik2_project *mp) {
	m2name filename = mp->dest_primarytiledims_filename;

	m2file file = m_fopen(filename, "r");
	off_t filesize = get_file_size(filename);
	char buf[filesize];
	memset(buf, 0, filesize);

	m_fread(buf, filesize, file);

  	char *ptr = NULL;
	ptr=strtok(buf,"\n\t");
	assert(ptr!=NULL);
	mp->primary_tile_x_count = atoi( ptr );

	ptr=NULL;
	ptr=strtok(NULL,"\n");
	assert(ptr!=NULL);
	mp->primary_tile_y_count = atoi( ptr );

	m_fclose(file);
}

size_t mosaik2_database_read_size(mosaik2_database *md) {
	return (size_t)
    ( get_file_size( md->imagestddev_filename)
		+ get_file_size( md->imagecolors_filename)
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
	m_access( mp->dest_primarytiledims_filename, F_OK);
	m_access( mp->dest_result_filename, F_OK);
	m_access( mp->dest_imagedims_filename, F_OK);
}

void mosaik2_database_check(mosaik2_database *md) {

	//check_thumbs_db_name( md->thumbs_db_name );
	m_access( md->thumbs_db_name, F_OK );
	m_access( md->imagecolors_filename, F_OK );
	m_access( md->imagestddev_filename, F_OK );
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
	m_access( md->lock_filename, F_OK);
	m_access( md->lastmodified_filename, F_OK) ;
	m_access( md->tileoffsets_filename, F_OK);

	// TODO make more plause checks
	m2elem element_count = mosaik2_database_read_element_count(md);
	md->element_count = element_count;
	uint8_t database_image_resolution = mosaik2_database_read_image_resolution(md);

	assert(get_file_size(md->imagecolors_filename)     >= element_count * md->imagecolors_sizeof*database_image_resolution*database_image_resolution);

	assert(get_file_size(md->imagestddev_filename)     >= element_count * md->imagestddev_sizeof*database_image_resolution*database_image_resolution);
	assert(get_file_size(md->imagecolors_filename)     <= element_count * md->imagecolors_sizeof*256*256);
	assert(get_file_size(md->imagestddev_filename)     <= element_count * md->imagestddev_sizeof*256*256);

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
	assert(get_file_size(md->lastindexed_filename)    == (element_count > 0 ? md->lastindexed_sizeof : 0));
	assert(get_file_size(md->tileoffsets_filename)     == element_count * md->tileoffsets_sizeof);
}

int  mosaik2_project_check_dest_filename(m2name dest_filename) {
	
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
	int ft;
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





#ifdef HAVE_EXIF
uint8_t get_image_orientation(unsigned char *buffer, size_t buf_size) {

	ExifData *ed;
	ExifEntry *entry;

  /* Load an ExifData object from an EXIF m2file */
	ed = exif_data_new_from_data(buffer, buf_size);
  if (ed==NULL) {
		printf("unable to create exif data\n");
		exit(EXIT_FAILURE);
	}

	entry = exif_content_get_entry(ed->ifd[EXIF_IFD_0], EXIF_TAG_ORIENTATION);
	if (entry) {
		char buf[64];
		if (exif_entry_get_value(entry, buf, sizeof(buf))) {
    	trim_spaces(buf);


      if (strcmp(buf, "Right-top")==0) {
//	exif_data_free(ed);
//			exif_entry_free(entry);
				return ORIENTATION_RIGHT_TOP;
			} else if(strcmp(buf, "Bottom-right")==0) {
//	exif_data_free(ed);
	//		exif_entry_free(entry);
				return ORIENTATION_BOTTOM_RIGHT;
			} else if(strcmp(buf, "Left-Bottom")==0) {
	//exif_data_free(ed);
		//	exif_entry_free(entry);
				return ORIENTATION_LEFT_BOTTOM;
			}
    }
	}
	//exif_data_free(ed);
	//exif_entry_free(entry);
	return ORIENTATION_TOP_LEFT;
}
#else
uint8_t get_image_orientation(unsigned char *buffer, size_t buf_size) {
	return ORIENTATION_TOP_LEFT;
}
#endif


gdImagePtr read_image_from_file(m2name filename) {
	m2file in = m_fopen(filename, "rb");
	size_t file_size = get_file_size(filename);
	unsigned char *buf = m_calloc(1, file_size);
	m_fread(buf, file_size, in);
	gdImagePtr im = read_image_from_buf(buf, file_size);
	free(buf);
	m_fclose(in);
	return im;
}

gdImagePtr read_image_from_buf(unsigned char *buf, size_t file_size) {
   gdImagePtr im;
   int file_type = get_file_type_from_buf(buf, file_size);
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
	
	uint8_t orientation = get_image_orientation(buf, file_size);
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

void check_resolution(uint32_t resolution) {
	if(resolution<1 || resolution >= 256) {
		fprintf(stderr, "illegal resolution (%i): accepted range is 0 < resolution < 256\n", resolution);
		exit(EXIT_FAILURE);
	}
}

size_t write_data(void *ptr, size_t size, size_t nmemb, m2file stream) {
	fprintf(stderr, "write_data\n");
    size_t written;
    written = fwrite(ptr, size, nmemb, stream);
    return written;
}

int mosaik2_indextask_read_image(mosaik2_indextask *task) {
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
#ifdef HAVE_CURL
		fprintf(stderr, "only reading of local files is currently implemented\n");
		exit(1);
#else
		fprintf(stderr, "only reading of local files is currently implemented\n");
		exit(1);
#endif
		/*CURL *curl;
		CURLcode res;
		char filename[100];
		snprintf(filename,100,"download_temp_file_%i",task->idx); 
		m2file tmpfile =m_fopen(filename, "w+");
		curl = curl_easy_init();
		if(curl) {
			curl_easy_setopt(curl, CURLOPT_URL, task->filename);
			curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
			curl_easy_setopt(curl, CURLOPT_WRITEDATA, tmpfile);
			res = curl_easy_perform(curl);
			struct stat st;
			m_stat(get_file_name(tmpfile), &st);
			task->filesize = st.st_size;
			task->lastmodified = st.st_mtim.tv_sec;
			m_fseeko(tmpfile, 0, SEEK_SET);
			task->image_data = m_malloc(task->filesize);
			m_fread(task->image_data, task->filesize, tmpfile);
			m_fclose(tmpfile);
			curl_easy_cleanup(curl);
		}*/
	}
	return 0;
}
void mosai2_indextask_deconst(mosaik2_indextask *task) {
	
}

void mosaik2_tile_image(mosaik2_tile_infos *ti, gdImagePtr *im, double *colors, double *stddev) {
	
}

unsigned char* read_stdin( size_t *file_size) {

	unsigned char *buffer0[BUFSIZ];
  unsigned char *buffer = malloc(BUFSIZ);
	(*file_size) = 0;

  if(!buffer) {
    fprintf(stderr,"memory could not be allocated for primary image data\n");
    exit(EXIT_FAILURE);
  }
	int i=1;
	while(1) {
  	size_t bytes_read = fread(buffer0, 1, BUFSIZ, stdin);
		size_t new_file_size = (*file_size) + bytes_read;
		if((buffer = realloc(buffer, new_file_size))==NULL || errno == ENOMEM) {
  		free( buffer );
    	fprintf(stderr, "image could not be loaded bytes_should:%li, bytes_read:%li\n", (*file_size), bytes_read);
    	exit(EXIT_FAILURE);
		}
		i++;
		
		memcpy(buffer+(*file_size),buffer0,bytes_read);
		(*file_size) = new_file_size;
		if(bytes_read<BUFSIZ) 
			break;
	}
	return buffer;
}

	//works only with images generated by ld TODO
	//read the image size from the internal jpeg data
	//It is not necessary to load a complete image into memory for this.
void mosaik2_project_read_image_dims(mosaik2_project *mp) {
	mp->image_height = 0;
	mp->image_width = 0;

	off_t filesize = get_file_size(mp->dest_imagedims_filename);
	m2file file = NULL;
	file = m_fopen(mp->dest_imagedims_filename, "r");
	assert( file != NULL);

	char buf[filesize];
	memset(buf, 0, filesize);
	m_fread(buf, filesize, file);

	char *ptr = NULL;
	ptr=strtok(buf,"\t");
	assert(ptr!=NULL);
	mp->image_width = atoi( ptr );

	ptr=strtok(NULL,"\t");
	assert(ptr!=NULL);
	mp->image_height = atoi( ptr );

	ptr=NULL;
	ptr=strtok(NULL,"\t\n");
	assert(ptr!=NULL);
	mp->pixel_per_tile = atoi( ptr );
	mp->primary_tile_x_count = mp->image_width / mp->pixel_per_tile;
	mp->primary_tile_y_count = mp->image_height / mp->pixel_per_tile;
	assert( !(mp->image_height == 0 && mp->image_width == 0 && mp->pixel_per_tile == 0));

	m_fclose(file);
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
	memset(md->histogram_stddev, 0, sizeof(md->histogram_stddev));

	if(valid_count == 0)
	{
		return; // histogram_colors and ..stddev set to 0
	}


	m2file tiledims_file = m_fopen(md->tiledims_filename, "r");
	m2file imagecolors_file = m_fopen(md->imagecolors_filename, "r");
	m2file imagestddev_file = m_fopen(md->imagestddev_filename, "r");
	m2file tileoffsets_file = m_fopen(md->tileoffsets_filename, "r");
	m2file duplicates_file = m_fopen(md->duplicates_filename, "r");
	m2file invalid_file = m_fopen(md->invalid_filename, "r");

	uint8_t database_image_resolution = mosaik2_database_read_image_resolution(md);
	unsigned char tiledims[] = {0,0};
	
	float histogram_color0[RGB];
	float histogram_stddev0[RGB];

	unsigned char duplicates0 = 0;
	unsigned char invalid0 = 0;

	memset(histogram_color0, 0, sizeof(histogram_color0));
	memset(histogram_stddev0, 0, sizeof(histogram_stddev0));

	unsigned char imagecolors[256*256*RGB];
	unsigned char imagestddev[256*256*RGB];
	unsigned char tileoffsets[2];
	
	for(uint32_t i=0;i<element_count;i++) {

		memset(histogram_color0, 0, sizeof(histogram_color0));
		memset(histogram_stddev0, 0, sizeof(histogram_stddev0));

		m_fread(&tiledims, sizeof(tiledims), tiledims_file);
		float tilesize = (float) tiledims[0]*tiledims[1];

		m_fread(&duplicates0, sizeof(char), duplicates_file);
		if(duplicates0 != 0) {
			m_fseeko(imagecolors_file, tilesize*RGB, SEEK_CUR);
			m_fseeko(imagestddev_file, tilesize*RGB, SEEK_CUR);
			m_fseeko(tileoffsets_file, 2*sizeof(char), SEEK_CUR);
			m_fseeko(invalid_file, sizeof(char), SEEK_CUR);
			continue;
		}
		m_fread(&invalid0, sizeof(char), invalid_file);
		if(invalid0 != 0) {
			m_fseeko(imagecolors_file, tilesize*RGB, SEEK_CUR);
			m_fseeko(imagestddev_file, tilesize*RGB, SEEK_CUR);
			m_fseeko(tileoffsets_file, 2*sizeof(char), SEEK_CUR);
			continue;
		}

		m_fread(imagecolors, tilesize*RGB, imagecolors_file);
		m_fread(imagestddev, tilesize*RGB, imagestddev_file);
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
				histogram_stddev0[R] += imagestddev[idx+R] / tilesize;
				histogram_stddev0[G] += imagestddev[idx+G] / tilesize;
				histogram_stddev0[B] += imagestddev[idx+B] / tilesize;

			}
		}
		md->histogram_color[R] += histogram_color0[R] / valid_count_f;
		md->histogram_color[G] += histogram_color0[G] / valid_count_f;
		md->histogram_color[B] += histogram_color0[B] / valid_count_f;
		md->histogram_stddev[R] += histogram_stddev0[R] / valid_count_f;
		md->histogram_stddev[G] += histogram_stddev0[G] / valid_count_f;
		md->histogram_stddev[B] += histogram_stddev0[B] / valid_count_f;

	}
	
	m_fclose( imagecolors_file );
	m_fclose( imagestddev_file );
	m_fclose( tileoffsets_file );
	m_fclose( duplicates_file );
	m_fclose( invalid_file );
       
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
		fprintf(stderr, "file (%s) could not be opened\n", filename);
		exit(EXIT_FAILURE);
	}
	return file;
}

int m_open(const char *pathname, int flags, mode_t mode) {
	int fd = open(pathname, flags, mode);
	if(fd == -1) {
		fprintf(stderr, "file (%s) could not be opened\n", pathname);
		perror("error:");
		exit(EXIT_FAILURE);
	}
	return fd;
}


void m_close(int fd) {
	int cal = close(fd);
	if(cal == -1 )  {
		fprintf(stderr, "file could not be closed\n");
		perror("error:");
	}
}

void m_fclose(m2file file) {
	int val = fclose(file);
	if(val != 0) {
		fprintf(stderr, "could not close file\n");
		perror("error");
		exit(EXIT_FAILURE);
	}
}
void m_fread(void *buf, size_t nmemb, m2file stream) {
	size_t bytes_read = fread(buf, 1, nmemb, stream);
	if(nmemb != bytes_read) {
		fprintf(stderr, "m_fread: could not (%li) read the expected (%li) amount of data at position %li \n", bytes_read, nmemb, ftello(stream));
		char* filename = get_file_name(stream);
		fprintf(stderr, "filename: %s\n", filename);
		free(filename);
		exit(EXIT_FAILURE);
	}
}

void m_fwrite(const void *ptr, size_t nmemb, m2file stream) {
	size_t bytes_written = fwrite(ptr, 1, nmemb, stream);
	if(nmemb != bytes_written) {
		fprintf(stderr, "m_fwrite: could not (%li) write the expected (%li) amount of data at position %li\n", bytes_written, nmemb, ftello(stream));
		exit(EXIT_FAILURE);
	}
}

int m_fseeko(m2file stream, off_t offset, int whence) {
	int val = fseeko(stream, offset, whence);
	if(val!=0) {
		fprintf(stderr, "m_fseeko: cannot seek to %li\n", offset);
		perror("error");
		exit(EXIT_FAILURE);
	}
	return val;
}

void *m_malloc(size_t size) {
	void *buf = malloc(size);
	if(buf == NULL) {
		fprintf(stderr, "could not allocate memory\n");
		exit(EXIT_FAILURE);
	}
	return buf;
}

void *m_calloc(size_t nmemb, size_t size) {
	void *buf = calloc(nmemb, size);
	if(buf == NULL) {
		fprintf(stderr, "could not allocate memory: %lix %li bytes\n", nmemb, size);
		exit(EXIT_FAILURE);
	}
	return buf;
}

int m_fflush(m2file stream) {
	int val = fflush(stream);
	if(val!=0) {
		fprintf(stderr, "m_fflush: could not flush\n");
		perror("error");
		exit(EXIT_FAILURE);
	}
	return val;
}

void m_stat(const char *pathname, struct stat *statbuf) {
	int val = stat(pathname, statbuf);
	if(val != 0) {
		fprintf(stderr, "m_stat: could not stat file (%s)\n", pathname);
		perror("error");
		exit(EXIT_FAILURE);
	}
}
void m_fstat(int fd, struct stat *statbuf) {
	int val = fstat(fd, statbuf);
	if(val != 0) {
		fprintf(stderr, "m_fstat: could not stat fd");
		fprintf(stderr, " (%s)\n", get_fd_name(fd));
		perror("error");
		exit(EXIT_FAILURE);
	}
}

void *m_mmap(void *addr, size_t length, int prot, int flags, int fd, off_t offset) {
	void *ptr = mmap(addr, length, prot, flags, fd, offset);
	if(ptr == MAP_FAILED ) {
		fprintf(stderr, "memory mapped file (%s) failed\n", get_fd_name(fd));
		perror("error");
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
		perror("tmpfile() failed");
		exit(EXIT_FAILURE);
	}
	return f;
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
		perror("error");
		exit(EXIT_FAILURE);
	}
	return val;
}

void m_access(m2ctext pathname, int mode) {
	int val = access(pathname, mode);
	if(val != 0) {
		fprintf(stderr, "m_access: cannot access file (%s) with mode: %i\n", pathname, mode);
		perror("error");
		exit(EXIT_FAILURE);
	}
}

m2rtext m_fgets(m2rtext s, int size, m2file stream) {
	char* val = fgets(s, size, stream);
	if(val == NULL) {
		fprintf(stderr, "m_fgets: read less data from stream than expected (%i)", size);
		exit(EXIT_FAILURE);
	}
	return s;
}


/* Max-Heap and Min-Heap from https://de.wikibooks.org/wiki/Algorithmen_und_Datenstrukturen_in_C/_Heaps under CC BY-SA 3.0 */
/* Adaption: Element types are changed from int to mosaik2_database_candidate -------------------------------------------- */

void swap(Heap* h, int n, int m) {
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

void max_heap_bubble_up(Heap* h, int n) {
   int parent;

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

void max_heap_sift_down(Heap* h, int n) {
   int last = h->last;

   while (1) {
      int max   = n;
      int left  = n * 2;
      int right = left + 1;

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

int max_heap_delete(Heap* h, int n, mosaik2_database_candidate *d) {
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

void min_heap_bubble_up(Heap* h, int n) {
   int parent;

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

void min_heap_sift_down(Heap* h, int n) {
   int last = h->last;

   while (1) {
      int min   = n;
      int left  = n * 2;
      int right = left + 1;

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

int min_heap_delete(Heap* h, int n, mosaik2_database_candidate *d) {
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
	if(&h->last>0) {
		memcpy(d, &h->keys[1], sizeof(mosaik2_database_candidate));
		return 0;
	}
	return 1;
}
/* END: Max-Heap and Min-Heap from https://de.wikibooks.org/wiki/Algorithmen_und_Datenstrukturen_in_C/_Heaps under CC BY-SA 3.0 */

char* get_file_name(m2file file) {
	int fno = fileno(file);
	if(fno == -1) {
		fprintf(stderr, "could not retrieve fd from FILE*\n");
		exit(EXIT_FAILURE);
	}
	return get_fd_name( fileno(file));
}

char * get_fd_name(int fd) {
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

