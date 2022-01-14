/*
   _                   _  _     _            
  (_) _ _  __ __ __ _ | |(_) __| |      __ 
  | || ' \ \ V // _` || || |/ _` |  _  / _|
  |_||_||_| \_/ \__/_||_||_|\__/_| (_) \__|

*/
#include "libmosaik2.h"

uint8_t DRY_RUN = 0;

void mark_invalid(FILE *invalid_file, size_t nmemb, char *filename ) {

	if(DRY_RUN == 1) 
		return;
	// need to be an opend file	
	if( fseek(invalid_file,nmemb*1,SEEK_SET) == -1 ) {
		fprintf(stderr, "error while setting file cursor to %li in invalid file\n", nmemb );
		exit(EXIT_FAILURE);
	}
	uint8_t invalid_data=1;
	if(fwrite(&invalid_data,1,1,invalid_file) != 1 ) { // == 1, because of one element was hopefully written
		fprintf(stderr, "error while marking image %li in invalid file as invalid\n", nmemb);
	}
}

void print_invalid(char *filename, size_t filesize, time_t last_modified ) {
	fprintf(stdout, "%s\t%li\t%li\n", filename, filesize, last_modified);
}

void print_invalid_(char *filename) {
	size_t filesize;
	time_t last_modified;
  struct stat st;
	if( stat(filename, &st) != 0) {
		fprintf(stderr, "error cannot gather file information from (%s)\n", filename);
		exit(EXIT_FAILURE);
	}
	filesize = st.st_size;	
	last_modified = st.st_mtim.tv_sec;

	print_invalid(filename, filesize, last_modified);
}

