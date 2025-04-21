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

#ifdef HAVE_LIBCURL
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

	char *home = getenv("HOME");
	//char *pwd = getenv("PWD");

	m2ftype ft =  mosaik2_project_check_dest_filename( dest_filename );

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

	
	m2rezo database_image_resolution = 0;
	uint64_t candidates_count = 0;


	// init
	if(args->verbose) fprintf(stderr, "init\n");

	mosaik2_project mp;
	mosaik2_database mds[argv_end_idx_thumbs_db_names - argv_start_idx_thumbs_db_names];

	//read primary tile n count from file
	for(uint32_t i=argv_start_idx_thumbs_db_names, i0=0; i<argv_end_idx_thumbs_db_names; i++,i0++) {
		if(args->verbose) fprintf( stderr, "check %i %s\n", i, args->mosaik2dbs[i]);
		
		mosaik2_database_init(&mds[i0], args->mosaik2dbs[i]);
		mosaik2_database_check(&mds[i0]);
		mosaik2_database_lock_reader(&mds[i0]);
		mosaik2_database_read_database_id(&mds[i0]);
		candidates_count += mosaik2_database_read_element_count(&mds[i0]);

		

		mosaik2_project_init(&mp, mds[i0].id, dest_filename);
		
		mosaik2_project_read_primary_tile_dims(&mp);

		if( i > argv_start_idx_thumbs_db_names
				&& (primary_tile_x_count != mp.primary_tile_x_count
				||  primary_tile_y_count != mp.primary_tile_y_count)) {
			fprintf(stderr,"cannot mix different primary tile resolutions. sorry. make sure running gathering MODE with same database_image_resolution\n");
			exit(EXIT_FAILURE);
		}

		primary_tile_x_count = mp.primary_tile_x_count;
		primary_tile_y_count = mp.primary_tile_y_count;

		/*off_t primarytiledims_filesize = get_file_size(mp.dest_primarytiledims_filename);
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
			fprintf(stderr,"cannot mix different primary tile resolutions. sorry. make sure running gathering MODE with same database_image_resolution\n");
			
			exit(EXIT_FAILURE);
		}
		primary_tile_x_count = primary_tile_x_count_local;
		primary_tile_y_count = primary_tile_y_count_local;
*/

		m2rezo database_image_resolution_local = mosaik2_database_read_image_resolution( &mds[i0] );
		if(i == argv_start_idx_thumbs_db_names) {
			database_image_resolution = database_image_resolution_local;
		} else {
			if( database_image_resolution != database_image_resolution_local ) {
				fprintf(stderr, "cannot mix different database_image_resolution. "
				"sorry. make sure initialize your mosaik2 dbs with the same database_image_resolution\n");
				exit(EXIT_FAILURE);
			}	
		}
		if(args->verbose)	fprintf(stderr,"database_image_resolution read from "
							"parameter %i (%s) => %i\n", i, args->mosaik2dbs[i],
							database_image_resolution_local);
	}

	uint32_t total_primary_tile_count = primary_tile_x_count * primary_tile_y_count;
	if(args->verbose) fprintf(stderr,"primary_tile_count:%i*%i=%i\n", primary_tile_x_count, primary_tile_y_count, total_primary_tile_count);
	if(args->verbose) fprintf(stderr, "database_image_resolution:%i\n", database_image_resolution);
	mosaik2_project_result *candidates = m_calloc(total_primary_tile_count, sizeof( mosaik2_project_result ));

	for(uint32_t i=0;i<total_primary_tile_count;i++) {
		candidates[i].costs=FLT_MAX;
		candidates[i].sortorder=i;
	}

	for(uint32_t i=argv_start_idx_thumbs_db_names,i0=0;i<argv_end_idx_thumbs_db_names;i++,i0++) {

		mosaik2_project_init(&mp, mds[i0].id, dest_filename);

		m2file result_file = m_fopen(mp.dest_result_filename, "rb");
				
		if(args->verbose)fprintf(stderr,"load result file %s (%s)\n", args->mosaik2dbs[i], mp.dest_result_filename);
		off_t dest_result_filesize = get_file_size(mp.dest_result_filename);
		char buf[dest_result_filesize];
		memset(buf, 0 , dest_result_filesize);
		m_fread(buf, dest_result_filesize, result_file);
	
		//uint32_t total_primary_tile_count0 = primary_tile_x_count * primary_tile_y_count;
		mosaik2_project_result *candidates0 = m_calloc( total_primary_tile_count, sizeof(mosaik2_project_result));
		for(uint32_t l=0;l<total_primary_tile_count;l++) {
			candidates0[l].costs=FLT_MAX;
			candidates0[l].sortorder=l;
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
			candidates0[j].exclude = atoi( ptr );  ptr = strtok(NULL, "\n\t"); if(ptr==NULL)break;
			j++;
		}
	
		for(uint32_t k=0;k<total_primary_tile_count;k++) {
			//simple merge of better images and adapt the exclude bit
			if(candidates0[k].costs < candidates[k].costs || candidates0[k].exclude ) {
				candidates[k].md = candidates0[k].md;
				candidates[k].index = candidates0[k].index;
				candidates[k].costs = candidates0[k].costs;
				candidates[k].off_x = candidates0[k].off_x;
				candidates[k].off_y = candidates0[k].off_y;
				candidates[k].exclude = candidates0[k].exclude;
			} 
		}

		m_free((void**)&candidates0);
	}

	if(args->verbose)
		for(uint32_t i=0;i<total_primary_tile_count;i++) {
			fprintf(stderr,"%s:%i\n", candidates[i].md->thumbs_db_name, candidates[i].index);
		}

	
	if(args->verbose)
		fprintf(stderr,"data is loaded (%i*%i=%i)\n",primary_tile_x_count,primary_tile_y_count,total_primary_tile_count);

	qsort( candidates, total_primary_tile_count, sizeof(mosaik2_project_result), cmpfunc);
	if(args->verbose)
		fprintf(stderr, "sort arr\n");
	
	if(args->verbose)
		for(uint32_t i=0;i<total_primary_tile_count;i++) {
			printf("%i	%i	%f	%i	%i	%s	%i\n",
			candidates[i].sortorder, 
			candidates[i].index,
			candidates[i].costs, 
			candidates[i].off_x, 
			candidates[i].off_y,
			candidates[i].md->thumbs_db_name,
			candidates[i].exclude);
  	}
	


	mosaik2_create_cache_dir();



	m2name thumbs_db_name="";
	m2file filenames_file = NULL;
	m2file filehashes_file = NULL;

	uint64_t j=0;
	float total_costs = 0;
	char buf[MAX_FILENAME_LEN];

	
	for(uint32_t i=0;i<total_primary_tile_count;i++) {

		//when current candidate is from a diffent mosaik2 database
		//open new files
		if(strcmp(candidates[i].md->thumbs_db_name, thumbs_db_name)!=0) {
			if(filenames_file != NULL) m_fclose(filenames_file);
			if(filehashes_file != NULL) m_fclose(filehashes_file);

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

			filenames_file = m_fopen(mds[i_].filenames_filename, "r");
			filehashes_file = m_fopen(mds[i_].filehashes_filename, "r");
			if(args->verbose)
				fprintf(stderr, "enrich data from results file %s\n", mds[i_].thumbs_db_name);
		}

		//TODO VERY INEFFICIENT
		for(m2elem len=candidates[i].index;j<=len;j++) {
			m_fgets(buf, MAX_FILENAME_LEN, filenames_file);
		}
		
		//concat(candidates[i].thumbs_db_filenames,buf);
		concat(candidates[i].origin_filename,buf);
		
		m_fseeko(filehashes_file, MD5_DIGEST_LENGTH*candidates[i].index,SEEK_SET);
		m_fread(candidates[i].hash, MD5_DIGEST_LENGTH, filehashes_file);

		size_t sz;
		sz = snprintf(NULL, 0, "%s/.mosaik2/mosaik2.%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x",
		home,
		candidates[i].hash[0], candidates[i].hash[1], candidates[i].hash[2], candidates[i].hash[3],
		candidates[i].hash[4], candidates[i].hash[5], candidates[i].hash[6], candidates[i].hash[7],
		candidates[i].hash[8], candidates[i].hash[9], candidates[i].hash[10], candidates[i].hash[11],
		candidates[i].hash[12], candidates[i].hash[13],	candidates[i].hash[14],	candidates[i].hash[15]);

		char buf0[sz+1];
		snprintf(buf0, sz+1, "%s/.mosaik2/mosaik2.%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x",
		home,
		candidates[i].hash[0], candidates[i].hash[1], candidates[i].hash[2], candidates[i].hash[3],
		candidates[i].hash[4], candidates[i].hash[5], candidates[i].hash[6], candidates[i].hash[7],
		candidates[i].hash[8], candidates[i].hash[9], candidates[i].hash[10], candidates[i].hash[11],
		candidates[i].hash[12], candidates[i].hash[13], candidates[i].hash[14], candidates[i].hash[15]);

		concat(candidates[i].cache_filename,buf0);

		total_costs += candidates[i].costs;
		candidates[i].is_file_local = is_file_local( candidates[i].origin_filename );
	}
	m_fclose(filenames_file);
	m_fclose(filehashes_file);





	if(args->verbose)fprintf(stderr,"data enriched (filenames and hashes)\n");

	qsort( candidates, total_primary_tile_count, sizeof(mosaik2_project_result), cmpfunc_back);

	//download images from webresources, which are not cached already
	//files in local filesystem arent touched here, as long they are mounted
	for(uint32_t i=0;i<total_primary_tile_count;i++) {

		//check if next file is already cached
		//fprintf(stderr, "is_file_local:%i %s %s\n",candidates[i].is_file_local, candidates[i].origin_filename,candidates[i].cache_filename);

		if( candidates[i].is_file_local == 1 ) {
			if( access( candidates[i].origin_filename, F_OK ) == 0 ) {
				if(!args->quiet)printf("%i/%i local file already exist %s:%i\n", i,total_primary_tile_count,candidates[i].md->thumbs_db_name,candidates[i].index );
				// TODO check hash
			continue;
			} else {
				fprintf(stderr, "local file is not accessable (%s)\nmay have a look at the invalid mode",candidates[i].origin_filename);
				exit(EXIT_FAILURE);
			}
		}
		if( candidates[i].is_file_local == 0 && access( candidates[i].cache_filename, F_OK ) == 0 ) {
			if(!args->quiet)printf("%i/%i chached file already exist %s:%i\n", i,total_primary_tile_count,candidates[i].md->thumbs_db_name,candidates[i].index );
			// TODO check hash
			continue;
		}
		
		
#ifdef HAVE_LIBCURL
			printf("%i/%i downloading %s:%i %s\n", i,total_primary_tile_count, candidates[i].md->thumbs_db_name,candidates[i].index,candidates[i].origin_filename );

			CURL *curl;
			CURLcode res;

			m2file download_file = m_fopen(candidates[i].cache_filename, "w+b");
			curl_global_init(CURL_GLOBAL_ALL);

			curl = curl_easy_init();
			char errbuf[CURL_ERROR_SIZE];
			errbuf[0] = 0;
			if(curl) {
				curl_easy_setopt(curl, CURLOPT_USERAGENT, "mosaik2");
				curl_easy_setopt(curl, CURLOPT_URL, candidates[i].origin_filename);
				curl_easy_setopt(curl, CURLOPT_VERBOSE, 0L);
				curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 1L);
				curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
				curl_easy_setopt(curl, CURLOPT_WRITEDATA, download_file);
				curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, errbuf);
				
				res = curl_easy_perform(curl);
				if(res != CURLE_OK) {
					size_t len = strlen(errbuf);
					fprintf(stderr, "libcurl: (%d) ", res);
					if(len)
						fprintf(stderr, "%s%s", errbuf,
								((errbuf[len - 1] != '\n') ? "\n" : ""));
					else
						fprintf(stderr, "%s\n", curl_easy_strerror(res));
					exit(EXIT_FAILURE);
				}
				m_fclose(download_file);
				curl_easy_cleanup(curl);
				curl_global_cleanup();
			}
