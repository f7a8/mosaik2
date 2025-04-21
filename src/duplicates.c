/*
    _        _ __  _  _            _                       
 __| | _  _ | '_ \| |(_) __  __ _ | |_  ___  ___        __ 
/ _` || || || .__/| || |/ _|/ _` ||  _|/ -_)(_-/  _    / _|
\__/_| \_._||_|   |_||_|\__|\__/_| \__|\___|/__/ (_)   \__|
*/


#include "libmosaik2.h"

//TODO very inefficient, better search the external sorted hashnumber

int check_filehashes_index(mosaik2_database *md);
void build_filehashes_index(mosaik2_database *md, mosaik2_arguments *args);
int qsort_(const void*, const void*);

const int FILEHASHES_INDEX_VALID = 1;
const int FILEHASHES_INDEX_INVALID = 0;



int mosaik2_duplicates(mosaik2_arguments *args) {

	m2name mosaik2_database_name_1 = args->mosaik2db;
	m2name mosaik2_database_name_2 = mosaik2_database_name_1;
	if(args->mosaik2dbs_count > 0  ) {
		mosaik2_database_name_2 = args->mosaik2dbs[0];
	}
	int dry_run = args->dry_run;

	mosaik2_database md0;
	mosaik2_database_init(&md0, mosaik2_database_name_1);
	mosaik2_database_check(&md0);

	mosaik2_database md1;
	mosaik2_database_init(&md1, mosaik2_database_name_2);
	mosaik2_database_check(&md1);

	if(dry_run < 0 || dry_run > 1) {
		fprintf(stderr, "dry_run must be 0 or 1\n");
		exit(EXIT_FAILURE);
	}

	if(args->has_phash_distance && ( args->phash_distance < 1 || args->phash_distance > 32)) {
		fprintf(stderr, "perceptual hash distance must be between 1 and 32\n");
		exit(EXIT_FAILURE);
	}

	if(dry_run) {
		mosaik2_database_lock_reader(&md0);
		mosaik2_database_lock_reader(&md1);
	} else {
		mosaik2_database_lock_reader(&md1); 
	}

	if(check_filehashes_index(&md0) == FILEHASHES_INDEX_INVALID) {
		build_filehashes_index(&md0, args);
	}
	if(check_filehashes_index(&md1) == FILEHASHES_INDEX_INVALID) {
		build_filehashes_index(&md1, args);
	}

#ifdef HAVE_PHASH
	if(args->has_phash_distance && mosaik2_database_phashes_check(&md0) == PHASHES_INVALID ) {
		mosaik2_database_phashes_build(&md0, args);
	}
	if(args->has_phash_distance && mosaik2_database_phashes_check(&md1) == PHASHES_INVALID ) {
		mosaik2_database_phashes_build(&md1, args);
	}
#endif
	int debug=0;

	m2file filehashes_file0       = m_fopen(md0.filehashes_filename, "rb");
	m2file filehashes_file1       = m_fopen(md1.filehashes_filename, "rb");
	m2file filehashes_index_file0 = m_fopen(md0.filehashes_index_filename, "rb");
	m2file filehashes_index_file1 = m_fopen(md1.filehashes_index_filename, "rb");
	m2file filenames_index_file0   = m_fopen(md0.filenames_index_filename, "r");
	m2file filenames_index_file1   = m_fopen(md1.filenames_index_filename, "r");
	m2file filenames_file0         = m_fopen(md0.filenames_filename, "r");
	m2file filenames_file1         = m_fopen(md1.filenames_filename, "r");
#ifdef HAVE_PHASH
	m2file phash_file0            = m_fopen(md0.phash_filename, "r");
	m2file phash_file1            = m_fopen(md1.phash_filename, "r");
#endif
	m2file duplicates_file0       = m_fopen(md0.duplicates_filename, args->has_phash_distance ? "r+" : "r");
	m2file duplicates_file1       = m_fopen(md1.duplicates_filename, "r+");//normal writing without truncating or appending
	m2file invalid_file0       = m_fopen(md0.invalid_filename, "r");
	m2file invalid_file1       = m_fopen(md1.invalid_filename, "r");

	int compare_same_file=is_same_file(md0.filehashes_filename, md1.filehashes_filename);

	if(compare_same_file) {
		unset_file_buf(duplicates_file0);
		unset_file_buf(duplicates_file1);
	}

	// DRY RUN operates the same way, data is written to a temporary copy of duplicates file from md1
	if(dry_run==1) {
		size_t bytes;
		uint8_t buf[BUFSIZ];

		
		m2file tmp_file;
	       
		tmp_file = m_tmpfile();
		while ((bytes = fread(&buf, 1, BUFSIZ, duplicates_file1)) != 0) {
			m_fwrite(&buf, bytes, tmp_file);
		}
		m_fflush(tmp_file);
		rewind(tmp_file);	//playback file position to 0
		m_fclose(duplicates_file1);
		duplicates_file1 = tmp_file;

		if( compare_same_file ) {
			duplicates_file0 = fdopen( dup(fileno(tmp_file)),"a+");

		} else {
			tmp_file = m_tmpfile();
			while ((bytes = fread(&buf, 1, BUFSIZ, duplicates_file0)) != 0) {
				m_fwrite(&buf, bytes, tmp_file);
			}
			m_fflush(tmp_file);
			rewind(tmp_file);	//playback file position to 0
			m_fclose(duplicates_file0);
			duplicates_file0 = tmp_file;
		}
	} // dry_run end


	int dataset_len = md0.filehashes_index_sizeof;// MD5_DIGEST_LENGTH + sizeof(size_t);	
	m2elem element_count0 = mosaik2_database_read_element_count(&md0);
	m2elem element_count1 = mosaik2_database_read_element_count(&md1);
	unsigned char f0[dataset_len]; // current hash to compare with rest
	unsigned char f1[dataset_len]; // contains part of the rest
	
	size_t size_read_h0, size_read_h1;
	uint8_t duplicates_data0=0,duplicates_data1=0,invalid_data0=0, invalid_data1=0;
	m2elem i0=0, j0=0;

	/* idea:
	 * filehashes.idx are openend from two mosaik2 databases (m0 and m1), also their duplicates.bin files.
	 * The first file is called filehashes_index_file0 (f0) and the second filehashes_index_file1.
	 * A loop over all filehashes in f0 is started. In that outer loop an inner
	 * loop is started over all filehashes h0 in filehashes_index_file1 (f1) load the desired comparing partner h1.
	 * If h0 and h1 are equal, a 1 is written to the m1 duplicates.bin at the right position.
	 * The right position in duplicates.bin differes from the logical number in f1. In the filehashes_files are not 
	 * only the hashes itself saved in order, but their corresponding logical offset (size_t) called o0 and o1
	 * is appended to every filehash.
	 * If h1 is greater than h0, increment the f0 pointer, and set the f1 pointer to the desired comparig partner.
	 * The desired comparing depends on the equalness between m0 and m1. If they are the same, the comparing 
	 * partner should be inital off by one, otherwise not.
	 */


	// loop through every element. 
	// it the next element in the filehashes.idx file (sorted) the same hash like the current one?
	// mark the next one as duplicate.

	m2elem duplicates_count=0;
	m2size quoted_filename_length = MAX_FILENAME_LEN*2+2;
			
	for(i0=0,j0=0;(( size_read_h0 = fread(f0, dataset_len, 1, filehashes_index_file0)) == 1) /*&& i0 < element_count-2*/;i0++){

		//do not compare files, which are marked as invalid
		read_file_entry(invalid_file0, &invalid_data0, md0.invalid_sizeof, i0*md0.invalid_sizeof);
		if( invalid_data0 != INVALID_NONE ) {
			continue;
		}

		if(compare_same_file) { 
			// in the same file the compare partner needs to be off by one
			m_fseeko(filehashes_index_file1, m_ftello(filehashes_index_file0), SEEK_SET);
			m_fseeko(duplicates_file1, m_ftello(filehashes_index_file0), SEEK_SET);
		}


		for(;(( size_read_h1 = fread(f1, dataset_len, 1, filehashes_index_file1)) == 1) /*&& i0 < element_count-1*/;j0++) {
			

			//do not compare files, which are marked as invalid
			read_file_entry(invalid_file1, &invalid_data1, md1.invalid_sizeof, j0*md1.invalid_sizeof);
			if( invalid_data1 != INVALID_NONE ) {
				continue;
			}

			//compare only both md5 digests
			int qsort = qsort_(f0, f1);
			if(qsort < 0) {
				// match partner is lower than orignal? try next match partner
				m_fseeko(filehashes_index_file1,-dataset_len,SEEK_CUR);
				m_fseeko(duplicates_file1,-1,SEEK_CUR);//TODO raises an error
				break;
			} else if( qsort > 0) {
				// match parter is higher than orignal, skip here inner for loop try next f0 original.
				continue;
			} else { // oh, they are equal!

				//f0 or f1 already marked as duplicate?
				
			
				size_t dup_offset0;
				memcpy(&dup_offset0, f0+MD5_DIGEST_LENGTH, sizeof(size_t));

				m_fseeko(duplicates_file0,dup_offset0,SEEK_SET);
				m_fread(&duplicates_data0,1,duplicates_file0);

				if(duplicates_data0==1) {
					if(debug)fprintf(stdout,"#filehash0 0#%li is already marked as duplicate, ignore it\n", dup_offset0);
					continue;
				}

				size_t dup_offset1;
				memcpy(&dup_offset1, f1+MD5_DIGEST_LENGTH, sizeof(size_t));

				m_fseeko(duplicates_file1, dup_offset1, SEEK_SET);
				m_fread(&duplicates_data1,1,duplicates_file1);

				if(duplicates_data1==IS_DUPLICATE) {
					if(debug)fprintf(stderr,"#filehash1 1#%li is already marked as duplicate, ignore it\n", dup_offset1);
					continue;
				}

				m_fseeko(duplicates_file1, dup_offset1, SEEK_SET);
				duplicates_data1=IS_DUPLICATE;
				m_fwrite(&duplicates_data1, 1, duplicates_file1);
				//fprintf(stderr, "in duplicates_file1 written at %li this value %i\n", dup_offset1, duplicates_data1);
				
				if(duplicates_count==0 && args->verbose) {
					printf("\nimage_which_stays,image_marked_as_duplicate,count_bits,cost_0,cost_1\n");
				}
				//fprintf(stderr, "i0: %u, j0: %u, element_count:%u\n", i0,j0,element_count);
				//fprintf(stderr, "dup_offset1: %lu, dup_offset1+1: %lu\n",  dup_offset1,dup_offset1+1);
				char quoted_filename[quoted_filename_length];
				if(args->verbose) {
					m2name filename = mosaik2_database_read_element_filename(&md0,dup_offset0,filenames_index_file0,filenames_file0);
					quote_string(filename, quoted_filename_length, quoted_filename);
					printf("%s,", quoted_filename);
					m_free((void **)&filename);
					filename = mosaik2_database_read_element_filename(&md1,dup_offset1,filenames_index_file1,filenames_file1);
					quote_string(filename, quoted_filename_length, quoted_filename);
					printf("%s,,,\n", quoted_filename);
					m_free((void **)&filename);
				} else {
					m2name filename = mosaik2_database_read_element_filename(&md1,dup_offset1,filenames_index_file1,filenames_file1);
					quote_string(filename, quoted_filename_length, quoted_filename);
					printf("%s,,,\n", quoted_filename);
					m_free((void **)&filename);
				}
				duplicates_count++;
			} // oh, they are equal! else block ends
		}
	}
	m_fflush(duplicates_file1);
	

#ifdef HAVE_PHASH
	if(args->has_phash_distance) {

		mosaik2_database_read_histogram(&md0);
		mosaik2_database_read_histogram(&md1);
		
		m_fseeko(duplicates_file0, 0, SEEK_SET);
		m_fseeko(duplicates_file1, 0, SEEK_SET);
		m_fseeko(phash_file0, 0, SEEK_SET);
		m_fseeko(phash_file1, 0, SEEK_SET);

		unsigned char phash_buf0[md0.phash_sizeof], phash_buf1[md1.phash_sizeof];
		memset(phash_buf0, 0, sizeof(unsigned long long));
		memset(phash_buf1, 0, sizeof(unsigned long long));
		m2phash phash0, phash1;

		for(i0 = 0;i0<element_count0;i0++) {
			
			read_file_entry(duplicates_file0, &duplicates_data0, sizeof(duplicates_data0), i0*sizeof(duplicates_data0));
			
			//fprintf(stderr, "for(i:%i duplicate0 value %i\n",i0,duplicates_data0);
			
			if( duplicates_data0 != IS_NO_DUPLICATE ) {
				//fprintf(stderr, "CONTINUE1   duplicates_data0 != IS_NO_DUPLICATE\n");
				//fprintf(stderr, "i:%i is marked as duplicate:%i\n",i0,IS_NO_DUPLICATE);
				continue;
			}

			//do not compare files, which are marked as invalid
			read_file_entry(invalid_file0, &invalid_data0, md0.invalid_sizeof, i0*md0.invalid_sizeof);
			if( invalid_data0 != INVALID_NONE ) {
				continue;
			}


			//fprintf(stderr, "READ PHASH0 at %i\n",i0*md1.phash_sizeof);
			// in case of phash... load those data
			read_file_entry(phash_file0, &phash0, md0.phash_sizeof, i0*md0.phash_sizeof);

			//m_fseeko(duplicates_file1,0,SEEK_SET);
			
			if(compare_same_file)
				j0=i0+1;
			else
				j0 = 0;
			for(;j0<element_count1;j0++) {
				read_file_entry(duplicates_file1, &duplicates_data1, sizeof(duplicates_data1), j0*sizeof(duplicates_data1));

				if(i0 == j0 && compare_same_file) {
					continue;
				}
					
				if(duplicates_data1 != IS_NO_DUPLICATE) {
					continue;
				}

				//do not compare files, which are marked as invalid
				read_file_entry(invalid_file1, &invalid_data1, md1.invalid_sizeof, j0*md1.invalid_sizeof);
				if( invalid_data1 != INVALID_NONE ) {
					continue;
				}


				//fprintf(stderr, "  i:%i j:%i  duplicate0:%i  duplicate1:%i\n",i0, j0, duplicates_data0, duplicates_data1);


				//fprintf(stderr, "READ PHASH1  at %i for %i\n",j0*md1.phash_sizeof,md1.phash_sizeof);
				read_file_entry(phash_file1, &phash1, md1.phash_sizeof, j0*md1.phash_sizeof); //read single values from a single database file.
				
				/*m2name filename = mosaik2_database_read_element_filename(&md0,i0,filenames_index_file0,filenames_file0);
				printf(" ==> %i : %i bits  %s -> ", phash0.val, phash1.val, filename);
				m_free((void**)&filename);
				filename = mosaik2_database_read_element_filename(&md1,j0,filenames_index_file1,filenames_file1);
				printf("%s\n", filename);
				m_free((void**)&filename);*/

				// check if original hash function calls did return 0 to show that they computed a valid hash
				if( phash0.val == 0 && phash1.val == 0 ) {
					// if two hash phash0 and phash1 are XORed, the resulting number of setted bits 
					phash my_phash = phash0.hash ^ phash1.hash;
					int count_bits = __builtin_popcountll(my_phash);

					/*m2name filename = mosaik2_database_read_element_filename(&md0,i0,filenames_index_file0,filenames_file0);
					printf(" ==> %i bits  %s -> ", count_bits, filename);
					m_free((void**)&filename);
					filename = mosaik2_database_read_element_filename(&md1,j0,filenames_index_file1,filenames_file1);
					printf("%s\n", filename);
					m_free((void**)&filename);*/
					//fprintf(stderr, "count_bits %4i %4i:%20llu %20llu | 0:", i0,j0, phash0, phash1);
					//fprintf(stderr, " count %2i \n", count_bits);
					
					if(count_bits <= args->phash_distance) {
						if(duplicates_count==0 && args->verbose) {
							printf("\nimage_which_stays,image_marked_as_duplicate,count_bits,cost_0,cost_1\n");
						}
						duplicates_count++;

						// to read an entire mosaik2_database_element seems to be a bit overhead.
						// but in most cases phash duplicates should match only a small percentage
						// and code is reused.
						mosaik2_database_element mde0, mde1;
						memset(&mde0, 0, sizeof(mde0));
						memset(&mde1, 0, sizeof(mde1));
						//fprintf(stderr, "mosaik2_database_read_element\n");
						mosaik2_database_read_element(&md0, &mde0, i0);
						mosaik2_database_read_element(&md1, &mde1, j0);

						float costs0 = mosaik2_database_costs(&md0, &mde0);
						float costs1 = mosaik2_database_costs(&md1, &mde1);
				
						char mde0qfilename[quoted_filename_length];
						char mde1qfilename[quoted_filename_length];

						quote_string(mde0.filename, quoted_filename_length, mde0qfilename);
						quote_string(mde1.filename, quoted_filename_length, mde1qfilename);

						//printf(":%s,%s,%i,%f,%f\n", mde0.filename, mde1.filename, count_bits, costs0, costs1);
						if(costs0<=costs1) {

							printf("%s,%s,%i,%f,%f,%llx,%llx\n", mde1qfilename, mde0qfilename, count_bits, costs1, costs0, phash1.hash, phash0.hash);
							//printf("duplicate:%s\n",mde0.filename);
						
							//TODO should only be in dest db
							//if(args->verbose)
							//	printf("%s,%s,%i,%f,%f\n", mde1.filename, mde0.filename, count_bits, costs1, costs0);
							//else
							//	printf("%s\n", mde0.filename);
							//m2off m2off0 = m_ftello(duplicates_file0);
							m_fseeko(duplicates_file0, i0, SEEK_SET);
							duplicates_data0=IS_PHASH_DUPLICATE;
							m_fwrite(&duplicates_data0, 1, duplicates_file0);
							m_fflush(duplicates_file0);
							//m2off m2off1 = m_ftello(duplicates_file0);
							
							//printf("write %i at %i in duplicates_file0 (ftello:%li ,%li\n", duplicates_data0, i0, m2off0, m2off1);
						} else {
						
							printf("%s,%s,%i,%f,%f,%llx,%llx\n", mde0qfilename, mde1qfilename, count_bits, costs0, costs1, phash0.hash, phash1.hash);
							//if(args->verbose)
							//printf("else %s,%s,%i,%f,%f\n", mde1.filename, mde0.filename,  count_bits, costs1,costs0);
							//printf("duplicate:%s\n",mde1.filename);
							//else
							//	printf("%s\n", mde1.filename);
							//m2off m2off0 = m_ftello(duplicates_file1);
							m_fseeko(duplicates_file1, j0, SEEK_SET);
							duplicates_data1=IS_PHASH_DUPLICATE;
							m_fwrite(&duplicates_data1, 1, duplicates_file1);
							m_fflush(duplicates_file1);
							//m2off m2off1 = m_ftello(duplicates_file1);
							//printf("write %i at %i in duplicates_file1 (ftello:%li ,%li)\n", duplicates_data1, j0,m2off0, m2off1);
						}

						
						/*for(int i=0;i<element_count0;i++) {
							m2data d;
							read_file_entry(duplicates_file0, &d, sizeof(d), i*sizeof(d)); //read single values from a single database file.
							printf("%i ", d);
						}printf(" << 0\n");

						for(int i=0;i<element_count1;i++) {
							m2data d;
							read_file_entry(duplicates_file1, &d, sizeof(d), i*sizeof(d)); //read single values from a single database file.
							printf("%i ", d);
						}printf(" << 1\n");*/

						m_free((void **)&mde0.filename);
						m_free((void **)&mde1.filename);

						if(costs0<=costs1) {
							//printf("BREAK\n" );
							break;
						}
					} else {
						// images are not close enough
					}
				}
			}
			//fprintf(stderr, "j0 ende, i:%i\n",i0);
		}
		//fprintf(stderr, "i0 ende, i:%i\n",i0);
	}
#endif

	if(duplicates_count>0 && dry_run ==0) {
		mosaik2_database_touch_lastmodified(&md0);
	}

	m_fflush(stdout);

	m_fflush(duplicates_file1);
	if(debug)fprintf(stderr,"closing files\n");

	m_fclose(duplicates_file0);
	m_fclose(duplicates_file1);
	m_fclose(filehashes_file0);
	m_fclose(filehashes_file1);
	m_fclose(filehashes_index_file0);
	m_fclose(filehashes_index_file1);
	m_fclose(filenames_file0);
	m_fclose(filenames_file1);
	m_fclose(filenames_index_file0);
	m_fclose(filenames_index_file1);
#ifdef HAVE_PHASH
	m_fclose(phash_file0);
	m_fclose(phash_file1);
#endif

	return 0;
}


