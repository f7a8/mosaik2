/*
    _        _ __  _  _            _                       
 __| | _  _ | '_ \| |(_) __  __ _ | |_  ___  ___        __ 
/ _` || || || .__/| || |/ _|/ _` ||  _|/ -_)(_-/  _    / _|
\__/_| \_._||_|   |_||_|\__|\__/_| \__|\___|/__/ (_)   \__|
*/


#include "libmosaik2.h"

//TODO very inefficient, better search the external sorted hashnumber

int check_filehashes_index(mosaik2_database *md);
void build_filehashes_index(mosaik2_database *md);
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
	init_mosaik2_database(&md0, mosaik2_database_name_1);
	mosaik2_database_check(&md0);

	mosaik2_database md1;
	init_mosaik2_database(&md1, mosaik2_database_name_2);
	mosaik2_database_check(&md1);

	if(dry_run < 0 || dry_run > 1) {
		fprintf(stderr, "dry_run must be 0 or 1\n");
		exit(EXIT_FAILURE);
	}

	if(args->has_phash_distance && ( args->phash_distance < 1 || args->phash_distance > 32)) {
		fprintf(stderr, "perceptual hash distance must be between 1 and 32\n");
		exit(EXIT_FAILURE);
	}

	if(check_filehashes_index(&md0) == FILEHASHES_INDEX_INVALID) {
		build_filehashes_index(&md0);
	}
	if(check_filehashes_index(&md1) == FILEHASHES_INDEX_INVALID) {
		build_filehashes_index(&md1);
	}

#ifdef HAVE_PHASH
	if(args->has_phash_distance && mosaik2_database_phashes_check(&md0) == PHASHES_INVALID ) {
		mosaik2_database_phashes_build(&md0);
	}
	if(args->has_phash_distance && mosaik2_database_phashes_check(&md1) == PHASHES_INVALID ) {
		mosaik2_database_phashes_build(&md1);
	}
#endif
	int debug=0;

	m2file filehashes_file0       = m_fopen(md0.filehashes_filename, "rb");
	m2file filehashes_file1       = m_fopen(md1.filehashes_filename, "rb");
	m2file filehashes_index_file0 = m_fopen(md0.filehashes_index_filename, "rb");
	m2file filehashes_index_file1 = m_fopen(md1.filehashes_index_filename, "rb");
	m2file duplicates_file0       = m_fopen(md0.duplicates_filename, args->has_phash_distance ? "r+" : "r");
	m2file duplicates_file1       = m_fopen(md1.duplicates_filename, "r+");//normal writing without truncating or appending
	m2file filenames_index_file   = m_fopen(md1.filenames_index_filename, "r");
	m2file filenames_file         = m_fopen(md1.filenames_filename, "r");
#ifdef HAVE_PHASH
	m2file phash_file0            = m_fopen(md0.phash_filename, "r");
	m2file phash_file1            = m_fopen(md1.phash_filename, "r");
#endif

	int compare_same_file=is_same_file(md0.filehashes_filename, md1.filehashes_filename);
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


	int dataset_len = MD5_DIGEST_LENGTH + sizeof(size_t);	
	unsigned char f0[dataset_len]; // current hash to compare with rest
	unsigned char f1[dataset_len]; // contains part of the rest
	
	size_t size_read_h0, size_read_h1;
	uint8_t duplicates_data0=0,duplicates_data1=0;
	uint32_t i0=0, j0=0;

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

	uint32_t duplicates_count=0;
			
	for(i0=0,j0=0;(( size_read_h0 = fread(f0, dataset_len, 1, filehashes_index_file0)) == 1);i0++){


		if(compare_same_file) { 
			// in the same file the compare partner needs to be off by one
			m_fseeko(filehashes_index_file1, ftello(filehashes_index_file0), SEEK_SET);
			m_fseeko(duplicates_file1, ftello(filehashes_index_file0), SEEK_SET);
		}


		for(;(( size_read_h1 = fread(f1, dataset_len, 1, filehashes_index_file1)) == 1);j0++) {
			

			//compare both md5 digests
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

				if(duplicates_data1==1) {
					if(debug)fprintf(stderr,"#filehash1 1#%li is already marked as duplicate, ignore it\n", dup_offset1);
					continue;
				}

				m_fseeko(duplicates_file1, dup_offset1, SEEK_SET);
				duplicates_data1=IS_DUPLICATE;
				m_fwrite(&duplicates_data1, 1, duplicates_file1);

				m2name filename =  mosaik2_database_read_element_filename(&md1,dup_offset1+1,filenames_index_file);
				printf("%s\n", filename);
				free(filename);
				duplicates_count++;
			} // else block ends
		}
	}

