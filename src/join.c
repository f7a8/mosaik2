//TODO check is db older than the result file?
//TODO unique_tiles

/*
    _
   (_)      _
   | | ___ (_) _ _
  _/ |/ _ \| || ' \
 |__/ \___/|_||_||_|
*/

#include "libmosaik2.h"

#ifdef HAVE_CURL
//for curl writing
static size_t write_data(void *ptr, size_t size, size_t nmemb, void *stream) {
  size_t written = fwrite(ptr, size, nmemb, (m2file )stream);
  return written;
}
#endif

void inject_exif_comment(m2file out, off_t out_file_size, char *comment, size_t comment_len);

int mosaik2_join(mosaik2_arguments *args) {

	m2name dest_filename = args->dest_image;
	int dest_tile_width = args->pixel_per_tile;
	int unique_tile = args->duplicate_reduction;
	int local_cache = args->symlink_cache;
	int debug = args->verbose;

	char * home = getenv("HOME");
	char *pwd = getenv("PWD");

	int ft =  mosaik2_project_check_dest_filename( dest_filename );

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


	
	uint8_t argv_start_idx_thumbs_db_names = 0;
	uint8_t argv_end_idx_thumbs_db_names = args->mosaik2dbs_count;	
	int primary_tile_x_count=0;// atoi(argv[2]);
	int primary_tile_y_count=0;// = atoi(argv[3]);

	
	uint8_t database_image_resolution = 0;
	uint64_t candidates_count = 0;


	// init
if(debug) fprintf(stderr, "init\n");
	mosaik2_project mp;
	mosaik2_database mds[argv_end_idx_thumbs_db_names - argv_start_idx_thumbs_db_names];

	//read primary tile n count from file
	for(uint32_t i=argv_start_idx_thumbs_db_names, i0=0; i<argv_end_idx_thumbs_db_names; i++,i0++) {
		if(debug) fprintf( stderr, "check %i %s\n", i, args->mosaik2dbs[i]);
		
		init_mosaik2_database(&mds[i0], args->mosaik2dbs[i]);
		mosaik2_database_check(&mds[i0]);
		mosaik2_database_read_database_id(&mds[i0]);
		candidates_count += read_thumbs_db_count(&mds[i0]);

		init_mosaik2_project(&mp, mds[i0].id, dest_filename);

		if(debug)
			fprintf(stderr,"thumbs_db_name:%s,dest_filename:%s,primarytiledis:%s\n", args->mosaik2dbs[i],dest_filename,mp.dest_primarytiledims_filename);

		m2file primarytiledims_file = m_fopen(mp.dest_primarytiledims_filename, "r");
		if(debug) {
			fprintf(stderr, "primarytiledims file loaded\n");
		}

		off_t primarytiledims_filesize = get_file_size(mp.dest_primarytiledims_filename);
		char buf[primarytiledims_filesize];
		memset(buf, 0, primarytiledims_filesize);
		m_fread(buf, primarytiledims_filesize, primarytiledims_file);

		int primary_tile_x_count_local = 0;
		int primary_tile_y_count_local = 0;
	
		char *ptr;
		ptr=strtok(buf,"\n\t");if(ptr==NULL){fprintf(stderr,"error while parsing primary tile dims file\n");exit(EXIT_FAILURE);}
		primary_tile_x_count_local = atoi( ptr );
		ptr=strtok(NULL,"\n\t");if(ptr==NULL){fprintf(stderr,"error while parsing primary tile dims file\n");exit(EXIT_FAILURE);}
		primary_tile_y_count_local = atoi( ptr );
		m_fclose(primarytiledims_file);

		if( i > argv_start_idx_thumbs_db_names
				&& (primary_tile_x_count != primary_tile_x_count_local
				|| primary_tile_y_count != primary_tile_y_count_local)) {
			fprintf(stderr,"cannot mix different primary tile resolutions. sorry. make sure running gathering program with same database_image_resolution\n");
			exit(EXIT_FAILURE);
		}
		primary_tile_x_count = primary_tile_x_count_local;
		primary_tile_y_count = primary_tile_y_count_local;

		uint8_t database_image_resolution_local = read_database_image_resolution( &mds[i0] );
		if(i == argv_start_idx_thumbs_db_names) {
			database_image_resolution = database_image_resolution_local;
		} else {
			if( database_image_resolution != database_image_resolution_local ) {
				fprintf(stderr, "cannot mix different database_image_resolution. sorry. make sure initialize your mosaik2 dbs with the same database_image_resolution\n");
				exit(EXIT_FAILURE);
			}	
		}
		if(debug)	fprintf(stderr,"database_image_resolution read from parameter %i (%s) => %i\n", i, args->mosaik2dbs[i], database_image_resolution_local);
	}

	uint32_t total_primary_tile_count = primary_tile_x_count * primary_tile_y_count;
	if(debug) fprintf(stderr,"primary_tile_count:%i*%i=%i\n", primary_tile_x_count, primary_tile_y_count, total_primary_tile_count);
	if(debug) fprintf(stderr, "database_image_resolution:%i\n", database_image_resolution);
	mosaik2_project_result *candidates = m_calloc(total_primary_tile_count, sizeof( mosaik2_project_result ));

	for(uint32_t i=0;i<total_primary_tile_count;i++) {
		candidates[i].costs=FLT_MAX;
		candidates[i].sortorder=i;
	}

	for(uint32_t i=argv_start_idx_thumbs_db_names,i0=0;i<argv_end_idx_thumbs_db_names;i++,i0++) {

		init_mosaik2_project(&mp, mds[i0].id, dest_filename);

		m2file result_file = m_fopen(mp.dest_result_filename, "rb");
		if(args->verbose)fprintf(stderr,"load result file %s (%s)\n", args->mosaik2dbs[i], mp.dest_result_filename);
		
		off_t dest_result_filesize = get_file_size(mp.dest_result_filename);
		char buf[dest_result_filesize];
		memset(buf, 0 , dest_result_filesize);
		m_fread(buf, dest_result_filesize, result_file);
	
		//uint32_t total_primary_tile_count0 = primary_tile_x_count * primary_tile_y_count;
		mosaik2_project_result *candidates0 = m_calloc( total_primary_tile_count, sizeof(mosaik2_project_result));
		for(uint32_t i=0;i<total_primary_tile_count;i++) {
			candidates0[i].costs=FLT_MAX;
			candidates0[i].sortorder=i;
		}

		int j=0;
		char *ptr;
		ptr = strtok(buf, "\n\t");
		while(ptr != NULL) {
			ptr = strtok(NULL, "\n\t"); if(ptr==NULL)break;
			candidates0[j].md = &mds[i];
			candidates0[j].index = atoll( ptr ); ptr = strtok(NULL, "\n\t"); if(ptr==NULL)break;
			candidates0[j].costs = (float) atof( ptr ); ptr = strtok(NULL, "\n\t"); if(ptr==NULL)break;
			candidates0[j].off_x = atoi( ptr );  ptr = strtok(NULL, "\n\t"); if(ptr==NULL)break;
			candidates0[j].off_y = atoi( ptr );  ptr = strtok(NULL, "\n\t"); if(ptr==NULL)break;
			j++;
		}
	
		for(uint32_t i=0;i<total_primary_tile_count;i++) {
			if(candidates0[i].costs < candidates[i].costs ) {
				candidates[i].md = candidates0[i].md;
				candidates[i].index = candidates0[i].index;
				candidates[i].costs = candidates0[i].costs;
				candidates[i].off_x = candidates0[i].off_x;
				candidates[i].off_y = candidates0[i].off_y;
			} 
		}

		free(candidates0);
	}

	if(debug)
		for(uint32_t i=0;i<total_primary_tile_count;i++) {
			fprintf(stderr,"%s:%i\n", candidates[i].md->thumbs_db_name, candidates[i].index);
		}

	
	if(debug)
		fprintf(stderr,"data is loaded (%i*%i=%i)\n",primary_tile_x_count,primary_tile_y_count,total_primary_tile_count);

	qsort( candidates, total_primary_tile_count, sizeof(mosaik2_project_result), cmpfunc);
	if(debug)
		fprintf(stderr, "sort arr\n");
	
	if(debug)
		for(uint32_t i=0;i<total_primary_tile_count;i++) {
			printf("%i	%i	%f	%i	%i	%s\n",
			candidates[i].sortorder, 
			candidates[i].index, 
			candidates[i].costs, 
			candidates[i].off_x, 
			candidates[i].off_y,
			candidates[i].md->thumbs_db_name);
  	}
	
	m2name thumbs_db_name="";
	m2file thumbs_db_file = NULL;
	m2file thumbs_db_hash = NULL;

	uint64_t j=0;
	float total_costs = 0;
	char buffer[MAX_FILENAME_LEN];

	size_t sz = snprintf(NULL, 0, "%s/.mosaik2/mosaik2.hash",home);
	char mkdir_buf[sz+1];
	memset(mkdir_buf,0,sz+1);
	snprintf(mkdir_buf, sz+1, "%s/.mosaik2/mosaik2.hash",home);
	m2ctext mkdir_path = dirname(mkdir_buf);
	if(access(mkdir_path, W_OK)!=0) {
		if(debug) fprintf(stderr, "cache dir (%s) is not writeable, try to mkdir it\n", mkdir_path);
		//not accessible or writeable, try to create dir
		if( mkdir(mkdir_path, S_IRWXU | S_IRGRP | S_IROTH ) != 0) {
			fprintf(stderr, "cache directory (%s) could not be created\n", mkdir_path);
			exit(EXIT_FAILURE);
		}
	}
	
	for(uint32_t i=0;i<total_primary_tile_count;i++) {
		if(strcmp(candidates[i].md->thumbs_db_name, thumbs_db_name)!=0) {
			if(thumbs_db_file != NULL) m_fclose(thumbs_db_file);
			if(thumbs_db_hash != NULL) m_fclose(thumbs_db_hash);

			thumbs_db_name = candidates[i].md->thumbs_db_name;

			int found = 0;
			uint32_t i_=0;
			for(int i0=0;i0< argv_end_idx_thumbs_db_names - argv_start_idx_thumbs_db_names;i0++) {
				if(strcmp(candidates[i].md->thumbs_db_name, mds[i0].thumbs_db_name)==0) {
					found = 1;
					i_=i0;
					break;
				}
			}
			if( found == 0) {
				//should not happen
				fprintf(stderr,"unable to refind mosaik2_database for %s\n", candidates[i].md->thumbs_db_name);
				exit(EXIT_FAILURE);
			}

			j=0;

			thumbs_db_file = m_fopen(mds[i_].filenames_filename, "r");
			thumbs_db_hash = m_fopen(mds[i_].filehashes_filename, "r");
			if(debug)
				fprintf(stderr, "enrich data from results file %s\n", mds[i_].thumbs_db_name);
		}


		for(m2elem len=candidates[i].index;j<=len;j++) {
			m_fgets(buffer, MAX_FILENAME_LEN, thumbs_db_file);
		}
		strncpy(candidates[i].thumbs_db_filenames,buffer,strlen(buffer));
		if(strlen(buffer)>0)	candidates[i].thumbs_db_filenames[strlen(buffer)-1]=0;
		
		
		m_fseeko(thumbs_db_hash, MD5_DIGEST_LENGTH*candidates[i].index,SEEK_SET);
		m_fread(candidates[i].hash, MD5_DIGEST_LENGTH, thumbs_db_hash);

		size_t sz;
		sz = snprintf(NULL, 0, "%s/.mosaik2/mosaik2.%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x",
		home,
		candidates[i].hash[0], candidates[i].hash[1], candidates[i].hash[2], candidates[i].hash[3],
		candidates[i].hash[4], candidates[i].hash[5], candidates[i].hash[6], candidates[i].hash[7],
		candidates[i].hash[8], candidates[i].hash[9], candidates[i].hash[10], candidates[i].hash[11],
		candidates[i].hash[12], candidates[i].hash[13],	candidates[i].hash[14],	candidates[i].hash[15]);

		char buf[sz+1];
		snprintf(buf, sz+1, "%s/.mosaik2/mosaik2.%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x",
		home,
		candidates[i].hash[0], candidates[i].hash[1], candidates[i].hash[2], candidates[i].hash[3],
		candidates[i].hash[4], candidates[i].hash[5], candidates[i].hash[6], candidates[i].hash[7],
		candidates[i].hash[8], candidates[i].hash[9], candidates[i].hash[10], candidates[i].hash[11],
		candidates[i].hash[12], candidates[i].hash[13], candidates[i].hash[14], candidates[i].hash[15]);
		strncpy(candidates[i].temp_filename,buf,strlen(buf));


		total_costs += candidates[i].costs;
	}
	m_fclose(thumbs_db_file);
	m_fclose(thumbs_db_hash);

	if(args->verbose)fprintf(stderr,"data enriched (filenames and hashes)\n");

	qsort( candidates, total_primary_tile_count, sizeof(mosaik2_project_result), cmpfunc_back);

	//download part
	for(uint32_t i=0;i<total_primary_tile_count;i++) {
		if( access( candidates[i].temp_filename, F_OK ) == 0 ) {
			if(!args->quiet)printf("%i/%i already exist %s:%i\n", i,total_primary_tile_count,candidates[i].md->thumbs_db_name,candidates[i].index );
			// TODO check hash
			continue;
		}
		/*if(errno == ENOENT) { //dangeling symlink
			fprintf(stderr,"%i/%i found dangling symbolic link %s for %s:%li %s\nthose bad symlinks can be removed by `find ~/.mosaik2/ -xtype l -delete`\n", i,total_primary_tile_count, candidates[i].temp_filename, candidates[i].md->thumbs_db_name,candidates[i].index,candidates[i].thumbs_db_filenames );
			fprintf(stderr,"those bad symlinks can be removed by `find ~/.mosaik2/ -xtype l -delete`\n" );
			fprintf(stderr,"you may want have a look at the invalidate mode\n" );
			exit(EXIT_FAILURE);
		}*/

	if(is_file_local( candidates[i].thumbs_db_filenames )) {

		//TODO
		if(local_cache==1) {
			if(!args->quiet)fprintf(stdout,"%i/%i copy %s:%i %s", i,total_primary_tile_count, candidates[i].md->thumbs_db_name,candidates[i].index,candidates[i].thumbs_db_filenames );
			File_Copy( candidates[i].thumbs_db_filenames, candidates[i].temp_filename);
			if(!args->quiet) fprintf(stdout,".\n");
		} else {

			if(!args->quiet) {
				fprintf(stdout,"%i/%i create symlink %s:%i %s\n",
			        i, total_primary_tile_count,
			        candidates[i].md->thumbs_db_name, candidates[i].index,
			        candidates[i].thumbs_db_filenames );
			}
			
			int absolute = candidates[i].thumbs_db_filenames[0] == '/' ;

				int target_len = absolute ? strlen(candidates[i].thumbs_db_filenames) :  strlen(pwd) + 1 + strlen(candidates[i].thumbs_db_filenames);
				char target[target_len+1];
				memset(target, 0, target_len+1);
				if(absolute)
					strncat(target, candidates[i].thumbs_db_filenames, target_len);
				else {
					strncat(target, pwd, strlen(pwd));
					strcat(target, "/");
					strncat(target, candidates[i].thumbs_db_filenames, strlen(candidates[i].thumbs_db_filenames));
				}

				int simlink = symlink(target, candidates[i].temp_filename);
				if(simlink != 0) {
					fprintf(stderr, "error creating symlink %s for %s\n", candidates[i].temp_filename, target);
					perror("error message =>");
					exit(EXIT_FAILURE);
				}
			}

		} else {
#ifdef HAVE_CURL
			printf("%i/%i downloading %s:%i %s\n", i,total_primary_tile_count, candidates[i].md->thumbs_db_name,candidates[i].index,candidates[i].thumbs_db_filenames );

			CURL *curl_handle;
			m2file pagefile;
			curl_global_init(CURL_GLOBAL_ALL);

			curl_handle = curl_easy_init();
			//printf("[%s]\n", candidates[i].thumbs_db_filenames);
			curl_easy_setopt(curl_handle, CURLOPT_USERAGENT, "mosaik2");
			curl_easy_setopt(curl_handle, CURLOPT_URL, candidates[i].thumbs_db_filenames);
			curl_easy_setopt(curl_handle, CURLOPT_VERBOSE, 0L);
			curl_easy_setopt(curl_handle, CURLOPT_NOPROGRESS, 1L);
			curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, write_data);
			pagefile = m_fopen(candidates[i].temp_filename, "wb");
			if(pagefile) {
				curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, pagefile);
				curl_easy_perform(curl_handle);
				m_fclose(pagefile);
			}
			curl_easy_cleanup(curl_handle);
			curl_global_cleanup();
#else
			fprintf(stderr, "mosaik2 was compiled without curl support, no downloads are possible, only loading images from the local filesystem\n");
#endif
		}
	}
	if(args->verbose)printf("join mosaik2 for real\n");
	gdImagePtr out_im = gdImageCreateTrueColor(dest_tile_width*primary_tile_x_count,dest_tile_width*primary_tile_y_count);
	if(out_im == NULL) {
		fprintf(stderr, "could not create image object\n");
		exit(EXIT_FAILURE);
	}
	gdImageSetInterpolationMethod(out_im, GD_GAUSSIAN );

	m2file out = m_fopen(dest_filename, "wb");
	m2file html_out = m_fopen(mp.dest_html_filename, "wb");
	m2file src_out = m_fopen(mp.dest_src_filename, "wb");

	fprintf(html_out, "<html><body><table>");
	gdImagePtr im;
	for(int y=0;y<primary_tile_y_count;y++) {
		fprintf(html_out, "<tr>");
		for(int x=0;x<primary_tile_x_count;x++) {
			int primary_tile_idx = y*primary_tile_x_count+x;
      
			if(!args->quiet)
				printf("%i/%i %s from %s\n",primary_tile_idx,total_primary_tile_count,candidates[primary_tile_idx].temp_filename,candidates[primary_tile_idx].thumbs_db_filenames);
			im = read_image_from_file(candidates[primary_tile_idx].temp_filename);

			fprintf(html_out, "<td");
			if(im==NULL) {
				fprintf(stderr,"continue\nEXIT\n");exit(99);
				fprintf(html_out, " class='err'>");

				char *url = candidates[primary_tile_idx].thumbs_db_filenames;
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
					fprintf(src_out,"%i: %s\n", primary_tile_idx, url);
				} else {
					fprintf(src_out,"%i: %s\n", primary_tile_idx, url_thumb);
				}
				fprintf(html_out, "</td>");
				
				continue;
			}
			fprintf(html_out, ">");

			char *url = candidates[primary_tile_idx].thumbs_db_filenames;
			char url_thumb[1000];
			char url_file[1000];
			memset(url_file, '\0', 1000);
			memset(url_thumb, '\0', 1000);
			int is_file_commons = is_file_wikimedia_commons(url);
			if(debug)
			       	fprintf(stderr,"is_file_commons:%i\n", is_file_commons);
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
				fprintf(src_out,"%s\n", url_file);

				//free(url_thumb);
				//free(url_file);
			
			} else {
				fprintf(html_out, "<img src='%s' width='%i' height='%i'/>", url, dest_tile_width, dest_tile_width);
				
				fprintf(src_out,"%s\n", url);
				if(x==primary_tile_x_count-1) {
					fprintf(src_out, "\n");
				}
			}
			fprintf(html_out, "</td>");
			
			gdImageSetInterpolationMethod(im, GD_GAUSSIAN );

			uint32_t width = gdImageSX(im);
			uint32_t height = gdImageSY(im);
			
			int short_dim = width<height?width:height;
			//int long_dim  = width<height?height:width;
			 
			int pixel_per_tile = ( short_dim - (short_dim % database_image_resolution) ) / database_image_resolution;
			
			int tile_x_count;
			int tile_y_count;
			 
			if(short_dim == width){
			  tile_x_count = database_image_resolution;
			  tile_y_count = height / pixel_per_tile;
			} else {
			  tile_x_count = width / pixel_per_tile;
			  tile_y_count = database_image_resolution;
			}
 
			int offset_x = ((width - tile_x_count * pixel_per_tile)/2);
 			int offset_y = ((height - tile_y_count * pixel_per_tile)/2);

 			//int lx = offset_x + pixel_per_tile * tile_x_count;
 			//int ly = offset_y + pixel_per_tile * tile_y_count;


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
			int srcX = offset_x+candidates[primary_tile_idx].off_x*pixel_per_tile;
			int srcY = offset_y+candidates[primary_tile_idx].off_y*pixel_per_tile;
			int dstW = dest_tile_width;
			int dstH = dest_tile_width;
			int srcW = database_image_resolution*pixel_per_tile;
			int srcH = database_image_resolution * pixel_per_tile;

			gdImageCopyResized ( 
					out_im, im,
					dstX, dstY,
					srcX, srcY,
					dstW, dstH,
					srcW, srcH);
  
			gdImageDestroy(im);
		}
		fprintf(html_out, "</tr>");
	}

	fprintf(html_out, "</table><p>total costs:%f<br/>costs per tile:%f</p></body></html>", total_costs, (total_costs/(total_primary_tile_count*database_image_resolution*database_image_resolution*1.0)));

	if(debug)
	       fprintf(stderr,"writing file to disk %i %i \n", out_im==NULL, out==NULL);
	if(ft == FT_PNG) {
		gdImagePng(out_im, out);
	} else if( ft == FT_JPEG ) {
		gdImageJpeg(out_im, out, 100);
	}

	gdImageDestroy(out_im);
	m_fclose(html_out);
	m_fclose(src_out);
	m_fclose(out);
	float costs = total_costs / (total_primary_tile_count*database_image_resolution*database_image_resolution*1.0);

	out = m_fopen(dest_filename, "r+");
	char comment[100];
	memset(comment, 0, 100);	
	snprintf(comment, 100, "mosaik2 (%ix%i from %li -r%i) => %f", primary_tile_x_count, primary_tile_y_count, candidates_count, database_image_resolution, costs);
	off_t out_file_size = get_file_size(dest_filename);
	inject_exif_comment(out, out_file_size, comment, strlen(comment));
	m_fclose(out);

	free(candidates);
	if(args->quiet != 1)fprintf(stdout, "total candidate costs: %f,\n per tile:%f\n", total_costs, (total_costs/(total_primary_tile_count*database_image_resolution*database_image_resolution*1.0)));

	return 0;
}