/*
 * creates an external sort if free ram is lower than the filehashes.bin size.
 * to achive this: splits up all hashes into n segments, while one segement should be at least as big
 * as 90% of free ram, and write them into individually tmpfiles. All tempfiles are sorted by qsort_ 
 * individually and saved back to file. Later a step retrieves all first (lowest) hashes from all tmpfile
 * and write them in the right order (again with help from qsort_) in the destination file 
 * filehashes_index_filename. Tmpfiles are removed after close automatically by using tmpfile() function.
 */
void build_filehashes_index(mosaik2_database *md, mosaik2_arguments *args) {
	if(args->verbose)fprintf(stderr, "(re)build filehashes index\n");

	//overwrite existing with "w"
	m2file filehashes_index_file = m_fopen(md->filehashes_index_filename, "w");

	struct sysinfo info;
	m_sysinfo(&info);

	double freeram = info.freeram * 0.9; // use 90 % of free ram
	if(freeram<MD5_DIGEST_LENGTH*100) {
		fprintf(stderr,"build_filehashes_index: error, free ram is too low\n");
		exit(EXIT_FAILURE);
	}
	int dataset_len = md->filehashes_index_sizeof;//MD5_DIGEST_LENGTH + sizeof(size_t);
	m2elem nmemb = mosaik2_database_read_element_count(md);
	m2elem loop_size = ceil(nmemb * (dataset_len) / freeram);
	uint32_t chunk_nmemb[loop_size];
	for(m2elem i=0;i<loop_size;i++) {
		chunk_nmemb[i] = (nmemb / loop_size);
	}
	chunk_nmemb[0] += nmemb - chunk_nmemb[0] * loop_size; // add the difference from average chunk_nmemb to nmemb to the first
	for(m2elem i=0;i<loop_size;i++) {
		//if(debug)fprintf(stderr, "nmemb:%i chunk_nmemb:%i\n", nmemb, chunk_nmemb[i]);
	}
	
	unsigned char *buf = m_malloc(chunk_nmemb[0] * (dataset_len));

	m2file tfiles[loop_size];
	for(m2elem i=0;i<loop_size;i++) {
		tfiles[i] = tmpfile();
		if(tfiles[i]==NULL) {
			fprintf(stderr, "build_filehashes_index: error, could not create tempfile for external sort\n");
			exit(EXIT_FAILURE);
		}
	}

	m2file filehashes_file = m_fopen(md->filehashes_filename, "r");
	for(m2elem i=0;i<loop_size;i++) {
		m_fread(buf, MD5_DIGEST_LENGTH*chunk_nmemb[i], filehashes_file);

		//all hashes were read as a blob, now after every hash the position in the file has to be injected
		//p(buf,buf+dataset_len,1);
		
		for(size_t j=chunk_nmemb[i]-1;j!=0;j--) {
			size_t offset_dest = j*(dataset_len);
			size_t offset_no = j*(dataset_len)+MD5_DIGEST_LENGTH;
			size_t offset_src = j*MD5_DIGEST_LENGTH;
			memmove(buf+offset_dest, buf+offset_src, MD5_DIGEST_LENGTH);
			memcpy(buf+offset_no, &j, sizeof(size_t));
		}
		memset(buf+MD5_DIGEST_LENGTH, 0, sizeof(size_t)); // set first number to 0

		qsort(buf, chunk_nmemb[i], dataset_len, qsort_);

		//write all sorted chunks to disk
		m_fwrite(buf, dataset_len*chunk_nmemb[i], tfiles[i]);
		m_fseeko(tfiles[i], 0, SEEK_SET);
	}

	m_free((void **)&buf);
	m_fclose(filehashes_file);


	//merge all temporary file together
	//open all temporary files and iterate them
	unsigned char last_data[loop_size * (dataset_len)];
	memset(last_data, 0, loop_size * (dataset_len));
	size_t read[loop_size];
	memset(read,0,loop_size*sizeof(size_t));
	size_t reads=0;

	uint8_t any_eof=0;
	uint8_t eof[loop_size];//marks if a tempfile has readed eof
	memset(eof,0,loop_size);
	m2elem eof_count=0;

	m2elem smallest_index;

	for(m2elem j=0;j<loop_size;j++) {
	//	rewind(tfiles[j]);
		int elems = fread(&last_data[j*dataset_len], dataset_len, 1, tfiles[j]);
		eof[j] = elems == 0;
		if(eof[j]==1) {
			any_eof=1;
			eof_count++;
		}
		if(eof[j] == 0) {
			read[j]++;
			reads++;
		}
	}

	//iterate over all target member elements
	for(size_t i=0;i<nmemb;i++) {
		smallest_index=0;

		//more than one tempfile exists
		//and none tempfile has eof or if one tempfile has ended,
		//    there must be more than 2 none ended left
		if(loop_size>1&&(any_eof==0||(any_eof==1 && eof_count < loop_size - 1))) {

			//merge all together
			//gets tricky here

			//search the smallest index
			m2elem j=0;
			for(;j<loop_size && eof[j] == 1;j++);
			smallest_index=j;
			//j shows on the first index, that has not ended

			//j shoud show now on the first tfile that has not ben closed
			for(j++;j<loop_size;j++) {
				if(eof[j]==1) { //compare buddy has ended, ignore
					continue;
				}
				if(qsort_(&last_data[smallest_index*dataset_len],&last_data[j*dataset_len])==1) {
					smallest_index = j;
				}
			}
			//smallest index should show on the smallest none ended index
		} else {
			for(smallest_index=0;smallest_index<loop_size;smallest_index++) {
				if(eof[smallest_index]==0)
					break;
			}
		}

		//the smallest possible values is written to the output file

		m_fwrite(&last_data[smallest_index*dataset_len],dataset_len,filehashes_index_file);

		// at its position the next values is loaded
		size_t freads=fread(&last_data[smallest_index*dataset_len],dataset_len,1,tfiles[smallest_index]);

		//should that file has reached the end
		if(freads!=1) {
			any_eof=1;
			eof[smallest_index]=1;//when tempfile is marked as eof, it will not be touched when detecting the smallest index
			eof_count++;
		} else {
			read[smallest_index]++;
			reads++;
		}
	}

	for(m2elem i=0;i<loop_size;i++) {
		m_fclose(tfiles[i]);// will be unlinked anyway, here no error handling
	}

	m_fclose(filehashes_index_file);
}

