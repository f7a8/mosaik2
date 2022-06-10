#include "libmosaik2.h"

const int FT_JPEG = 0;
const int FT_PNG = 1;
const int FT_ERR = -1;
uint8_t ORIENTATION_TOP_LEFT=0;
uint8_t ORIENTATION_RIGHT_TOP=1;
uint8_t ORIENTATION_BOTTOM_RIGHT=2;
uint8_t ORIENTATION_LEFT_BOTTOM=3;

void init_mosaik2_context(mosaik2_context *ctx) {
	memset(ctx, 0, sizeof(mosaik2_context));
	memset(ctx->pids, 0, 1024*sizeof(pid_t));
	char *env_mosaik2_debug = getenv("MOSAIK2_DEBUG");
	if(env_mosaik2_debug != NULL) {
		ctx->debug = strncmp("1", getenv("MOSAIK2_DEBUG"), 1) == 0;
	}
}

void init_mosaik2_database(mosaik2_database *md, char *thumbs_db_name) {

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
	memset( (*md).tilecount_filename,0,256);
	memset( md->tilecount_filename, 0, 256);
	memset( md->id_filename, 0, 256);
	memset( md->id,0, 14);
	memset( md->version_filename, 0, 256);
	memset( md->readme_filename, 0, 256);
	memset( md->pid_filename, 0, 256);
	memset( md->lock_filename, 0, 256);
	memset( md->lastmodified_filename, 0, 256);

	size_t l = strlen(thumbs_db_name);
	strncpy( (*md).thumbs_db_name,thumbs_db_name,l);

	strncpy( (*md).imagecolors_filename,thumbs_db_name,l);
	strncat( (*md).imagecolors_filename, "/imagecolors.bin",16);

	strncpy( (*md).imagestddev_filename,thumbs_db_name,l);
	strncat( (*md).imagestddev_filename,"/imagestddev.bin",16);

	strncpy( (*md).imagedims_filename,thumbs_db_name,l);
	strncat( (*md).imagedims_filename,"/imagedims.bin",14);

	strncpy( (*md).image_index_filename,thumbs_db_name,l);
	strncat( (*md).image_index_filename,"/image.idx",10);

	strncpy( (*md).filenames_filename,thumbs_db_name,l);
	strncat( (*md).filenames_filename,"/filenames.txt",14);

	strncpy( (*md).filenames_index_filename,thumbs_db_name,l);
	strncat( (*md).filenames_index_filename,"/filenames.idx",14);

	strncpy( (*md).filehashes_filename,thumbs_db_name,l);
	strncat( (*md).filehashes_filename,"/filehashes.bin",15);

	strncpy( (*md).filehashes_index_filename,thumbs_db_name,l);
	strncat( (*md).filehashes_index_filename,"/filehashes.idx",15);

	strncpy( (*md).timestamps_filename,thumbs_db_name,l);
	strncat( (*md).timestamps_filename,"/timestamps.bin",15);

	strncpy( (*md).filesizes_filename,thumbs_db_name,l);
	strncat( (*md).filesizes_filename,"/",1);
	strncat( (*md).filesizes_filename,"filesizes.bin",13);

	strncpy( (*md).tiledims_filename,thumbs_db_name,l);
	strncat( (*md).tiledims_filename,"/",1);
	strncat( (*md).tiledims_filename,"tiledims.bin",12);

	strncpy( (*md).invalid_filename,thumbs_db_name,l);
	strncat( (*md).invalid_filename,"/",1);
	strncat( (*md).invalid_filename,"invalid.bin",11);
	
	strncpy( (*md).duplicates_filename,thumbs_db_name,l);
	strncat( (*md).duplicates_filename,"/duplicates.bin",15);

	strncpy( (*md).tilecount_filename,thumbs_db_name,l);
	strncat( (*md).tilecount_filename,"/tilecount.txt",14);

	strncpy( (*md).id_filename,thumbs_db_name,l);
	strncat( (*md).id_filename,"/id.txt",7);

	(*md).id_len = 14;

	strncpy( md->version_filename,thumbs_db_name,l);
	strncat( md->version_filename,"/dbversion.txt",14);

	strncpy( md->readme_filename,thumbs_db_name,l);
	strncat( md->readme_filename,"/README.txt",11);

	strncpy( md->pid_filename,thumbs_db_name,l);
	strncat( md->pid_filename,"/mosaik2.pid",12);

	strncpy( md->lock_filename, thumbs_db_name, l);
	strncat( md->lock_filename, "/.lock", 6);

	strncpy( md->lastmodified_filename, thumbs_db_name, l);
	strncat( md->lastmodified_filename, "/.lastmodified", 14); 
}