void inject_exif_comment(m2file out, off_t out_file_size, char *comment, size_t comment_len) {
		uint32_t buflen = out_file_size < BUFSIZ ? out_file_size : BUFSIZ;
		unsigned char buf[buflen];

		rewind(out);
		//              size   nmeb
		size_t read_nmemb = fread(&buf, buflen, 1, out);
		if( read_nmemb != 1) {
			for(int i=0;i<buflen;i++) {
				fprintf(stderr, "%i ", buf[i]);
				if(i%80==0&&i>0)
					fprintf(stderr, "\n");
			}
			fprintf(stderr, "could not read first %i bytes of already written dest-file\n", buflen);
			exit(EXIT_FAILURE);
		}

		//assumption that in first bufsize is the total exif comment
		//search for offset and length of the comment

		int exif_offset=-1;
		int exif_length=-1;
		for(int i=0;i<buflen-2;i++) {
			if(exif_offset == -1 && buf[i]== 0xFF && buf[i+1] == 0xFE)
				exif_offset = i + 4;
			if(exif_offset != -1 && exif_length == -1 && buf[i] == 0x0A && buf[i+1] == 0xFF) {
				exif_length = i - exif_offset;
				break;
			}
		}

		if(exif_offset == -1 || exif_length == -1 ) {
			fprintf(stderr, "could not detect exif comment, cannot overwrite it, continue\n");
			return;
		}

		int comment_len0 = comment_len;		
		if(exif_length < comment_len0)
			comment_len0 = exif_length;

		fseeko(out, exif_offset-2, SEEK_SET);
	
		char new_exif_length[2] = {comment_len0 / 256, comment_len0 % 256};

		if( fwrite(new_exif_length, 2, 1, out) != 1) {
			fprintf(stderr, "cannot write exif length\n");
			exit(EXIT_FAILURE);
		}
		if( fwrite(comment, comment_len0, 1, out) != 1) {
			fprintf(stderr, "cannot write exif comment\n");
			exit(EXIT_FAILURE);
		}

		char space[]= {' '};
		for(int i = comment_len; i<exif_length;i++) { // overwrite old exif with white spaces
			if( fwrite(&space, 1, 1, out) != 1 ) {
				perror("could not fill rest exif comment with space\n");
				exit(EXIT_FAILURE);
			}
		}

}