#ifdef HAVE_PHASH
	if(args->has_phash_distance) {

	read_thumbs_db_histogram(&md0);
	read_thumbs_db_histogram(&md1);

		m_fseeko(duplicates_file0, 0, SEEK_SET);
		m_fseeko(duplicates_file1, 0, SEEK_SET);
		m_fseeko(phash_file0, 0, SEEK_SET);
		m_fseeko(phash_file1, 0, SEEK_SET);

		unsigned char phash_buf0[md0.phash_sizeof], phash_buf1[md1.phash_sizeof];
		memset(phash_buf0, 0, sizeof(unsigned long long));
		memset(phash_buf1, 0, sizeof(unsigned long long));
		unsigned long long phash0=0, phash1=0;
		int phash_val0=0, phash_val1=0;

		for(i0 = 0;(( size_read_h0 = fread(&duplicates_data0, md0.duplicates_sizeof, 1, duplicates_file0)) == 1);i0++) {
			if( duplicates_data0 != IS_NO_DUPLICATE ) {
				//	fprintf(stderr, "i0 %i is marked as duplicate\n",i0);
				continue;
			}

			// in case of phash... load those data
			read_file_entry(phash_file0, &phash0, sizeof(unsigned long long), i0*md0.phash_sizeof); //read single values from a single database file.
			read_file_entry(phash_file0, &phash_val0, sizeof(int), i0*md0.phash_sizeof+sizeof(unsigned long long)); //read single values from a single database file.

			m_fseeko(duplicates_file1,0,SEEK_SET);
			for(j0=0;((size_read_h1=fread(&duplicates_data1, md1.duplicates_sizeof, 1, duplicates_file1)) == 1);j0++) {
//fprintf(stderr, ".");
				if(i0 == j0 && compare_same_file)
					continue;
				if(duplicates_data1 != IS_NO_DUPLICATE) {
				//	fprintf(stderr, "i0 (%i) j0 %i is marked as duplicate\n",i0, j0);
					continue;
				}

				read_file_entry(phash_file1, &phash1, sizeof(unsigned long long), j0*md1.phash_sizeof); //read single values from a single database file.
				read_file_entry(phash_file1, &phash_val1, sizeof(int), j0*md1.phash_sizeof+sizeof(unsigned long long)); //read single values from a single database file.
				
				// check if original hash function calls did return 0 to show that they computed a valid hash
				if( phash_val0 == 0 && phash_val1 == 0 ) {
					// if two hash phash0 and phash1 are XORed, the resulting number of setted bits 
					unsigned long long phash = phash0 ^ phash1;
					int count_bits = __builtin_popcountll(phash);
					//fprintf(stderr, "count_bits %4i %4i:%20llu %20llu | 0:", i0,j0, phash0, phash1);
						//fprintf(stderr, " count %2i \n", count_bits);
					
								
					if(count_bits <= args->phash_distance) {
						duplicates_count++;

						// to read an entire mosaik2_database_element seems to be a bit overhead.
						// but in most cases phash duplicates should match only a small percentage
						// and code is reused.
						mosaik2_database_element mde0, mde1;
						memset(&mde0, 0, sizeof(mde0));
						memset(&mde1, 0, sizeof(mde1));
						mosaik2_database_read_element(&md0, &mde0, i0);
						mosaik2_database_read_element(&md1, &mde1, j0);

						float costs0 = mosaik2_database_costs(&md0, &mde0);
						float costs1 = mosaik2_database_costs(&md1, &mde1);
				
						if(costs0<=costs1) {
							if(args->verbose)
								printf("%s,%s,%i,%f,%f\n", mde0.filename, mde1.filename, count_bits, costs0, costs1);
							else
								printf("%s\n", mde0.filename);
							m_fseeko(duplicates_file0, i0, SEEK_SET);
							duplicates_data0=IS_PHASH_DUPLICATE;
							m_fwrite(&duplicates_data0, 1, duplicates_file0);
							m_fflush(duplicates_file0);
						} else {
							if(args->verbose)
								printf("%s,%s,%i,%f,%f\n", mde1.filename, mde0.filename, count_bits, costs1, costs0);
							else
								printf("%s\n", mde0.filename);
							m_fseeko(duplicates_file1, j0, SEEK_SET);
							duplicates_data1=IS_PHASH_DUPLICATE;
							m_fwrite(&duplicates_data1, 1, duplicates_file1);
							m_fflush(duplicates_file1);
						}

						free(mde0.filename);
						free(mde1.filename);
						//no other duplicates check here neccessary
						continue;
					}
				}
			}
		}
	}
