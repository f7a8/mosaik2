/*
   _                   _  _     _            
  (_) _ _  __ __ __ _ | |(_) __| |      __ 
  | || ' \ \ V // _` || || |/ _` |  _  / _|
  |_||_||_| \_/ \__/_||_||_|\__/_| (_) \__|

*/
#include "libmosaik2.h"

/* automatically invalid detection is written to the first bit and preserves existing user defined invalid states in other bits */
void mark_invalid(FILE *invalid_file, size_t nmemb, char *filename ) {

	// need to be an opend file	
	m_fseeko(invalid_file, nmemb, SEEK_SET);
	int old_value=0;
	m_fread(&old_value, 1, invalid_file);
	int new_value = old_value | 1;
	m_fseeko(invalid_file, nmemb, SEEK_SET);
	m_fwrite(&new_value, 1, invalid_file);
}

void print_invalid(char *filename, size_t filesize, time_t last_modified ) {
	fprintf(stdout, "%s\t%li\t%li\n", filename, filesize, last_modified);
}

void print_invalid_(char *filename,int access) {
	if(access != 0) {
		fprintf(stderr, "file (%s) not accessible => invalid: %s\n", filename, strerror(errno));
		return;
	}
	size_t filesize;
	time_t last_modified;
	 struct stat st;
	//TODO take care about missing files
	m_stat(filename, &st);
	filesize = st.st_size;	
	last_modified = st.st_mtim.tv_sec;

	print_invalid(filename, filesize, last_modified);
}

