/*
    _        _ __  _  _            _                       
 __| | _  _ | '_ \| |(_) __  __ _ | |_  ___  ___        __ 
/ _` || || || .__/| || |/ _|/ _` ||  _|/ -_)(_-/  _    / _|
\__/_| \_._||_|   |_||_|\__|\__/_| \__|\___|/__/ (_)   \__|
*/


#include "libmosaik2.h"

//TODO very inefficient, better search the external sorted hashnumber

int mosaik2_duplicates(char *mosaik2_db_name_1, char *mosaik2_db_name_2, int dry_run) {

	mosaik2_database md0;
	init_mosaik2_database(&md0, mosaik2_db_name_1);
	check_thumbs_db(&md0);

	mosaik2_database md1;
	init_mosaik2_database(&md1, mosaik2_db_name_2);
	check_thumbs_db(&md1);

	if(dry_run < 0 || dry_run > 1) {
		fprintf(stderr, "dry_run must be 0 or 1\n");
	}

	int debug=0;

	//uint64_t mosaik2_database_elems0 = read_thumbs_db_count(&md0);
	//uint64_t mosaik2_database_elems1 = read_thumbs_db_count(&md1);

	FILE *filehashes_file0 = fopen(md0.filehashes_filename, "rb");
	if( filehashes_file0 == NULL) {
		fprintf(stderr, "first filehashes file (%s) could not be opened\n", md0.filehashes_filename);
		exit(EXIT_FAILURE);
	}

	FILE *filehashes_file1 = fopen(md1.filehashes_filename, "rb");
	if( filehashes_file1 == NULL ) {
		fprintf(stderr, "snd filehashes file (%s) could not be opened\n", md1.filehashes_filename);
		exit(EXIT_FAILURE);
	} 

	

	FILE *duplicates_file0 = fopen(md0.duplicates_filename, "r");
	if(duplicates_file0 == NULL) {
		fprintf(stderr, "first duplicates file (%s) could not be opened\n", md0.duplicates_filename);
		exit(EXIT_FAILURE);
	}
	

	FILE *duplicates_file1 = fopen(md1.duplicates_filename, "r+");//normal writing without truncating or appending
	if(duplicates_file1 == NULL) {
		fprintf(stderr, "snd duplicates file (%s) could not be opened\n", md1.duplicates_filename);
		exit(EXIT_FAILURE);
	}

	FILE *filenames_index_file = fopen(md1.filenames_index_filename, "r");
	if(filenames_index_file==NULL) {
		fprintf(stderr,"filenames index file (%s) could not be opened\n",md1.filenames_index_filename);
		exit(EXIT_FAILURE);
	}

	FILE *filenames_file = fopen(md0.filenames_filename, "r");
	if(filenames_file==NULL) {
		fprintf(stderr, "filenames file (%s) could not be opened\n", md0.filenames_filename);
		exit(EXIT_FAILURE);
	}

	// DRY RUN operates the same way, data is written to a duplicates file, but to a temporary one
	if(dry_run==1) {
		int tmp_fd = mkstemp(md1.temporary_duplicates_filename);
		if(tmp_fd==-1) {
			fprintf(stderr, "could not create temporary file for dry run");
			exit(EXIT_FAILURE);
		}
		FILE *tmp_file = fdopen(tmp_fd, "w");
		size_t pos = fseek(duplicates_file1,0,SEEK_SET);
		if(pos==-1) {
			fprintf(stderr, "unable to set file cursor in first filehash to copy to a tempory file\n");
			exit(EXIT_FAILURE);
		}
		size_t bytes;
		uint8_t buf[BUFSIZ];
		while ((bytes = fread(&buf, 1, BUFSIZ, duplicates_file1)) != 0) {
			size_t elems = fwrite(&buf, 1, bytes, tmp_file);
			if(elems != bytes) {
				fprintf(stderr, "an unexpected size of data was read\n");
				exit(EXIT_FAILURE);
			}
		}
		fflush(tmp_file);
		
		//close both files and reopen the temporary one in rw mode
		if( fclose(tmp_file)!= 0 || fclose(duplicates_file1) != 0 ) {
			fprintf(stderr, "error closing duplicates file\n");
			
		}
		//opens again the temporary file as rw, cusor at the beginning
		duplicates_file1 = fopen(md1.temporary_duplicates_filename, "r+"); 
		if(duplicates_file1==NULL) {
			fprintf(stderr, "count not close duplicates_file1\n");
			exit(EXIT_FAILURE);
		}
	}
		
	uint8_t filehash0[MD5_DIGEST_LENGTH]; // current hash to compare with rest
	uint8_t filehash1[MD5_DIGEST_LENGTH]; // contains part of the rest
	
  size_t size_read0, size_read1;
	uint64_t i0=0, j0=0;
	uint8_t duplicates_data=1;
	uint8_t compare_same_file=strncmp(md0.filehashes_filename, md1.filehashes_filename, strlen(md0.filehashes_filename)) == 0;
	while( ( size_read0 = fread(filehash0, MD5_DIGEST_LENGTH, 1, filehashes_file0) ) == 1 ) {
			// is current element already marked as duplicate? => ignore
			size_t bytes=fread(&duplicates_data,1,1,duplicates_file0);
			if(bytes !=1) {
				fprintf(stderr,"error reading already saved duplicates (1): %li\n",bytes);
				exit(EXIT_FAILURE);
			}
			if(duplicates_data==1) {
				//COMPARE PARTNER A is aready saved as duplicates, ignore it.
				if(debug)fprintf(stdout,"filehash0 %li is already marked as duplicate, ignore it\n", i0);
				i0++;
				continue;
			}	


		if(debug)fprintf(stdout,   "read0 i:%li\n",i0);

			if(compare_same_file) {
				if(debug)fprintf(stdout,"set snd filehashes ptr to byte offset %li\n", i0*MD5_DIGEST_LENGTH+MD5_DIGEST_LENGTH);
				if(fseek(filehashes_file1, i0*MD5_DIGEST_LENGTH+MD5_DIGEST_LENGTH, SEEK_SET)!=0) {
					fprintf(stderr,"comparing the same data: could not seek off by one element in the second compare file\n");
					exit(EXIT_FAILURE);
				};// if same file start at one hash after i0
				j0 = i0+1;
			} else {
				if(debug)fprintf(stdout,"set snd filehashes ptr to 0\n");
				rewind(filehashes_file1);// another file must be compared entirely
				j0 = 0;
			}
			while( ( size_read1 = fread(filehash1, MD5_DIGEST_LENGTH, 1, filehashes_file1) ) == 1 ) {

				//if compare buddy is already duplicated skip it
				// TODO is this faster, than check all compare bodies?
					if(fseek(duplicates_file1,j0,SEEK_SET)!=0) {
						fprintf(stderr,"error setting duplicates file cursor\n");
						exit(EXIT_FAILURE);
					}
					size_t fread_t = fread(&duplicates_data,1,1,duplicates_file1);
					if(fread_t!=1) {
						fprintf(stderr,"error reading already saved duplicate (2) %li\n", fread_t);
						exit(EXIT_FAILURE);
					}
					if(duplicates_data==1) {
						if(debug)printf("COMPARE PARTNER %li is aready saved as duplicates, ignore it\n", j0);
						j0++;
						continue;
					}	

				if(memcmp(filehash0, filehash1, MD5_DIGEST_LENGTH) == 0) {
						//if(debug) {fprintf(stdout, "duplicates found!! test exit \n%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x\n%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x\n", filehash0[0], filehash0[1], filehash0[2], filehash0[3],filehash0[4],filehash0[5],filehash0[6],filehash0[7],filehash0[8],filehash0[9],filehash0[10],filehash0[11],filehash0[12],filehash0[13],filehash0[14],filehash0[15],filehash1[0], filehash1[1], filehash1[2], filehash1[3], filehash1[4], filehash1[5], filehash1[6], filehash1[7], filehash1[8], filehash1[9], filehash1[10], filehash1[11], filehash1[12], filehash1[13], filehash1[14], filehash1[15]); exit(0);}
						if(debug)	fprintf(stdout, "mark byte %li as duplicates", j0);
						fseek(duplicates_file1, j0, SEEK_SET);	
						duplicates_data=1;
						size_t fwrite_bytes = fwrite(&duplicates_data, 1, 1, duplicates_file1);
						if(fwrite_bytes != 1) {
							fprintf(stderr, "database element could not be marked as duplicate\n");
							exit(EXIT_FAILURE);
						}

						if(fseek(filenames_index_file, j0*sizeof(j0), SEEK_SET) != 0) {//j0 show on the element index, it has to multiplied for the right index
							fprintf(stderr, "error setting filenames_index_file stream position\n");
							exit(EXIT_FAILURE);
						}
						uint64_t filenames_index=0;
						if( fread(&filenames_index, sizeof(uint64_t), 1, filenames_index_file) == -1 ) {
							fprintf(stderr, "error reading filenames_offset");
							exit(EXIT_FAILURE);
						}
						if(fseek(filenames_file, filenames_index, SEEK_SET) != 0) {
							fprintf(stderr, "error setting filenames_file stream position:%li\n",filenames_index+1);
							exit(EXIT_FAILURE);
						}
						
						char filename[MAX_FILENAME_LEN];
						char *filename2 = fgets(filename, MAX_FILENAME_LEN, filenames_file);
						if(filename != filename2) {
							fprintf(stderr, "read less data than it expected");
							exit(EXIT_FAILURE);
						}
						printf("%s",filename);//contains newline..
						break;
				}
			j0++;
		}
		i0++;
	}
	
	if(debug)fprintf(stderr,"closing files\n");
	if( fclose(duplicates_file0) != 0 
		|| fclose(duplicates_file1) != 0
		|| fclose(filehashes_file0) != 0
		|| fclose(filehashes_file1) != 0
		|| fclose(filenames_file) != 0
		|| fclose(filenames_index_file) != 0 ) {
			fprintf(stderr, "error closing files\n");
			exit(EXIT_FAILURE);
	}
	
	if(dry_run ) {
		if(debug)fprintf(stderr,"delete temp file from dry run\n");
		if(  unlink(md1.temporary_duplicates_filename) != 0) {
			fprintf(stderr,"could not delete temporary duplicates file (for dry run)\n");
			exit(EXIT_FAILURE);
		}
	}

	return 0;
}
