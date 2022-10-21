#include "libmosaik2.h"

void print_database(mosaik2_arguments *args, char* mosaik2_db_name, mosaik2_database *md);
void print_element (mosaik2_arguments *args, char* mosaik2_db_name, mosaik2_database *md, int element_number);

int mosaik2_info(mosaik2_arguments *args) {

	char *mosaik2_db_name = args->mosaik2db;
	int element_number = args->element_number;

	if(args->has_element_number && element_number < 1  ) {
		fprintf(stderr, "illegal value of element_number. exit\n");
		exit(EXIT_FAILURE);
	}

	mosaik2_database md;
	init_mosaik2_database(&md, mosaik2_db_name);
	check_thumbs_db(&md);

	if( args->has_element_number==1) {
		print_element(args, mosaik2_db_name, &md, element_number-1);
	} else {
		read_database_id(&md);
		print_database(args, mosaik2_db_name, &md);
	}

	return 0;
}
void read_entry(char *filename, void *val, int len, int offset)  {
	FILE *f = fopen(filename, "r");
	if(f == NULL) {
		fprintf(stderr, "mosaik2 database file (%s)  could not be opened\n", filename);
		exit(EXIT_FAILURE);
	}
	if( fseeko(f, offset, SEEK_SET) != 0) {
		fprintf(stderr, "mosaik2 database file could not be seeked\n");
		exit(EXIT_FAILURE);
	}
	if( fread(val, 1, len, f) != len) {
		fprintf(stderr, "cannot read value\n");
		exit(EXIT_FAILURE);
	}
	if(fclose(f) != 0) {
		fprintf(stderr, "mosaik2 database file (%s) could not be closed\n", filename);
		exit(EXIT_FAILURE);
	}
}

void print_database(mosaik2_arguments *args, char* mosaik2_db_name, mosaik2_database *md) {

	printf("path=%s\n", mosaik2_db_name);
	printf("id=%s\n", md->id);
	printf("db-format-version=%i\n", MOSAIK2_DATABASE_FORMAT_VERSION);
	printf("image-resolution=%i\n", read_thumbs_conf_tilecount(md));
	time_t lastmodified = read_thumbs_db_lastmodified(md);
	printf("last-modified=%s", ctime( &lastmodified));
	printf("size=%li\n", read_thumbs_db_size(md));
	printf("element-count=%li\n", read_thumbs_db_count(md));
	printf("duplicates-count=%li\n", read_thumbs_db_duplicates_count(md));
	printf("invalid-count=%li\n", read_thumbs_db_invalid_count(md));
}

void print_element(mosaik2_arguments *args, char* mosaik2_db_name, mosaik2_database *md, int element_number) {
	uint64_t element_count = read_thumbs_db_count(md);
	if( element_number >= element_count ) {
		fprintf(stderr, "element number out of range\n");
		exit(EXIT_FAILURE);
	}


	printf("db-name=%s\n", mosaik2_db_name);
	printf("element-number=%i\n", element_number+1);

	
	unsigned char hash[MD5_DIGEST_LENGTH];
	read_entry(md->filehashes_filename, hash, MD5_DIGEST_LENGTH, element_number*MD5_DIGEST_LENGTH);
	printf("md5-hash="); for(int i=0;i<MD5_DIGEST_LENGTH;i++) { printf("%02x", hash[i]); } 	printf("\n");

	off_t offset;
	off_t offset2;
	read_entry(md->filenames_index_filename, &offset, sizeof(offset), element_number*sizeof(off_t));
	// calculate the filename length is done by getting the next offset, but if it is the last, take the file size
	if( element_number == element_count-1) {
		offset2 = get_file_size(md->filenames_filename);
	} else {
		read_entry(md->filenames_index_filename, &offset2, sizeof(offset), (element_number+1)*sizeof(off_t));
		offset2--;
	}
	char filename[offset2-offset+1];
	memset(filename,0,offset2-offset+1);
	read_entry(md->filenames_filename, &filename, offset2-offset, offset);
	printf("filename=%s\n", filename);

	ssize_t val_ssize;
	read_entry(md->filesizes_filename, &val_ssize, sizeof(ssize_t), element_number*sizeof(ssize_t));
	printf("filesize=%li\n", val_ssize);

	int val_int[2];
	read_entry(md->imagedims_filename, &val_int, sizeof(int)*2, element_number*2*sizeof(int));
	printf("image-dim=%ix%i\n", val_int[0], val_int[1]);


	unsigned char val_char[2];
	read_entry(md->tiledims_filename, &val_char, sizeof(char)*2, element_number*2*sizeof(char));
	printf("tile-dim=%ix%i\n", val_char[0], val_char[1]);

	time_t val_time;
	read_entry(md->timestamps_filename, &val_time, sizeof(time_t), element_number*sizeof(time_t));
	printf("timestamp=%s", ctime(&val_time));

	unsigned char val;
	read_entry(md->duplicates_filename, &val, 1, element_number * 1);
	printf("duplicate=%i\n", val);
	
	read_entry(md->invalid_filename, &val, 1, element_number);
	printf("invalid=%i\n", val);

}