int mosaik2_invalid(mosaik2_arguments *args) {

	char *mosaik2_db_name = args->mosaik2db;
	int ignore_old_invalids = args->ignore_old_invalids;
	int dry_run = args->dry_run;
	int no_hash_cmp = args->no_hash_cmp;
	int debug = args->verbose;
	uint32_t element_number = 0;
	uint32_t invalid_count = 0;

	mosaik2_database md;
	init_mosaik2_database(&md, mosaik2_db_name);
	check_thumbs_db(&md);

	if(ignore_old_invalids<0 || ignore_old_invalids >1) {
		fprintf(stderr, "ingore_old_invalids must be 0 or 1\n");
		exit(EXIT_FAILURE);
	}

	if(dry_run < 0 || dry_run > 1) {
		fprintf(stderr, "dry_run must be 0 or 1\n");
		exit(EXIT_FAILURE);
	}

	if(no_hash_cmp < 0 || no_hash_cmp > 1 ) {
		fprintf(stderr, "no_hash_cmp must be 0 or 1\n");
	}

	if(args->has_element_identifier == ELEMENT_NUMBER && args->element_number < 1 ) {
		fprintf(stderr, "illegal value of element_number. exit\n");
		exit(EXIT_FAILURE);
	}
	element_number = args->element_number - 1;

	if (args->has_element_identifier == ELEMENT_FILENAME ) {
		int val = mosaik2_database_find_element_number(&md, args->element_filename, &element_number);
		if(val != 0) {
			fprintf(stderr, "element not found\n");
			exit(EXIT_FAILURE);
		}
	}



	uint32_t mosaik2_database_elems = read_thumbs_db_count(&md);
	if( args->has_element_identifier == ELEMENT_NUMBER && element_number > mosaik2_database_elems ) {
		fprintf(stderr, "element number out of range\n");
		exit(EXIT_FAILURE);
	}


	FILE *invalid_file = m_fopen(md.invalid_filename, "r+");

	if(args->has_element_identifier != 0) {

		m_fseeko(invalid_file, element_number, SEEK_SET);
		int old_value=0;
		m_fread(&old_value, 1, invalid_file);
		int new_value = old_value ^ 2;
		m_fseeko(invalid_file, element_number, SEEK_SET);
		m_fwrite(&new_value, 1, invalid_file);
		m_fflush(invalid_file);
		m_fclose(invalid_file);

		return 0;
	}

	FILE *filenames_file  = m_fopen(md.filenames_filename, "r");
	FILE *timestamps_file = m_fopen(md.timestamps_filename, "rb");
	FILE *filesizes_file  = m_fopen(md.filesizes_filename, "rb");
	FILE *filehashes_file = m_fopen(md.filehashes_filename, "rb");
	
	char buf[MAX_FILENAME_LEN];
	uint8_t image_data[BUFSIZ];
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
		m_fseeko(invalid_file,j*1,SEEK_SET);

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
			if( access_code != 0 ) {


				// NOT READABLE => INVALID
				if(debug)fprintf(stderr, "%li. file (%s) cannot be accessed => %s\n", j, buf, strerror(errno));
				print_invalid_(buf,access_code);
				if(!dry_run) {
					mark_invalid(invalid_file,j,md.invalid_filename);
					invalid_count++;
				}
				continue;
			} else {

				//FILE IS ACCESSABLE

				if(debug) fprintf(stderr, "check timestamp\n");
				// lets first do a timestamp comparison (should be faster, than hash cmp)
				time_t old_timestamp;
				m_fseeko(timestamps_file,j*sizeof(time_t),SEEK_SET);
				m_fread(&old_timestamp, sizeof(time_t), timestamps_file);


				struct stat file_stat;
				m_stat( buf, &file_stat);
				time_t cur_timestamp = file_stat.st_mtim.tv_sec;
				off_t cur_filesize = file_stat.st_size;

				if(debug) fprintf(stderr, "old_timestamp: %li, new_timestamp: %li\n", old_timestamp, cur_timestamp);

				// THERE IS A FILE WITH A ANOTHER TIMESTAMP
				if(cur_timestamp == old_timestamp) {
					//BOTH TIMESTAMPS ARE EQUAL, NOW COMPARE THE SIZE
					if(debug) printf("no newer file\n");
					//continue;
				} else {
					if(debug)fprintf(stdout, "file version with different timestamps for (%s) available, mark this file as invalid\n",buf);
					print_invalid( buf, cur_filesize, cur_timestamp );	
					if(!dry_run) {
						mark_invalid(invalid_file, j,md.invalid_filename);
						invalid_count++;
					}
					continue;
				}
				
				size_t old_filesize = 0;
				m_fseeko(filesizes_file,j*sizeof(size_t),SEEK_SET);
				m_fread(&old_filesize, sizeof(size_t), filesizes_file );

				if(debug) fprintf(stderr, "old_filesize: %li, cur_filesize: %li\n", old_filesize, cur_filesize);
				// IF FILESIZE HAS CHANGED => IT IS INVALID
				if(cur_filesize != old_filesize) {
					
					fprintf(stderr, "file (%s) version with different filesize (old:%li cur:%li) available => invalid\n",buf, old_filesize, cur_filesize);
					print_invalid(buf, cur_filesize, cur_timestamp );
					if(!dry_run) {
						mark_invalid(invalid_file,j,md.invalid_filename);
						invalid_count++;
					}
					
				} else {
					
					if(no_hash_cmp == 1) 
						continue;

					//NEW TIMESTAMP, SAME SIZE? LET'S COMAPARE THE HASH FOR BEING SURE
					if(debug) printf("no changed filesize\n");
					
					//MAKE A MD5 HASH COMPARE
						
					uint8_t old_hash[MD5_DIGEST_LENGTH];
					m_fseeko(filehashes_file, j*MD5_DIGEST_LENGTH, SEEK_SET);
					m_fread(&old_hash, MD5_DIGEST_LENGTH, filehashes_file);

					FILE *image_file = m_fopen(buf, "rb");

					size_t bytes;
					unsigned char new_hash[MD5_DIGEST_LENGTH];
					
					#if OPENSSL_VERSION_NUMBER >= 0x10100000L
					EVP_MD_CTX *mdctx = EVP_MD_CTX_new();
					#else
					EVP_MD_CTX *mdctx = EVP_MD_CTX_create();
					#endif
					if(mdctx==NULL) {
						fprintf(stderr, "could not create digest context\n");
						exit(EXIT_FAILURE);
					}
					if(!EVP_DigestInit_ex(mdctx, EVP_md5(), NULL)) {
						fprintf(stderr, "could not create digest context\n");
						exit(EXIT_FAILURE);
					}
					while ((bytes = fread(image_data, 1, BUFSIZ, image_file)) != 0) {
						if(!EVP_DigestUpdate(mdctx, image_data, bytes)) {
							fprintf(stderr, "error update digest for element %li\n", j);
							exit(EXIT_FAILURE);
						}
					}
					unsigned int md5_digest_length = MD5_DIGEST_LENGTH;
					if(!EVP_DigestFinal_ex(mdctx, new_hash, &md5_digest_length)) {
						fprintf(stderr, "error digestfinal for element %li\n", j);
						exit(EXIT_FAILURE);
					}
					#if OPENSSL_VERSION_NUMBER >= 0x10100000L
					EVP_MD_CTX_free(mdctx);
					#else
					EVP_MD_CTX_destroy(mdctx);
					#endif
					m_fclose(image_file);

					if( memcmp(new_hash, old_hash, MD5_DIGEST_LENGTH) == 0 ) {
						if(debug) fprintf(stdout, "filehashes are EQUAL\n");
					} else {
						fprintf(stderr, "file hashes mismatch, mark file (%s) as invalid\n", buf);
						print_invalid( buf, cur_filesize, cur_timestamp );
						if(!dry_run) {
							mark_invalid(invalid_file,j,md.invalid_filename);
							invalid_count++;
						}
					}
				}
			}
		}
	}
	m_fflush(invalid_file);
	m_fclose(invalid_file);
	m_fclose(filesizes_file);
	m_fclose(filenames_file);
	m_fclose(timestamps_file);
	m_fclose(filehashes_file);

	if(invalid_count>0) {
		FILE *lastmodified_file = m_fopen(md.lastmodified_filename, "w");
		time_t now = time(NULL);
		m_fwrite(&now, md.lastmodified_sizeof, lastmodified_file);
		m_fclose(lastmodified_file);
	}

	return 0;
}