#else
			fprintf(stderr, "mosaik2 was compiled without curl support, no downloads are possible, only loading images from the local filesystem\n");
			exit(EXIT_FAILURE);
#endif
		//}
	}

	if(args->verbose) printf("join mosaik2 for real\n");
	gdImagePtr out_im = gdImageCreateTrueColor(dest_tile_width*primary_tile_x_count,dest_tile_width*primary_tile_y_count);
	if(out_im == NULL) {
		fprintf(stderr, "could not create image object\n");
		exit(EXIT_FAILURE);
	}
	gdImageSetInterpolationMethod(out_im, GD_GAUSSIAN );

	m2file out = m_fopen(dest_filename, "wb");
	m2file html_out = m_fopen(mp.dest_html_filename, "wb");
	m2file html2_out = m_fopen(mp.dest_html2_filename, "wb");
	m2file src_out = m_fopen(mp.dest_src_filename, "wb");

	fprintf(html_out, "<html><head><style></style><body><table>");





	int cellsize = 80;
	int offsetx = 2;
	int offsety= 156;
	

	fprintf(html2_out, "<html><head><meta charset=\"UTF-8\"><meta name=\"viewport\" content=\"width=device-width, "
	"initial-scale=1.0\"><style>.container {position: relative; width: %ipx; height: %ix;} "
	".image {width:100%%; height:100%%;} .grid {position:absolute;top:%ipx;left:%ipx;pointer-events:auto;} "
	".cell{position:absolute;width:%ipx;height:%ipx;border:1px solid rgba(0,0,0,0.3);} "
	".hov {background-color:rgba(255,0,0,0.3);border-color:red;}</style></head><body>",
	mp.image_height, mp.image_width, offsetx, offsety, cellsize, cellsize);
	fprintf(html2_out, "<div class=\"container\"><div class=\"grid\"></div><img src=\"%s\" class=\"image\" alt=\"Bild\"></div></div>", "P1010606_33.JPG");
	fprintf(html2_out, "\n<script>\nconst gridContainer = document.querySelector('.grid');\nconst gridWidth = %i;\nconst gridHeight = %i;\nconst cellSize = %i;\n"
	"const offsetx=%i;\nconst offsety=%i;\nconst canvas = document.createElement('canvas');\n"
    "const ctx = canvas.getContext('2d');\nconst candidates=[\n",mp.primary_tile_x_count,
	mp.primary_tile_y_count, cellsize, offsetx,offsety);
	gdImagePtr im;
	m2elem exclude_count = 0;
	for(int y=0;y<primary_tile_y_count;y++) {
		fprintf(html_out, "<tr>");
		for(int x=0;x<primary_tile_x_count;x++) {
			int primary_tile_idx = y*primary_tile_x_count+x;

			if(primary_tile_idx>0) fprintf(html2_out, ",\n");

			
			fprintf(html2_out, "{tile_idx:%i,x:%i,y:%i,element_number:%i,filename:\"%s\",costs:%f,offx:%i,offy:%i,excluded:%i}",
					primary_tile_idx,x,y, candidates[primary_tile_idx].index, candidates[primary_tile_idx].origin_filename, 
					candidates[primary_tile_idx].costs, candidates[primary_tile_idx].off_x,
					candidates[primary_tile_idx].off_y, candidates[primary_tile_idx].exclude);

			if ( candidates[primary_tile_idx].exclude){
				exclude_count++;
				fprintf(html_out, "<td title=\"%ix%i=%i\"></td>", x,y,primary_tile_idx);
				fprintf(src_out,"%i: EXCLUDED\n", primary_tile_idx);
				if(x==primary_tile_x_count-1) {
					fprintf(src_out, "\n");
				}
				if(!args->quiet)
				printf("%i/%i EXCLUDED\n",primary_tile_idx,total_primary_tile_count);
				
				// resize no image
				continue;
			}
      
			if(candidates[primary_tile_idx].is_file_local) {
				if(!args->quiet) {
					printf("%i/%i from local file %s\n",primary_tile_idx,total_primary_tile_count,candidates[primary_tile_idx].origin_filename);
				}
				im = read_image_from_file(candidates[primary_tile_idx].origin_filename);
			} else {
				if(!args->quiet) {
					printf("%i/%i %s from chached file %s\n",primary_tile_idx,total_primary_tile_count,
					candidates[primary_tile_idx].origin_filename,
					candidates[primary_tile_idx].cache_filename);
				}

				im = read_image_from_file(candidates[primary_tile_idx].cache_filename);
			}

			fprintf(html_out, "<td title=\"%ix%i=%i\"",x,y,primary_tile_idx);
			
			if(im==NULL) {
				fprintf(stderr,"continue\nEXIT\n");exit(99);
				fprintf(html_out, " class='err'>");

				
				fprintf(html_out, "<img src='%s' width='%i' height='%i'/>", candidates[primary_tile_idx].origin_filename,dest_tile_width,dest_tile_width);
				fprintf(src_out,"%i: %s\n", primary_tile_idx, candidates[primary_tile_idx].origin_filename);
				fprintf(html_out, "</td>");
				
				continue;
			}
			fprintf(html_out, ">");

			char *url = candidates[primary_tile_idx].origin_filename;
			char url_thumb[1000];
			char url_file[1000];
			memset(url_file, '\0', 1000);
			memset(url_thumb, '\0', 1000);
			/*int is_file_commons = is_file_wikimedia_commons(url);
			if(args->verbose && is_file_commons==1) {
				fprintf(stderr, "url:[%s] ", url);

				get_wikimedia_thumb_url(url, "100", url_thumb, 1000);
				if(args->verbose) fprintf(stderr, " [%s]", url_thumb);
				if(args->verbose) fprintf(stderr, "get file url:");
				get_wikimedia_file_url(url, url_file, 1000);
				if(args->verbose) fprintf(stderr, " [%s]\n", url_file);
				fprintf(html_out, "<a href='%s'>", url_file);
				fprintf(html_out, "<img src='%s' width='%i' height='%i'/>", url_thumb, dest_tile_width, dest_tile_width);
			
				fprintf(html_out, "</a>\n");
				fprintf(src_out,"%s\n", url_file);

			} else */ {
				fprintf(html_out, "<img src='%s' width='%i' height='%i'/>", url, dest_tile_width, dest_tile_width);
				
				fprintf(src_out,"%s\n", url);
				if(x==primary_tile_x_count-1) {
					fprintf(src_out, "\n");
				}
			}
			fprintf(html_out, "</td>");
			
			gdImageSetInterpolationMethod(im, GD_GAUSSIAN );

			int width = gdImageSX(im);
			int height = gdImageSY(im);
			
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
	fprintf(html2_out, "];\n\n const maxCost = Math.max(...candidates.map(item => item.costs));\n"
	"const minCost = Math.min(...candidates.map(item => item.costs));\n"
	"for (let y = 0; y < gridHeight; y++) {\n"
       "for (let x = 0; x < gridWidth; x++) {\n"
            "const cell = document.createElement('div');\n"
			"let idx=x+gridWidth*y;\n"
            "cell.classList.add('cell');\n"
            "cell.style.left = `${x * cellSize}px`;\n"
            "cell.style.top = `${y * cellSize}px`;\n"
			"let p = (candidates[idx].costs-minCost)/(maxCost-minCost);\n"
			"cell.title = `costs:${p}`;\n"
			"p=255-(p*255);\n"
	        "//displayImageInCell(cell, x, y);\n"
			" cell.style.backgroundColor = `rgb(${p} ${p} ${p} / 0.5)`;\n"
            "cell.addEventListener('mouseover', (event) => {});\n"
		    "cell.addEventListener('mouseout', () => {removeHoverClass();});\n"
            "gridContainer.appendChild(cell);\n"
        "}\n"
"}\n\n");
	fprintf(html2_out, "function addHoverClass(x, y) {"
"    for (let dx = x-1; dx <= x+1; dx++) {"
"        for (let dy = y-1; dy <= y+1; dy++) {"
"            const nx = x + dx;"
"            const ny = y + dy;"
"        "
		    "if(dx>=0 && dx < gridWidth && dy >= 0 && dy < gridHeight) {"
"    			let idx = dx + dy * gridWidth;"
""
		    	"const cell = gridContainer.children[idx];"
"	    	    cell.classList.add('hov');"
"			    cell.style.backgroundImage = '';"
"		    }"
 "       }"
"    }"
"}\n\n"
"function removeHoverClass() {\n"
"    for (let i = 0; i < gridContainer.children.length; i++) {\n"
"        gridContainer.children[i].classList.remove('hov');\n"
"    //    gridContainer.children[i].style.backgroundImage = `url(${scaledImage.src})`;\n"
"    }\n"
"}\n\n"
"function displayImageInCell(cell, x, y) {"
"    //const scaledImage = new Image();\n"
"    //scaledImage.src = canvas.toDataURL();\n"
"    //scaledImage.onload = () => {\n"
"    //    cell.style.backgroundImage = `url(${scaledImage.src})`;\n"
"    //    cell.style.backgroundSize = '100%% 100%%';\n"
"    //};\n"
"}</script></body></html>");

	if(args->verbose)
	       fprintf(stderr,"writing file to disk %i %i \n", out_im==NULL, out==NULL);
	if(ft == FT_PNG) {
		gdImagePng(out_im, out);
	} else if( ft == FT_JPEG ) {
		gdImageJpeg(out_im, out, 100);
	}

	gdImageDestroy(out_im);
	m_fclose(html_out);
	m_fclose(html2_out);
	m_fclose(src_out);
	m_fclose(out);
	float costs = total_costs / (total_primary_tile_count*database_image_resolution*database_image_resolution*1.0);

	out = m_fopen(dest_filename, "r+");
	char comment[100];
	memset(comment, 0, 100);	
	if(exclude_count>0)
		snprintf(comment, 100, "mosaik2 (%ix%i-%i from %li -r%i) => %f", 
		primary_tile_x_count, primary_tile_y_count, exclude_count, 
		candidates_count, database_image_resolution, costs);
	else
		snprintf(comment, 100, "mosaik2 (%ix%i from %li -r%i) => %f", 
		primary_tile_x_count, primary_tile_y_count, candidates_count, 
		database_image_resolution, costs);
	off_t out_file_size = get_file_size(dest_filename);
	inject_exif_comment(out, out_file_size, comment, strlen(comment));
	m_fclose(out);

	m_free((void **)&candidates);
	if(args->quiet != 1)
		fprintf(stdout, "total candidate costs: %f,\n per tile:%f\n",
	 	total_costs, (total_costs/(total_primary_tile_count*
		database_image_resolution*database_image_resolution*1.0)));

	fprintf(stderr, "join finish\n");
	return 0;
}

void inject_exif_comment(m2file out, off_t out_file_size, char *comment, size_t comment_len) {
		off_t buflen = out_file_size < BUFSIZ ? out_file_size : BUFSIZ;
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
			fprintf(stderr, "could not read first %li bytes of already written dest-file\n", buflen);
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