void init_mosaik2_project(mosaik2_project *mp, char *mosaik2_database_id, char *dest_filename) {

	size_t mosaik2_database_id_len = strlen(mosaik2_database_id);
	size_t dest_filename_len = strlen(dest_filename);

	strncpy(mp->dest_filename, dest_filename, dest_filename_len);
	
	char *thumbs_db_ending=".mtileres";
	size_t thumbs_db_ending_len = strlen(thumbs_db_ending);

	memset(mp->dest_primarytiledims_filename, 0, 256);
	strncpy(mp->dest_primarytiledims_filename, mp->dest_filename, dest_filename_len);
	strncat(mp->dest_primarytiledims_filename, ".", 1);
	strncat(mp->dest_primarytiledims_filename, mosaik2_database_id, mosaik2_database_id_len);
	strncat(mp->dest_primarytiledims_filename, thumbs_db_ending, thumbs_db_ending_len);
 
 	memset(mp->dest_result_filename, 0, 256);
	thumbs_db_ending=".result";
	thumbs_db_ending_len = strlen(thumbs_db_ending);
	strncpy(mp->dest_result_filename, mp->dest_filename, dest_filename_len);
	strncat(mp->dest_result_filename, ".", 1);
	strncat(mp->dest_result_filename, mosaik2_database_id, mosaik2_database_id_len);
	strncat(mp->dest_result_filename, thumbs_db_ending, thumbs_db_ending_len);

	memset(mp->dest_html_filename, 0, 256);
	strncpy(mp->dest_html_filename, mp->dest_filename, dest_filename_len);
	strncat(mp->dest_html_filename, ".html", 5);
	
	memset(mp->dest_src_filename, 0, 256);
	strncpy(mp->dest_src_filename, mp->dest_filename, dest_filename_len);
	strncat(mp->dest_src_filename, ".src", 4);
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

int is_file_local( const char *filename ) {
	if(!filename) {
		fprintf(stderr, "no filename\n");
		exit(EXIT_FAILURE);
	}
	//size_t len_filename = strlen(filename);
	if( StartsWith("https://", filename ) || StartsWith("http://", filename))
		return 0;
	return 1;
}
int is_file_wikimedia_commons( const char *filename ) {
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
	char *filename = strrchr(url, '/');
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
	char *filename = strrchr(url, '/');
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
	strncpy(dest,"[[File:", 7);
					fprintf(stderr, "7\n");
					fprintf(stderr, "dest %s common_prefix_len %i filename %s filename_len %lu dest_len %i \n", dest, commons_prefix_len, filename, filename_len, dest_len);
//	sprintf(dest + commons_prefix_len, filename, filename_len); 
	strncpy(dest + 7, filename, filename_len);
	strncpy(dest + 7 + filename_len, "|50px]]", 7);
					fprintf(stderr, "8\n");
}

off_t get_file_size(const char *filename) {
  struct stat st;
  if( stat(filename, &st) != 0) {
		fprintf(stderr, "error cannot gather file size from (%s)\n", filename);
		exit(EXIT_FAILURE);
	}
	off_t size = st.st_size;
	return size;
}
 
int get_file_type(const char *dest_filename) {
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

uint64_t read_thumbs_db_count(mosaik2_database *md) {

	off_t db_filesizes_size = get_file_size(md->filesizes_filename);
	if(db_filesizes_size == 0)
		return 0;
	// the *.db.filesizes is a binary file that includes the original 
	// filesizes of the indexed images and its saved as 4 byte integers
	return db_filesizes_size / sizeof(size_t);
}

uint8_t read_thumbs_conf_tilecount(mosaik2_database *md) {

    FILE *thumbs_conf_tilecount_file = fopen(md->tilecount_filename, "rb");
    if( thumbs_conf_tilecount_file == NULL) {
      fprintf(stderr, "thumbs db file with tile count (%s) could not be opened\n", md->tilecount_filename);
      exit(EXIT_FAILURE);
    }
    char buf[4];
    char *rbuf = fgets( buf, 4, thumbs_conf_tilecount_file );
		if(rbuf==NULL) {
			fprintf(stderr, "thumbs db file (%s) could not be read correctly\n", md->tilecount_filename);
			fclose( thumbs_conf_tilecount_file );
			exit(EXIT_FAILURE);
		}

    uint8_t thumbs_conf_tilecount = atoi(buf);
    fclose( thumbs_conf_tilecount_file );
		return thumbs_conf_tilecount;
}

void read_database_id(mosaik2_database *md) {

	FILE *id_file = fopen(md->id_filename,"r");
	if(id_file == NULL ) {
		fprintf(stderr, "mosaik2 database file with id (%s) could not be opened\n", md->id_filename);
		exit(EXIT_FAILURE);
	}
	char *rbuf = fgets( md->id, md->id_len, id_file);
	if(rbuf == NULL ) {
		fprintf(stderr, "mosaik2 datbase file with id (%s) could not be read correctly\n", md->id_filename);
		fclose(id_file);
		exit(EXIT_FAILURE);
	}

	fclose( id_file );
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

void check_thumbs_db(mosaik2_database *md) {

	//check_thumbs_db_name( md->thumbs_db_name );

	
	if( access( md->thumbs_db_name, F_OK ) != 0 ) {
		fprintf(stderr, "mosaik2 database directory (%s) is not accessable\n", md->thumbs_db_name);
		exit(EXIT_FAILURE);
	}

	if( access( md->imagecolors_filename, F_OK ) != 0 ) {
		fprintf(stderr, "mosaik2 database file (%s) is not accessable\n", md->imagecolors_filename);
		exit(EXIT_FAILURE);
	}

	if( access( md->imagestddev_filename, F_OK ) != 0 ) {
		fprintf(stderr, "mosaik2 database file (%s) is not accessable\n", md->imagestddev_filename);
		exit(EXIT_FAILURE);
	}

	if( access( md->imagedims_filename, F_OK ) != 0 ) {
		fprintf(stderr, "mosaik2 database file (%s) is not accessable\n", md->imagedims_filename);
		exit(EXIT_FAILURE);
	}

	if( access( md->image_index_filename, F_OK ) != 0 ) {
		fprintf(stderr, "mosaik2 database file (%s) is not accessable\n", md->image_index_filename);
		exit(EXIT_FAILURE);
	}

	if( access( md->filenames_filename, F_OK ) != 0 ) {
		fprintf(stderr, "mosaik2 database file (%s) is not accessable\n", md->filenames_filename);
		exit(EXIT_FAILURE);
	}
	
	if( access( md->filenames_index_filename, F_OK ) != 0 ) {
		fprintf(stderr, "mosaik2 database file (%s) is not accessable\n", md->filenames_index_filename);
		exit(EXIT_FAILURE);
	}
	
	if( access( md->filesizes_filename, F_OK ) != 0 ) {
		fprintf(stderr, "mosaik2 database file (%s) is not accessable\n", md->filesizes_filename);
		exit(EXIT_FAILURE);
	}
	
	if( access( md->filehashes_filename, F_OK ) != 0 ) {
		fprintf(stderr, "mosaik2 database file (%s) is not accessable\n", md->filehashes_filename);
		exit(EXIT_FAILURE);
	}
	
	if( access( md->filehashes_index_filename, F_OK ) != 0 ) {
		fprintf(stderr, "mosaik2 database file (%s) is not accessable\n", md->filehashes_index_filename);
		exit(EXIT_FAILURE);
	}
	
	if( access( md->tiledims_filename, F_OK ) != 0 ) {
		fprintf(stderr, "mosaik2 database file (%s) is not accessable\n", md->tiledims_filename);
		exit(EXIT_FAILURE);
	}
	
	if( access( md->invalid_filename, F_OK ) != 0 ) {
		fprintf(stderr, "mosaik2 database file (%s) is not accessable\n", md->invalid_filename);
		exit(EXIT_FAILURE);
	}

	if( access( md->duplicates_filename, F_OK ) != 0 ) {
		fprintf(stderr, "mosaik2 database file (%s) is not accessable\n", md->duplicates_filename);
	}

	if( access( md->tilecount_filename, F_OK ) != 0 ) {
		fprintf(stderr, "mosaik2 database file (%s) is not accessable\n", md->tilecount_filename);
		exit(EXIT_FAILURE);
	}

	if( access( md->lock_filename, F_OK ) != 0) {
		fprintf(stderr, "mosaik2 database file (%s) is not accessable\n", md->lock_filename);
		exit(EXIT_FAILURE);
	}

	if( access( md->lastmodified_filename, F_OK ) != 0) {
		fprintf(stderr, "mosaik2 database file (%s) is not accessable\n", md->lastmodified_filename);
		exit(EXIT_FAILURE);
	}

	// TODO make more plause checks
	uint64_t element_count = read_thumbs_db_count(md);
	if( get_file_size(md->filenames_index_filename) != element_count*sizeof(long)) {
		fprintf(stderr, "mosaik2 database file (%s) has not the expected size of:%li\n", md->filenames_index_filename, element_count*sizeof(long));
		exit(EXIT_FAILURE);
	}

	if( get_file_size(md->image_index_filename) != element_count*sizeof(long)) {
		fprintf(stderr, "mosaik2 database file (%s) has not the expected size:%li\n", md->image_index_filename, element_count*sizeof(long));
		exit(EXIT_FAILURE);
	}


	
}

int check_dest_filename(char *dest_filename) {
	
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

void check_thumbs_tile_count(uint32_t thumbs_tile_count) {
  if(thumbs_tile_count*thumbs_tile_count*(6*256)>UINT32_MAX) {
    //can candidates_score contain the badest possible score?
    fprintf(stderr, "thumb tile size too high for internal data structure\n");
    exit(EXIT_FAILURE);
  }
}

void remove_newline(char *str) {
	size_t l = strlen(str);
	if(l>0 && str[l-1]=='\n')
		str[l-1]='\0';
}

int cmpfunc (const void * a, const void * b) {
	//fprintf(stderr,".");
	struct result *a0 = (struct result *)a;
	struct result *b0 = (struct result *)b;

	//fprintf(stderr,"(%s)(%s).",a0->thumbs_db_name,b0->thumbs_db_name);
	int strcmp0 = strcmp(a0->thumbs_db_name, b0->thumbs_db_name);
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
	struct result *a0 = (struct result *)a;
	struct result *b0 = (struct result *)b;
	if(a0->sortorder > b0->sortorder)
		return 1;
	if(a0->sortorder < b0->sortorder)
		return -1;
	return 0;
}

int File_Copy(char FileSource[], char FileDestination[])
{
    char    c[4096]; // or any other constant you like
    FILE    *stream_R = fopen(FileSource, "r");
    FILE    *stream_W = fopen(FileDestination, "w");   //create and write to file
		if(stream_R == NULL) {
			fprintf(stderr,"file source (%s) is null\n", FileSource);
			exit(EXIT_FAILURE);
		}
		if(stream_W == NULL) {
			fprintf(stderr,"file dest (%s) is null\n", FileDestination);
			exit(EXIT_FAILURE);
		}	
    while (!feof(stream_R)) {
        size_t bytes = fread(c, 1, sizeof(c), stream_R);
        if (bytes) {
            fwrite(c, 1, bytes, stream_W);
        }
    }

    //close streams
    fclose(stream_R);
    fclose(stream_W);

    return 0;
}






uint8_t get_image_orientation(unsigned char *buffer, size_t buf_size) {
	
	ExifData *ed;
	ExifEntry *entry;

  /* Load an ExifData object from an EXIF file */
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

gdImagePtr myLoadPng(char *filename, char *origin_name) {
   FILE *in;
   struct stat stat_buf;
   gdImagePtr im;
   in = fopen(filename, "rb");
   if (in==NULL) {
     fprintf(stderr,"image (%s) could not be loaded\n", filename);
     exit(EXIT_FAILURE);
   } 
   if (fstat(fileno(in), &stat_buf) != 0) {
     fprintf(stderr,"fstat error\n");
     exit(EXIT_FAILURE);
   } 
   /* Read the entire thing into a buffer
     that we allocate */
   unsigned char *buffer = malloc(stat_buf.st_size);
   if (!buffer) { 
     fprintf(stderr,"could not allocate memory\n");
     exit(EXIT_FAILURE);
   } 
   if (fread(buffer, 1, stat_buf.st_size, in)
     != stat_buf.st_size) {
     fprintf(stderr,"data could not be read\n");
     exit(EXIT_FAILURE);
   } 
	 if(EndsWith(origin_name,"png")|| EndsWith(origin_name,"PNG")) {
   	im = gdImageCreateFromPngPtr(    stat_buf.st_size, buffer);
	 }else {
   	im = gdImageCreateFromJpegPtr(    stat_buf.st_size, buffer);
	 }

	
	uint8_t orientation = get_image_orientation(buffer, stat_buf.st_size);
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

	

   free(buffer);
   fclose(in);
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
void show_tag(ExifData *d, ExifIfd ifd, ExifTag tag)
{
    /* See if this tag exists */
    ExifEntry *entry = exif_content_get_entry(d->ifd[ifd],tag);
    if (entry) {
        char buf[1024];

        /* Get the contents of the tag in human-readable form */
        exif_entry_get_value(entry, buf, sizeof(buf));

        /* Don't bother printing it if it's entirely blank */
        trim_spaces(buf);
        if (*buf) {
            printf("%s\t%s\t%02X\n", exif_tag_get_name_in_ifd(tag,ifd), buf, entry->data[0]);
        }
    }
}

/* Show the given MakerNote tag if it exists */
void show_mnote_tag(ExifData *d, unsigned tag)
{
    ExifMnoteData *mn = exif_data_get_mnote_data(d);
    if (mn) {
        int num = exif_mnote_data_count(mn);
        int i;

        /* Loop through all MakerNote tags, searching for the desired one */
        for (i=0; i < num; ++i) {
            char buf[1024];
            if (exif_mnote_data_get_id(mn, i) == tag) {
                if (exif_mnote_data_get_value(mn, i, buf, sizeof(buf))) {
                    /* Don't bother printing it if it's entirely blank */
                    trim_spaces(buf);
                    if (*buf) {
                        printf("%s: %s\n", exif_mnote_data_get_title(mn, i),
                            buf);
                    }
                }
            }
        }
    }
}

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

int mosaik2_indextask_read_image(mosaik2_indextask *task) {
	if(is_file_local( task->filename )) {
		FILE *file = fopen( task->filename, "rb");
		if(file==NULL) {
			fprintf(stderr, "could not open file (%s)\n", task->filename);
			return errno;
		}
		unsigned char *buf = malloc(task->filesize);
		if(buf == NULL) {
			fprintf(stderr, "could not allocate memory for image data\n");
			fclose(file);
			return errno;
		}
		size_t read = fread(buf,1,task->filesize,file);
		if(read != task->filesize) {
			fprintf(stderr,"could not read (%li) the expected (%li) amount of data\n", read, task->filesize);
			fclose(file);
			free(buf);
			return EINVAL;
		}
		
		task->image_data = buf;
		fclose(file);
		
	} else {
		fprintf(stderr, "only reading of local files is currently implemented\n");
		exit(1);
	}
	return 0;
}
void mosai2_indextask_deconst(mosaik2_indextask *task) {
	
}

#include <sys/timeb.h>
void print_usage(char *m) {
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
}