/*
 * compares the last indexed timestamps of one mosaik2 database directory
 * if the file .lastmodifed has newer changes than the filehashes.idx FILEHASHES_INDEX_INVALID
 * is returnd, FILEHASHES_INDEX_VALID otherwise.
 */
int check_filehashes_index(mosaik2_database *md) {
	struct stat lastindexed_file, filehashes_index_file;

	m_stat(md->lastindexed_filename, &lastindexed_file);
	m_stat(md->filehashes_index_filename,&filehashes_index_file);

	if( filehashes_index_file.st_ctim.tv_sec > lastindexed_file.st_ctim.tv_sec ||
			( filehashes_index_file.st_ctim.tv_sec == lastindexed_file.st_ctim.tv_sec &&
				filehashes_index_file.st_ctim.tv_nsec > lastindexed_file.st_ctim.tv_nsec )) {
		//filehashes index was created after last indexed timestamp of that mosaik2_database => still fine
		return FILEHASHES_INDEX_VALID;

	}
	//if(debug) fprintf(stderr, "filehashes index outdated\n");
	return FILEHASHES_INDEX_INVALID;
}

int qsort_(const void *p0, const void *p1) {
	unsigned char *d0 = (unsigned char*) p0;
	unsigned char *d1 = (unsigned char*) p1;

	for(int i=0;i<MD5_DIGEST_LENGTH;i++) {
		if(d0[i] < d1[i])
			return -1;
		if(d0[i] > d1[i])
			return 1;
	}

	return 0;
}