#endif

	if(duplicates_count>0 && dry_run ==0) {
		m2file lastmodified_file = m_fopen(md0.lastmodified_filename, "w");
		time_t now = time(NULL);
		m_fwrite(&now, md0.lastmodified_sizeof, lastmodified_file);
		m_fclose(lastmodified_file);
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
	m_fclose(filenames_file);
	m_fclose(filenames_index_file);
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
void build_filehashes_index(mosaik2_database *md) {
	//if(debug)fprintf(stderr, "build filehashes index\n");

	m2file filehashes_index_file = m_fopen(md->filehashes_index_filename, "w");

	struct sysinfo info;
	m_sysinfo(&info);

	double freeram = info.freeram * 0.9; // use 90 % of free ram
	if(freeram<MD5_DIGEST_LENGTH*100) {
		fprintf(stderr,"free ram is too low\n");
		exit(EXIT_FAILURE);
	}
	int dataset_len = MD5_DIGEST_LENGTH + sizeof(size_t);
	uint32_t nmemb = read_thumbs_db_count(md);
	int loop_size = ceil(nmemb * (dataset_len) / freeram);
	uint32_t chunk_nmemb[loop_size];
	for(int i=0;i<loop_size;i++) {
		chunk_nmemb[i] = (nmemb / loop_size);
	}
	chunk_nmemb[0] += nmemb - chunk_nmemb[0] * loop_size; // add the difference from average chunk_nmemb to nmemb to the first
	for(int i=0;i<loop_size;i++) {
		//if(debug)fprintf(stderr, "nmemb:%i chunk_nmemb:%i\n", nmemb, chunk_nmemb[i]);
	}
	
	unsigned char *buf = m_malloc(chunk_nmemb[0] * (dataset_len));

	m2file tfiles[loop_size];
	for(int i=0;i<loop_size;i++) {
		tfiles[i] = tmpfile();
		if(tfiles[i]==NULL) {
			fprintf(stderr, "could not create tempfile for external sort\n");
			exit(EXIT_FAILURE);
		}
	}

	m2file filehashes_file = m_fopen(md->filehashes_filename, "r");
	for(size_t i=0;i<loop_size;i++) {
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
	int eof_count=0;

	int smallest_index;

	for(int j=0;j<loop_size;j++) {
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
			int j=0;
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

	for(int i=0;i<loop_size;i++) {
		m_fclose(tfiles[i]);// will be unlinked anyway, here no error handling
	}

	m_fclose(filehashes_index_file);
}

/*
 * compares the last modified timestamps of one mosaik2 database directory
 * if the file .lastmodifed has newer changes than the filehashes.idx FILEHASHES_INDEX_INVALID
 * is returnd, FILEHASHES_INDEX_VALID otherwise.
 */
int check_filehashes_index(mosaik2_database *md) {
	struct stat lastmodified_file, filehashes_index_file;

	m_stat(md->lastmodified_filename, &lastmodified_file);
	m_stat(md->filehashes_index_filename,&filehashes_index_file);

	if( filehashes_index_file.st_ctim.tv_sec > lastmodified_file.st_ctim.tv_sec ||
			( filehashes_index_file.st_ctim.tv_sec == lastmodified_file.st_ctim.tv_sec &&
				filehashes_index_file.st_ctim.tv_nsec > lastmodified_file.st_ctim.tv_nsec )) {
		//filehashes index was created after last modified timestamp of that mosaik2_database => still fine
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