int mosaik2_invalid(char *mosaik2_db_name, int ignore_old_invalids, int dry_run) {


	struct mosaik2_database_struct md;
	init_mosaik2_database_struct(&md, mosaik2_db_name);

	if(ignore_old_invalids<0 || ignore_old_invalids >1) {
		fprintf(stderr, "ingore_old_invalids must be 0 or 1\n");
	}

	if(dry_run < 0 || dry_run > 1) {
		fprintf(stderr, "dry_run must be 0 or 1\n");
	} else {
		DRY_RUN = dry_run;
	}
		

	int debug=0;
	uint64_t mosaik2_database_elems = read_thumbs_db_count(&md);
	//uint64_t found_invalid=0;
	
	FILE *filenames_file = fopen(md.filenames_filename, "r");
	if( filenames_file == NULL ) {
		fprintf(stderr, "filenames file (%s) could not be opened\n", md.filenames_filename);
		exit(EXIT_FAILURE);
	} 

	FILE *invalid_file = fopen(md.invalid_filename, "r+");
	if(invalid_file == NULL) {
		fprintf(stderr, "invalid file (%s) could not be opened\n", md.invalid_filename);
		exit(EXIT_FAILURE);
	}
	
	FILE *timestamps_file = fopen(md.timestamps_filename, "rb");	
	if( timestamps_file == NULL) {
		fprintf(stderr, "timestamps file (%s) could not be opened\n", md.timestamps_filename);
		exit(EXIT_FAILURE);
	}

	FILE *filesizes_file = fopen(md.filesizes_filename, "rb");	
	if( filesizes_file == NULL) {
		fprintf(stderr, "filesizes file (%s) could not be opened\n", md.filesizes_filename);
		exit(EXIT_FAILURE);
	}

	FILE *filehashes_file = fopen(md.filehashes_filename, "rb");
	if( filehashes_file == NULL ) {
		fprintf(stderr, "filehashes file (%s) could not be opened\n", md.filehashes_filename);
		exit(EXIT_FAILURE);
	}
	
	char buf[MAX_FILENAME_LEN];
	uint8_t image_data[65536];
	//printf("%li database elems\n", mosaik2_database_elems);
	for(uint64_t j=0;j<mosaik2_database_elems;j++) {
		memset(buf,0,MAX_FILENAME_LEN);
		char *freads_read = fgets(buf, MAX_FILENAME_LEN, filenames_file);
		//printf("%li: %s\n",j,buf);
		if(freads_read == NULL) {
			fprintf(stderr, "read less data than it expected at element %li\n",j);
			exit(EXIT_FAILURE);
		}
		size_t buflen = strlen( buf);
		if(buflen>0 && buf[buflen-1]=='\n') {
			buf[buflen-1]=0;
		}
		if(debug)fprintf(stderr,"filename:%s\n", buf);

		// IS THIS ELEMENT ALREADY INVALID? THAN SKIP IT
		if( fseek(invalid_file,j*1,SEEK_SET) == -1 ) {
			fprintf(stderr, "error while setting file cursor to %li in invalid file\n", j );
			exit(EXIT_FAILURE);
		}
	uint8_t invalid_data=0;
	if(fread(&invalid_data,1,1,invalid_file) != 1 ) { // == 1, because of one element was hopefully written
			fprintf(stderr, "error while reading image %li  invalids status\n", j);
	} else if( ignore_old_invalids == 0 && invalid_data == 1) {
		continue;
	}


		if(is_file_wikimedia_commons(buf)) {
			//TODO extend print_invalid for urls
			fprintf(stderr, "not implemented\n"); exit(EXIT_FAILURE);
		} else {
			if(debug) fprintf(stderr, "check access\n"); 
			
			int access_code = access( buf, R_OK );
			if(debug) fprintf(stderr, "access%i\n",access_code);
			if( access_code != 0 ) {


				// NOT READABLE => INVALID
				if(debug)fprintf(stderr, "%li. file (%s) is not readable => invalid\n", j, buf);
				print_invalid_(buf);
				mark_invalid(invalid_file,j,md.invalid_filename);
				continue;
			} else {



				if(debug) fprintf(stderr, "check timestamp\n");
				// lets first do a timestamp comparison (should be faster, than hash cmp)
				time_t old_timestamp;
				if(fseek(timestamps_file,j*sizeof(time_t),SEEK_SET) == -1) {
					fprintf(stderr, "error setting file cursor to nmemb %li in timestamps file\n", j);
					exit(EXIT_FAILURE);
				}
				if( fread(&old_timestamp, sizeof(time_t), 1, timestamps_file) == -1 ) {
					fprintf(stderr, "the timestamp data could not be read for element %li\n", j);
					exit(EXIT_FAILURE);
				}


				struct stat file_stat;
				stat( buf, &file_stat);
				time_t cur_timestamp = file_stat.st_mtim.tv_sec;
				off_t cur_filesize = file_stat.st_size;

				if(debug) fprintf(stderr, "old_timestamp: %li, new_timestamp: %li\n", old_timestamp, cur_timestamp);

				// THERE IS A FILE WITH A NEWER TIMESTAMP
				if(cur_timestamp > old_timestamp) {
					if(debug)fprintf(stdout, "file version with different timestamp for (%s) available, mark this file as invalid\n",buf);
					print_invalid( buf, cur_filesize, cur_timestamp );	
					mark_invalid(invalid_file, j,md.invalid_filename);
				} else {
					if(debug) printf("no newer file\n");
					continue;
				}
				
				uint32_t old_filesize = 0;
				if(fseek(filesizes_file,j*sizeof(uint32_t),SEEK_SET) == -1) {
					fprintf(stderr, "error setting file cursor to nmemb %li in timestamps file\n", j);
					exit(EXIT_FAILURE);
				}
				if( fread(&old_filesize, sizeof(uint32_t), 1, filesizes_file ) == -1 ) {
					fprintf(stderr, "the filesizes data could not be read for element %li\n", j);
					exit(EXIT_FAILURE);
				}

				// IF FILESIZE HAS CHANGED => IT IS INVALID
				if(cur_filesize != old_filesize) {
					
					fprintf(stderr, "file version with different filesize (%s) %i %li available, mark this file as invalid\n",buf, old_filesize, cur_filesize);
					print_invalid(buf, cur_filesize, cur_timestamp );
					mark_invalid(invalid_file,j,md.invalid_filename);
					
				} else {

					//NEW TIMESTAMP, SAME SIZE? LET'S COMAPARE THE HASH FOR BEING SURE
					if(debug) printf("no changed filesize\n");
					
					//MAKE A HASH COMPARE
						
					uint8_t old_hash[MD5_DIGEST_LENGTH];
					if( fseek(filehashes_file, j*MD5_DIGEST_LENGTH, SEEK_SET) == -1 ) {
						fprintf(stderr, "error while setting filehashes cursor position %li\n",j);
						exit(EXIT_FAILURE);
					}
					if( fread(&old_hash, MD5_DIGEST_LENGTH, 1,filehashes_file) != 1 ) {
						fprintf(stderr, "error while reading file hash for element %li\n",j);
						exit(EXIT_FAILURE);
					}


					FILE *image_file = fopen(buf, "rb");
					if(image_file==NULL) {
						fprintf(stderr,"error while opening image file (%s)\n", buf);
						exit(EXIT_FAILURE);
					}


					size_t bytes;
					unsigned char new_hash[MD5_DIGEST_LENGTH];
					
					MD5_CTX md5_ctx;
					if( MD5_Init(&md5_ctx) != 1) {
						fprintf(stderr, "could not init md5 context\n");
						exit(EXIT_FAILURE);
					}
					while ((bytes = fread (image_data, 1, 4096, image_file)) != 0) {
    				if( MD5_Update (&md5_ctx, image_data, bytes)  == 0 ) {
							fprintf(stderr, "error md5_update for element %li\n", j);
							exit(EXIT_FAILURE);
						}
					}
					if( MD5_Final(new_hash, &md5_ctx) == 0 ) {
						fprintf(stderr, "error md5_final for element %li\n", j);
						exit(EXIT_FAILURE);
					}
					
					if(  new_hash[0] == old_hash[0]
						&& new_hash[1] == old_hash[1]
						&& new_hash[2] == old_hash[2]
						&& new_hash[3] == old_hash[3]
						&& new_hash[4] == old_hash[4]
						&& new_hash[5] == old_hash[5]
						&& new_hash[6] == old_hash[6]
						&& new_hash[7] == old_hash[7]
						&& new_hash[8] == old_hash[8]
						&& new_hash[9] == old_hash[9]
						&& new_hash[10]== old_hash[10]
						&& new_hash[11]== old_hash[11]
						&& new_hash[12]== old_hash[12]
						&& new_hash[13]== old_hash[13]
						&& new_hash[14]== old_hash[14]
						&& new_hash[15]== old_hash[15]) {
						if(debug) fprintf(stdout, "filehashes are EQUAL\n");
					} else {
						fprintf(stderr, "file hashes mismatch, mark file (%s) as invalid\n", buf);
						print_invalid( buf, cur_filesize, cur_timestamp );
						mark_invalid(invalid_file,j,md.invalid_filename);
					}
				}
			}
		}
	}
	fflush(invalid_file);
	fclose(invalid_file);
	fclose(filesizes_file);
	fclose(filenames_file);
	fclose(timestamps_file);
	fclose(filehashes_file);
	return 0;
}
