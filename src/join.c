//TODO check is db older than the result file?
//TODO unique_tiles 

//      _
//     (_)      _       
//     | | ___ (_) _ _  
//	  _/ |/ _ \| || ' \
//	 |__/ \___/|_||_||_|

#include "mosaik21.h"

//for curl writing
static size_t write_data(void *ptr, size_t size, size_t nmemb, void *stream) {
  size_t written = fwrite(ptr, size, nmemb, (FILE *)stream);
  return written;
}

int mosaik2_join(char *dest_filename, int dest_tile_width, int unique_tile, int local_cache, int argc, char **argv) {
	char * home = getenv("HOME");

	int ft = check_dest_filename( dest_filename );

	if(dest_tile_width<1) {
		fprintf(stderr, "dest_tile_width should be greater 1\n");
		exit(EXIT_FAILURE);
	}

	if(local_cache<0||local_cache>1) {
		fprintf(stderr, "local_cache must be 0 or 1. 0 creates symlinks and 1 copies the files into the home directory\n");
		exit(EXIT_FAILURE);
	}

	if(unique_tile<0||unique_tile>1) {
		fprintf(stderr, "unique_tile must be 0 or 1.\n");
		exit(EXIT_FAILURE);
	} 

	int debug = 0;

	
	uint8_t argv_start_idx_thumbs_db_names = 6;
	uint8_t argv_end_idx_thumbs_db_names = argc;	
	uint8_t master_tile_x_count;// atoi(argv[2]);
	uint8_t master_tile_y_count;// = atoi(argv[3]);

	
	uint8_t tile_count = 0;

	// init
if(debug) fprintf(stderr, "init\n");
	struct mosaik2_project_struct mp;
	struct mosaik2_database_struct mds[argv_end_idx_thumbs_db_names - argv_start_idx_thumbs_db_names];

	//read master tile n count from file
	for(uint32_t i=argv_start_idx_thumbs_db_names, i0=0; i<argv_end_idx_thumbs_db_names; i++,i0++) {
		if(debug) fprintf( stderr, "check %i %s\n", i, argv[i]);
		
		init_mosaik2_database_struct(&mds[i0], argv[i]);
		check_thumbs_db(&mds[i0]);

		init_mosaik2_project_struct(&mp, argv[i], dest_filename);

		if(debug)
			fprintf(stderr,"thumbs_db_name:%s,dest_filename:%s,mastertiledis:%s\n", argv[i],dest_filename,mp.dest_mastertiledims_filename);

		FILE *mastertiledims_file = fopen(mp.dest_mastertiledims_filename, "r");
		if( mastertiledims_file == NULL) {
			fprintf(stderr, "master tile dims file (%s) could not be opened\n", mp.dest_mastertiledims_filename);
			exit(EXIT_FAILURE);
		} else if(debug) {
			fprintf(stderr, "mastertiledims file loaded\n");
		}
		struct stat st;
		stat(mp.dest_mastertiledims_filename, &st);
		char buf[st.st_size];
		memset(buf, 0 , st.st_size);

		int freads_reads = fread(buf, 1, st.st_size, mastertiledims_file);
		if(st.st_size != freads_reads) {
			fprintf(stderr, "read data than it expected (%s)", mp.dest_mastertiledims_filename);
			exit(EXIT_FAILURE);
		}

		uint8_t master_tile_x_count_local = 0;
		uint8_t master_tile_y_count_local = 0;
	
  	char *ptr;
		ptr=strtok(buf,"\n\t");if(ptr==NULL){fprintf(stderr,"error while parsing master tile dims file\n");exit(EXIT_FAILURE);}
		master_tile_x_count_local = atoi( ptr );
		ptr=strtok(NULL,"\n\t");if(ptr==NULL){fprintf(stderr,"error while parsing master tile dims file\n");exit(EXIT_FAILURE);}
		master_tile_y_count_local = atoi( ptr );
		fclose(mastertiledims_file);

		if( i > argv_start_idx_thumbs_db_names
				&& (master_tile_x_count != master_tile_x_count_local
				|| master_tile_y_count != master_tile_y_count_local)) {
			fprintf(stderr,"cannot mix different master tile resolutions. sorry. make sure running gathering program with same tile_count\n");
			exit(EXIT_FAILURE);
		}
		master_tile_x_count = master_tile_x_count_local;
		master_tile_y_count = master_tile_y_count_local;

		//fprintf(stderr, "struct todo\n");
		//exit(1);

		uint8_t tile_count_local = read_thumbs_conf_tilecount( &mds[i0] );
		if(i == argv_start_idx_thumbs_db_names) {
			tile_count = tile_count_local;
		} else {
			if( tile_count != tile_count_local ) {
				fprintf(stderr, "cannot mix different thumbs db tile_count resolutions. sorry. make sure initialize your thumbs dbs with the same tile_count\n");
				exit(EXIT_FAILURE);
			}	
		}
		if(debug)	fprintf(stderr,"tile_count read from parameter %i (%s) => %i\n", i, argv[i], tile_count_local);
	}

	uint32_t total_master_tile_count = master_tile_x_count * master_tile_y_count;
	if(debug) fprintf(stderr,"master_tile_count:%i*%i=%i\n", master_tile_x_count, master_tile_y_count, total_master_tile_count);
	if(debug) fprintf(stderr, "tile_count:%i\n", tile_count);
	struct result *canidates = malloc(total_master_tile_count * sizeof( struct result ));
	if( canidates == NULL ) {
		fprintf(stderr, "cannot allocate memory\n");
		exit(EXIT_FAILURE);
	}

	for(uint32_t i=0;i<total_master_tile_count;i++) {
		canidates[i].thumbs_db_name = NULL;
		canidates[i].index=0;
		canidates[i].score=LLONG_MAX;
		canidates[i].off_x=0;
		canidates[i].off_y=0;
		canidates[i].sortorder=i;
		memset(canidates[i].thumbs_db_filenames,0,MAX_FILENAME_LEN);
		memset(canidates[i].hash,0,MD5_DIGEST_LENGTH);
		memset(canidates[i].temp_filename,0,MAX_TEMP_FILENAME_LEN);
	}

	for(uint32_t i=argv_start_idx_thumbs_db_names;i<argv_end_idx_thumbs_db_names;i++) {

		
		fprintf(stderr,"load result file %s\n", argv[i]);

		FILE *result_file = fopen(mp.dest_result_filename, "rb");
		if( result_file == NULL) {
			fprintf(stderr, "result file (%s) could not be opened\n", mp.dest_result_filename);
			exit(EXIT_FAILURE);
		}
		
		struct stat st;
		stat(mp.dest_result_filename, &st);
		char buf[st.st_size];
		memset(buf, 0 , st.st_size);

		size_t freads_read = fread(buf, 1, st.st_size, result_file);
		if(st.st_size != freads_read) {
			fprintf(stderr, "read data than it expected (%s)", mp.dest_result_filename);
			exit(EXIT_FAILURE);
		}
	
		uint32_t total_master_tile_count0 = master_tile_x_count * master_tile_y_count;
		struct result *canidates0 = malloc( total_master_tile_count * sizeof(struct result));
		if(canidates0 == NULL) {
			fprintf(stderr, "cannot allocate memory for temporary data structure\n");
			exit(EXIT_FAILURE);
		}

		for(uint32_t i=0;i<total_master_tile_count;i++) {
			canidates0[i].thumbs_db_name = NULL;
			canidates0[i].index=0;
			canidates0[i].score=LLONG_MAX;
			canidates0[i].off_x=0;
			canidates0[i].off_y=0;
			canidates0[i].sortorder=i;
			memset(canidates0[i].hash,0,MD5_DIGEST_LENGTH);
			memset(canidates0[i].thumbs_db_filenames,0,MAX_FILENAME_LEN);
			memset(canidates0[i].temp_filename,0,MAX_TEMP_FILENAME_LEN);
		}



  	int j=0;
  	char *ptr;
  	ptr = strtok(buf, "\n\t");
  	while(ptr != NULL) {
			ptr = strtok(NULL, "\n\t"); if(ptr==NULL)break;
			canidates0[j].thumbs_db_name = argv[i];
			canidates0[j].index = atoll( ptr ); ptr = strtok(NULL, "\n\t"); if(ptr==NULL)break;
			canidates0[j].score = atoll( ptr ); ptr = strtok(NULL, "\n\t"); if(ptr==NULL)break;
			canidates0[j].off_x = atoi( ptr );  ptr = strtok(NULL, "\n\t"); if(ptr==NULL)break;
			canidates0[j].off_y = atoi( ptr );  ptr = strtok(NULL, "\n\t"); if(ptr==NULL)break;
			j++;
		}
	
		for(uint32_t i=0;i<total_master_tile_count;i++) {
			if(canidates0[i].score < canidates[i].score ) {
				//printf("i %i better score %lli <-> %lli\n", i, canidates_score0[i], canidates_score[i]);
				canidates[i].thumbs_db_name = canidates0[i].thumbs_db_name;
				canidates[i].index = canidates0[i].index;
				canidates[i].score = canidates0[i].score;
				canidates[i].off_x = canidates0[i].off_x;
				canidates[i].off_y = canidates0[i].off_y;
			} 
//			else { 				printf("i %i WORSE score %lli <-> %lli\n", i, canidates_score0[i], canidates_score[i]); 			}
		}
		free(canidates0);
	}

	
	fprintf(stderr,"data is loaded (%i*%i=%i)\n",master_tile_x_count,master_tile_y_count,total_master_tile_count);

	qsort( canidates, total_master_tile_count, sizeof(struct result), cmpfunc);
	fprintf(stderr, "sort arr\n");
	
	if(debug)
		for(uint32_t i=0;i<total_master_tile_count;i++) {
			printf("%i	%li	%li	%i	%i	%s\n",
			canidates[i].sortorder, 
			canidates[i].index, 
			canidates[i].score, 
			canidates[i].off_x, 
			canidates[i].off_y,
			canidates[i].thumbs_db_name);
  	}
	
	char *thumbs_db_name="";
	FILE *thumbs_db_file = NULL;
	FILE *thumbs_db_hash = NULL;

	uint64_t j=0;
	uint64_t total_score = 0;
	char puffer[MAX_FILENAME_LEN];
	
	for(uint32_t i=0;i<total_master_tile_count;i++) {
		if(strcmp(canidates[i].thumbs_db_name, thumbs_db_name)!=0) {
			if(thumbs_db_file != NULL) fclose(thumbs_db_file);
			if(thumbs_db_hash != NULL) fclose(thumbs_db_hash);

			thumbs_db_name = canidates[i].thumbs_db_name;

			int found = 0;
			uint32_t i_=0;
			struct mosaik2_database_struct_p *p;
			for(int i0=0;i0< argv_end_idx_thumbs_db_names - argv_start_idx_thumbs_db_names;i0++) {
				if(strcmp(canidates[i].thumbs_db_name, mds[i0].thumbs_db_name)==0) {
					found = 1;
					i_=i0;
					break;
				}
			}
			if( found == 0) {
				//should not happen
				fprintf(stderr,"unable to refind mosaik2_database_structure for %s\n", canidates[i].thumbs_db_name);
				exit(EXIT_FAILURE);
			}

			j=0;

			thumbs_db_file = fopen(mds[i_].filenames_filename, "r");
			if( thumbs_db_file == NULL) {
				fprintf(stderr, "thumbs db file (%s) could not be opened\n", mds[i_].filenames_filename);
				exit(EXIT_FAILURE);
			}

			thumbs_db_hash = fopen(mds[i_].filehashes_filename, "r");
			if(thumbs_db_hash == NULL) {
				fprintf(stderr, "thumbs db file (%s) could not be opened\n", mds[i_].filehashes_filename);
				exit(EXIT_FAILURE);
			} else {
				fprintf(stderr, "enrich data from results file %s\n", mds[i_].thumbs_db_name);
			}
		}


	//	memset(puffer,0,MAX_FILENAME_LEN);
		for(uint64_t len=canidates[i].index;j<=len;j++) {
			char *freads_read = fgets(puffer, MAX_FILENAME_LEN, thumbs_db_file);
			if(freads_read == NULL) {
				fprintf(stderr, "read less data than it expected");
				exit(EXIT_FAILURE);
			}
		}
		strncpy(canidates[i].thumbs_db_filenames,puffer,strlen(puffer));
		if(strlen(puffer)>0)	canidates[i].thumbs_db_filenames[strlen(puffer)-1]=0;
		
		
		fseek(thumbs_db_hash, MD5_DIGEST_LENGTH*canidates[i].index,SEEK_SET);
		size_t read = fread(canidates[i].hash, 1, MD5_DIGEST_LENGTH, thumbs_db_hash);
		if(read!=MD5_DIGEST_LENGTH) {
			fprintf(stderr, "did not read enough hash data, expected %i at idx:%li position:%li but got %li\n",  MD5_DIGEST_LENGTH,canidates[i].index, MD5_DIGEST_LENGTH*canidates[i].index,read);
			exit(EXIT_FAILURE);
		}
		char *buf;
		size_t sz;
		sz = snprintf(NULL, 0, "%s/.mosaik2/mosaik2.%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x",
		home,
		canidates[i].hash[0],		canidates[i].hash[1],		canidates[i].hash[2],		canidates[i].hash[3],
		canidates[i].hash[4],		canidates[i].hash[5],		canidates[i].hash[6],		canidates[i].hash[7],
		canidates[i].hash[8],		canidates[i].hash[9],		canidates[i].hash[10],	canidates[i].hash[11],
		canidates[i].hash[12],	canidates[i].hash[13],	canidates[i].hash[14],	canidates[i].hash[15]);
		//canidates[i].hash[16],	canidates[i].hash[17],	canidates[i].hash[18],	canidates[i].hash[19]);
		buf = (char *)malloc(sz + 1); /* make sure you check for != NULL in real code */
		if(buf==NULL) {
			fprintf(stderr, "cannot allocate memory\n");
			exit(EXIT_FAILURE);
		}
		snprintf(buf, sz+1, "%s/.mosaik2/mosaik2.%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x",
		home,
		canidates[i].hash[0],		canidates[i].hash[1],		canidates[i].hash[2],		canidates[i].hash[3],
		canidates[i].hash[4],		canidates[i].hash[5],		canidates[i].hash[6],		canidates[i].hash[7],
		canidates[i].hash[8],		canidates[i].hash[9],		canidates[i].hash[10],	canidates[i].hash[11],
		canidates[i].hash[12],	canidates[i].hash[13],	canidates[i].hash[14],	canidates[i].hash[15]);
//		canidates[i].hash[16],	canidates[i].hash[17],	canidates[i].hash[18],	canidates[i].hash[19]);
		strncpy(canidates[i].temp_filename,buf,strlen(buf));
		free(buf);
		total_score += canidates[i].score;
	}
	fclose(thumbs_db_file);
	fclose(thumbs_db_hash);

	fprintf(stderr,"data enriched (filenames and hashes)\n");

	qsort( canidates, total_master_tile_count, sizeof(struct result), cmpfunc_back);
	/*for(uint32_t i=0;i<total_master_tile_count;i++) {
		printf("%li	%lli	%lli	%i	%i	%s	%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x	%s\n",
		canidates[i].sortorder, 
		canidates[i].index, 
		canidates[i].score, 
		canidates[i].off_x, 
		canidates[i].off_y,
		canidates[i].thumbs_db_name,
		canidates[i].md5[0],canidates[i].md5[1],canidates[i].md5[2],canidates[i].md5[3],canidates[i].md5[4],canidates[i].md5[5],canidates[i].md5[6],canidates[i].md5[7],canidates[i].md5[8],canidates[i].md5[9],canidates[i].md5[10],canidates[i].md5[11],canidates[i].md5[12],canidates[i].md5[13],canidates[i].md5[14],canidates[i].md5[15],canidates[i].thumbs_db_filenames);
  }*/

	//download part
	for(uint32_t i=0;i<total_master_tile_count;i++) {

		if( access( canidates[i].temp_filename, F_OK ) == 0 ) {
			printf("%i/%i already exist %s:%li\n", i,total_master_tile_count,canidates[i].thumbs_db_name,canidates[i].index );
			// TODO check hash
			continue;
		}


	if(is_file_local( canidates[i].thumbs_db_filenames )) {

		//TODO
		if(local_cache==1) {
		fprintf(stderr,"%i/%i copy %s:%li %s", i,total_master_tile_count, canidates[i].thumbs_db_name,canidates[i].index,canidates[i].thumbs_db_filenames );
		File_Copy( canidates[i].thumbs_db_filenames, canidates[i].temp_filename);
		fprintf(stderr,".\n");
		} else {
			fprintf(stderr,"%i/%i symlink %s:%li %s", i,total_master_tile_count, canidates[i].thumbs_db_name,canidates[i].index,canidates[i].thumbs_db_filenames );
			int simlink = symlink(canidates[i].thumbs_db_filenames, canidates[i].temp_filename);
			if(simlink != 0) {
				fprintf(stderr, "error creating symlink %s for %s\n", canidates[i].temp_filename, canidates[i].thumbs_db_filenames);
				exit(EXIT_FAILURE);
			}
			fprintf(stderr,".\n");
		}

	} else {
		printf("%i/%i downloading %s:%li %s\n", i,total_master_tile_count, canidates[i].thumbs_db_name,canidates[i].index,canidates[i].thumbs_db_filenames );

		CURL *curl_handle;
  	FILE *pagefile;
		 curl_global_init(CURL_GLOBAL_ALL);
 
  curl_handle = curl_easy_init();
	//printf("[%s]\n", canidates[i].thumbs_db_filenames);
  curl_easy_setopt(curl_handle, CURLOPT_USERAGENT, "mosaik2");
  curl_easy_setopt(curl_handle, CURLOPT_URL, canidates[i].thumbs_db_filenames);
  curl_easy_setopt(curl_handle, CURLOPT_VERBOSE, 0L);
  curl_easy_setopt(curl_handle, CURLOPT_NOPROGRESS, 1L);
  curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, write_data);
  pagefile = fopen(canidates[i].temp_filename, "wb");
  if(pagefile) {
    curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, pagefile);
    curl_easy_perform(curl_handle);
    fclose(pagefile);
  }
  curl_easy_cleanup(curl_handle);
  curl_global_cleanup();
	}

	}
 printf("join mosaik2 for real\n");
 gdImagePtr out_im = gdImageCreateTrueColor(dest_tile_width*master_tile_x_count,dest_tile_width*master_tile_y_count);
	if(out_im == NULL) {
		fprintf(stderr, "could not create image object\n");
		exit(EXIT_FAILURE);
	}
	gdImageSetInterpolationMethod(out_im,	GD_GAUSSIAN );

  FILE *out = fopen(dest_filename, "wb");
  FILE *html_out = fopen(mp.dest_html_filename, "wb");
	FILE *src_out = fopen(mp.dest_src_filename, "wb");

	if(out == NULL || html_out == NULL || src_out == NULL) {
		fprintf(stderr, "output file (%s) could not be opened\n", dest_filename);
		exit(EXIT_FAILURE);
	}
 
	fprintf(html_out, "<html><body><table>");
	gdImagePtr im;
 for(int y=0;y<master_tile_y_count;y++) {
		fprintf(html_out, "<tr>");
    for(int x=0;x<master_tile_x_count;x++) {
			int master_tile_idx = y*master_tile_x_count+x;
      
			printf("%i/%i %s from %s\n",master_tile_idx,total_master_tile_count,canidates[master_tile_idx].temp_filename,canidates[master_tile_idx].thumbs_db_filenames);
			im = myLoadPng(canidates[master_tile_idx].temp_filename, canidates[master_tile_idx].thumbs_db_filenames);

			fprintf(html_out, "<td");
			if(im==NULL) {
				fprintf(stderr,"continue\nEXIT\n");exit(99);
				fprintf(html_out, " class='err'>");

				char *url = canidates[master_tile_idx].thumbs_db_filenames;
				char url_file[1000];
				char url_thumb[1000];

				memset(url_file, '\0', 1000);
				memset(url_thumb, '\0', 1000);

				int is_file_commons = is_file_wikimedia_commons(url);
				if(is_file_commons==1) {
					fprintf(stderr, "1\n");
					get_wikimedia_thumb_url(url, "100", url_thumb, strlen(url_thumb));
					fprintf(stderr, "2\n");
					get_wikimedia_file_url(url, url_file, strlen(url_file));
					fprintf(stderr, "3\n");
					fprintf(html_out, "<a href='%s'>", url_file);
				} 
				fprintf(html_out, "<img src='%s' width='%i' height='%i'/>", url_thumb,dest_tile_width,dest_tile_width);
				if(is_file_commons==1) {
					fprintf(html_out, "</a>\n");
					fprintf(src_out,"%i: %s\n", master_tile_idx, url);
				} else {
					fprintf(src_out,"%i: %s\n", master_tile_idx, url_thumb);
				}
				fprintf(html_out, "</td>");
				
				continue;
			}

				fprintf(html_out, ">");
			char *url = canidates[master_tile_idx].thumbs_db_filenames;
			char url_thumb[1000];
			char url_file[1000];
				memset(url_file, '\0', 1000);
				memset(url_thumb, '\0', 1000);
			int is_file_commons = is_file_wikimedia_commons(url);
			if(debug) fprintf(stderr,"is_file_commons:%i\n", is_file_commons);
			if(debug && is_file_commons==1) {
				fprintf(stderr, "url:[%s] ", url);

				get_wikimedia_thumb_url(url, "100", url_thumb, 1000);
				if(debug) fprintf(stderr, " [%s]", url_thumb);
				if(debug) fprintf(stderr, "get file url:");
				get_wikimedia_file_url(url, url_file, 1000);
				if(debug) fprintf(stderr, " [%s]\n", url_file);
				fprintf(html_out, "<a href='%s'>", url_file);
				fprintf(html_out, "<img src='%s' width='%i' height='%i'/>", url_thumb, dest_tile_width, dest_tile_width);
			
				fprintf(html_out, "</a>\n");
				fprintf(src_out,"%i: %s\n", master_tile_idx, url_file);

				//free(url_thumb);
				//free(url_file);
			
			} else {
				fprintf(html_out, "<img src='%s' width='%i' height='%i'/>", url, dest_tile_width, dest_tile_width);
				
				fprintf(src_out,"%i: %s\n", master_tile_idx, url);
				if(x==master_tile_x_count-1) {
					fprintf(src_out, "\n");
				}

			}


			fprintf(html_out, "</td>");
			
		gdImageSetInterpolationMethod(im,	GD_GAUSSIAN );

			uint32_t width = gdImageSX(im);
			uint32_t height = gdImageSY(im);
			
			int short_dim = width<height?width:height;
			int long_dim  = width<height?height:width;
			 
			int pixel_per_tile = ( short_dim - (short_dim % tile_count) ) / tile_count;
			
			int tile_x_count;
			int tile_y_count;
			 
			if(short_dim == width){
			  tile_x_count = tile_count;
			  tile_y_count = height / pixel_per_tile;
			} else {
			  tile_x_count = width / pixel_per_tile;
			  tile_y_count = tile_count;
			}
 
			int offset_x = ((width - tile_x_count * pixel_per_tile)/2);
 			int offset_y = ((height - tile_y_count * pixel_per_tile)/2);

 			int lx = offset_x + pixel_per_tile * tile_x_count;
 			int ly = offset_y + pixel_per_tile * tile_y_count;


			/*dst	The destination image.
			src	The source image.
			dstX	The x-coordinate of the upper left corner to copy to.
			dstY	The y-coordinate of the upper left corner to copy to.
			srcX	The x-coordinate of the upper left corner to copy from.
			srcY	The y-coordinate of the upper left corner to copy from.
			dstW	The width of the area to copy to.
			dstH	The height of the area to copy to.
			srcW	The width of the area to copy from.
			srcH	The height of the area to copy from.*/


			int dstX = x * dest_tile_width;
			int dstY = y * dest_tile_width;
			int srcX = offset_x+canidates[master_tile_idx].off_x*pixel_per_tile;
			int srcY = offset_y+canidates[master_tile_idx].off_y*pixel_per_tile;
			int dstW = dest_tile_width;
			int dstH = dest_tile_width;
			int srcW = tile_count*pixel_per_tile;
			int srcH = tile_count * pixel_per_tile;

//			fprintf(stderr,"dstX %i dstY %i srcXY %i %i dest %i %i srcWH %i %i\n" , dstX,dstY,srcX,srcY,dstW, dstH,srcW,srcH);
      /*gdImageCopyResized ( out_im,
      	im,
        x*dest_tile_width,//dstX
        y*dest_tile_width,//()    int   dstY,
        offset_x+canidates[master_tile_idx].off_x*pixel_per_tile,//thumbs_offx[t],//int  srcX,
        offset_y+canidates[master_tile_idx].off_y*pixel_per_tile,//thumbs_offy[t],//int  srcY,
        dest_tile_width,//int   dstW,
        dest_tile_width,//int   dstH,
        tile_count*pixel_per_tile,//thumbs_x[t],//int   srcW,
        tile_count*pixel_per_tile//thumbs_y[t]//int  srcH  
      );*/
      gdImageCopyResized ( out_im,
      	im,
        dstX,
        dstY,
        srcX,
        srcY,
        dstW,
        dstH,
        srcW,
        srcH  
      );
  
    gdImageDestroy(im);
		}
			fprintf(html_out, "</tr>");
	}

	fprintf(html_out, "</table><p>total score:%li<br/>score per tile:%f</p></body></html>", total_score, (total_score/(total_master_tile_count*tile_count*tile_count*1.0)));

//	fprintf(stderr,"alpha blending flag:%i\n",out_im->alphaBlendingFlag);
//	out_im->alphaBlendingFlag=0;
	if(debug) fprintf(stderr,"writing file to disk %i %i \n", out_im==NULL, out==NULL);
  if(ft == FT_PNG) {
  	gdImagePng(out_im, out);
  } else if( ft == FT_JPEG ) {
		
    gdImageJpeg(out_im, out, 100);
  }
	fclose(html_out);
	fclose(src_out);
  fclose(out);
	free(canidates);
	fprintf(stdout, "total score: %li\nscore per tile:%f\n", total_score, (total_score/(total_master_tile_count*tile_count*tile_count*1.0)));

}
