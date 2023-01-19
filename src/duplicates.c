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

/*void p(unsigned char *p0, unsigned char *p1, int nl) {
		size_t s0,s1;
		memcpy(&s0, p0+MD5_DIGEST_LENGTH, sizeof(size_t));
		memcpy(&s1, p1+MD5_DIGEST_LENGTH, sizeof(size_t));

		for(int i=0;i<MD5_DIGEST_LENGTH;i++) {
			fprintf(stderr, "%02x", p0[i]);
		}
		fprintf(stderr, ":%li  ", s0);
		for(int i=0;i<MD5_DIGEST_LENGTH;i++) {
			fprintf(stderr, "%02x", p1[i]);
		}
		fprintf(stderr, ":%li", s1);

	if(nl)
		fprintf(stderr, "\n");
}*/


int mosaik2_duplicates(mosaik2_arguments *args) {

	char *mosaik2_db_name_1 = args->mosaik2db;
	char *mosaik2_db_name_2 = mosaik2_db_name_1;
	if(args->mosaik2dbs_count > 0  ) {
		mosaik2_db_name_2 = args->mosaik2dbs[0];
	}
	int dry_run = args->dry_run;

	mosaik2_database md0;
	init_mosaik2_database(&md0, mosaik2_db_name_1);
	check_thumbs_db(&md0);

	mosaik2_database md1;
	init_mosaik2_database(&md1, mosaik2_db_name_2);
	check_thumbs_db(&md1);

	if(dry_run < 0 || dry_run > 1) {
		fprintf(stderr, "dry_run must be 0 or 1\n");
	}

	if(check_filehashes_index(&md0) == FILEHASHES_INDEX_INVALID) {
		build_filehashes_index(&md0);
	}
	if(check_filehashes_index(&md1) == FILEHASHES_INDEX_INVALID) {
		build_filehashes_index(&md1);
	}

	//uint64_t element_count1 = read_thumbs_db_count(&md1);	

	int debug=0;

	//uint64_t mosaik2_database_elems0 = read_thumbs_db_count(&md0);
	//uint64_t mosaik2_database_elems1 = read_thumbs_db_count(&md1);

	FILE *filehashes_file0       = m_fopen(md0.filehashes_filename, "rb");
	FILE *filehashes_file1       = m_fopen(md1.filehashes_filename, "rb");
	FILE *filehashes_index_file0 = m_fopen(md0.filehashes_index_filename, "rb");
	FILE *filehashes_index_file1 = m_fopen(md1.filehashes_index_filename, "rb");
	FILE *duplicates_file0       = m_fopen(md0.duplicates_filename, "r");
	FILE *duplicates_file1       = m_fopen(md1.duplicates_filename, "r+");//normal writing without truncating or appending
	FILE *filenames_index_file   = m_fopen(md1.filenames_index_filename, "r");
	FILE *filenames_file         = m_fopen(md1.filenames_filename, "r");

	// DRY RUN operates the same way, data is written to a temporary copy of duplicates file from md1
	if(dry_run==1) {
		FILE *tmp_file = tmpfile();
		
		size_t bytes;
		uint8_t buf[BUFSIZ];
		while ((bytes = fread(&buf, 1, BUFSIZ, duplicates_file1)) != 0) {
			m_fwrite(&buf, bytes, tmp_file);
		}
		m_fflush(tmp_file);
		rewind(tmp_file);	//playback file position to 0


		m_fclose(duplicates_file1);
		duplicates_file1 = tmp_file;
		
	} // dry_run end


	int dataset_len = MD5_DIGEST_LENGTH + sizeof(size_t);	
	unsigned char f0[dataset_len]; // current hash to compare with rest
	unsigned char f1[dataset_len]; // contains part of the rest
	
	size_t size_read_h0, size_read_h1;
	uint8_t duplicates_data0=0,duplicates_data1=0;
	uint64_t i0=0, j0=0;
	uint8_t compare_same_file=strncmp(md0.filehashes_filename, md1.filehashes_filename, strlen(md0.filehashes_filename)) == 0;

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
			
	for(i0=0,j0=0;(( size_read_h0 = fread(f0, dataset_len, 1, filehashes_index_file0)) == 1);i0++){


		if(compare_same_file) { 
			// in the same file the compare partner needs to be off by one
			m_fseeko(filehashes_index_file1, ftello(filehashes_index_file0), SEEK_SET);
			m_fseeko(duplicates_file1, ftello(filehashes_index_file0), SEEK_SET);
		}




		for(;(( size_read_h1 = fread(f1, dataset_len, 1, filehashes_index_file1)) == 1);j0++) {
			
			int qsort = qsort_(f0, f1);
			
			if(qsort < 0) {
				m_fseeko(filehashes_index_file1,-dataset_len,SEEK_CUR);
				m_fseeko(duplicates_file1,-1,SEEK_CUR);
				break;
			} else if( qsort > 0) {
			
				continue;
				exit(EXIT_FAILURE);
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
				duplicates_data1=1;
				m_fwrite(&duplicates_data1, 1, duplicates_file1);

				//long off0 = 0;
				//long off1 = -1;
				//read_filenames_index(filenames_index_file, dup_offset1, &off0);
				//if(dup_offset1!=element_count1-1) // elements after the last element should not be read
				//	read_filenames_index(filenames_index_file, dup_offset1+1, &off1);
				//print_filename(filenames_file, off0, off1);
				char *filename =  mosaik2_database_read_element_filename(&md1,dup_offset1+1,filenames_index_file);
				printf("%s", filename);
				free(filename);

			} // else block ends
		}
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
	
	return 0;
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

	FILE *filehashes_index_file = m_fopen(md->filehashes_index_filename, "w");

	struct sysinfo info;
	m_sysinfo(&info);

	double freeram = info.freeram * 0.9; // use 90 % of free ram
	if(freeram<MD5_DIGEST_LENGTH*100) {
		fprintf(stderr,"free ram is too low\n");
		exit(EXIT_FAILURE);
	}
	int dataset_len = MD5_DIGEST_LENGTH + sizeof(size_t);
	uint64_t nmemb = read_thumbs_db_count(md);
	int loop_size = ceil(nmemb * (dataset_len) / freeram);
	uint64_t chunk_nmemb[loop_size];
	for(int i=0;i<loop_size;i++) {
		chunk_nmemb[i] = (nmemb / loop_size);
	}
	chunk_nmemb[0] += nmemb - chunk_nmemb[0] * loop_size; // add the difference from average chunk_nmemb to nmemb to the first
	for(int i=0;i<loop_size;i++) {
		//if(debug)fprintf(stderr, "nmemb:%i chunk_nmemb:%i\n", nmemb, chunk_nmemb[i]);
	}
	
	unsigned char *buf = m_malloc(chunk_nmemb[0] * (dataset_len));

	FILE *tfiles[loop_size];
	for(int i=0;i<loop_size;i++) {
		tfiles[i] = tmpfile();
		if(tfiles[i]==NULL) {
			fprintf(stderr, "could not create tempfile for external sort\n");
			exit(EXIT_FAILURE);
		}
	}

	FILE *filehashes_file = m_fopen(md->filehashes_filename, "r");
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
