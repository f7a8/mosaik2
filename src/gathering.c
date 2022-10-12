/*mosaik2 - gathering
              _   _                              
             ( )_( )               _             
    __    _ _|  _) |__    __  _ __(_) ___    __  
  / _  \/ _  ) | |  _  \/ __ \  __) |  _  \/ _  \
 ( (_) | (_| | |_| | | |  ___/ |  | | ( ) | (_) |
  \__  |\__ _)\__)_) (_)\____)_)  (_)_) (_)\__  |
 ( )_) |                                  ( )_) |
   \___/                                    \___/ 


 TODO catch kill signal and resume at last result file or repeat all if its done
mona lisa 33 2020 96%
mona lisa 33 2017 95%
*/

#include "libmosaik2.h"
	
	
int mosaik2_gathering(mosaik2_arguments *args) {

	int primary_tile_count = args->num_tiles;
	char *dest_filename = args->dest_image;
	int ratio = args->color_stddev_ratio;
	int unique = args->unique;
	char *mosaik2_db_name = args->mosaik2db;

	mosaik2_database md;
	init_mosaik2_database(&md, mosaik2_db_name);
	read_database_id(&md);
	
	check_thumbs_db(&md);

	check_dest_filename(dest_filename);
	uint8_t thumbs_tile_count=read_thumbs_conf_tilecount(&md) ;
	uint64_t thumbs_count=read_thumbs_db_count(&md);
	
	if(thumbs_tile_count*thumbs_tile_count*(6*256)>UINT32_MAX) {
		//can candidates_costs contain the badest possible costs?
		fprintf(stderr, "thumb tile size too high for internal data structure\n");
		exit(EXIT_FAILURE);
	}

	if(ratio < 0 || ratio > 100) {
		fprintf(stderr, "ratio is only allowed as 0<=ratio<=100\n");
		exit(EXIT_FAILURE);
	}

	if(unique < 0 || unique > 1) {
		fprintf(stderr, "unique is only allowed to be 0 (one image may be used mutiple times) or 1 (one image is just used once)\n");
		exit(EXIT_FAILURE);
	}

	float image_ratio = ratio / 100;
	float stddev_ratio = 1.0 - image_ratio;


	mosaik2_project mp = {
		.ratio = ratio,
		.unique = unique,
		.primary_tile_count = primary_tile_count
	};

	init_mosaik2_project(&mp, md.id, dest_filename);

//	printf("analyze master image\n");

	uint8_t debug = 0;
	uint8_t debug1 = 0;
	const uint8_t html = 0;
	const uint8_t out = 0;
	//const uint8_t duplicates_allowed = 0;

	if(debug) printf("primary_tile_count:%i,thumbs_tile_count:%i\n",primary_tile_count,thumbs_tile_count);
	
	unsigned char* buffer = read_stdin(&mp.file_size);
	fclose(stdin);


  gdImagePtr im;
	int ft = get_file_type_from_buf( buffer, mp.file_size);

  if(ft == FT_JPEG) 
		im = gdImageCreateFromJpegPtrEx(mp.file_size, buffer, 0);
	else if(ft == FT_PNG)
		im = gdImageCreateFromPngPtr(mp.file_size, buffer);
	else {
		free(buffer);
		fprintf(stderr,"image could not be instanciated\n");
		exit(EXIT_FAILURE);
	}
	free(buffer);
	
	
  //       640        = 40                * 16;
  // tile_count on shorter side
	uint32_t tile_count = primary_tile_count * thumbs_tile_count;

	//       6000 
	uint32_t width = gdImageSX(im);
  //       4000
	uint32_t height = gdImageSY(im);

  // 6000  < 640        || 4000   < 640
	if(width < tile_count || height < tile_count ) {
		fprintf(stderr,"image too small\n");
		exit(EXIT_FAILURE);
	} else {
		printf("tile_count:%i (on shorter side)\n", tile_count);
	}
	
	uint32_t short_dim;//, long_dim;
	if(width<height) {
		short_dim = width;
		//long_dim = height;
	} else {
		short_dim = height; 
		//long_dim = width;
	}
 
  //       6                    = ( 4000      - (4000      % 640       ) ) / 640       ;
  //       6                    =   3480 / 640
	uint32_t pixel_per_tile = short_dim / tile_count; //automatically floored
	if(debug)
		fprintf(stdout, "pixel_per_tile=short_dim / tile_count => %i = %i / %i\n", pixel_per_tile, short_dim, tile_count);
  //     36                   = 6              * 6             ;
	double total_pixel_per_tile = pixel_per_tile * pixel_per_tile;
  
  
  //                          96 = 6 * 16
  uint32_t pixel_per_primary_tile = pixel_per_tile * thumbs_tile_count; 
  //                            9216 = 96 * 96
  //double total_pixel_per_primary_tile = pixel_per_primary_tile * pixel_per_primary_tile;
  
  
	uint32_t tile_x_count;
	uint32_t tile_y_count;
	uint32_t primary_tile_x_count;
	uint32_t primary_tile_y_count;
	uint32_t offset_x;
	uint32_t offset_y;
	uint32_t total_tile_count;
	uint32_t total_primary_tile_count;
	uint32_t lx, ly;

	primary_tile_x_count = width / pixel_per_primary_tile;
	primary_tile_y_count = height / pixel_per_primary_tile;
	tile_x_count = primary_tile_x_count * thumbs_tile_count;
	tile_y_count = primary_tile_y_count * thumbs_tile_count;
	offset_x = (width % primary_tile_x_count) / 2;
	offset_y = (height % primary_tile_y_count) / 2;
	
	total_tile_count = tile_x_count * tile_y_count;
	total_primary_tile_count = ( tile_x_count / thumbs_tile_count ) * ( tile_y_count / thumbs_tile_count );

	lx = offset_x + pixel_per_tile * tile_x_count;
	ly = offset_y + pixel_per_tile * tile_y_count;

	if(debug)
			printf("image_dims:%i %i, primary_tile_dims:%i %i(%i), tile_dims:%i %i, l:%i %i, off:%i %i pixel_per:%i %i\n", 
		width, height,
		primary_tile_x_count,primary_tile_y_count,total_primary_tile_count,
		tile_x_count,tile_y_count,
		lx,ly,
		offset_x,offset_y,
		pixel_per_primary_tile,pixel_per_tile);

	if(unique == 1 && total_primary_tile_count > thumbs_count) {
		fprintf(stderr, "there are too few candidates (%lu) for unique than needed (%i)\n", thumbs_count, total_primary_tile_count);
		exit(EXIT_FAILURE);
	}


	/*
		The data structure of the candidates list.
		For each tile are stored as many candidates as there are primarys tiles. With the largest possible number of multiple selected tiles, a later reduction procedure can lead to only once (best) used tiles, without there being too few alternatives.
	*/

	int max_candidates_len = unique == 1 ? total_primary_tile_count : 1;
	int total_candidates_count = total_primary_tile_count * max_candidates_len;

	if(debug)
		fprintf(stdout, "max_candidates_len:%i, total_candidates_count:%i\n", max_candidates_len, total_candidates_count);

	unsigned long      *candidates_index = malloc(total_candidates_count * sizeof(unsigned long));
	if( candidates_index == NULL ) { fprintf(stderr, "could not allocate memory for candidates_index\n"); exit(EXIT_FAILURE); }
	unsigned long long *candidates_costs = malloc(total_candidates_count * sizeof(unsigned long long));
	if( candidates_costs == NULL ) { fprintf(stderr, "could not allocate memory for candidates_costs\n"); exit(EXIT_FAILURE); }
	unsigned char      *candidates_off_x = malloc(total_candidates_count * sizeof(unsigned char));
	if( candidates_off_x == NULL ) { fprintf(stderr, "could not allocate memory for candidates_off_x\n"); exit(EXIT_FAILURE); }
	unsigned char      *candidates_off_y = malloc(total_candidates_count * sizeof(unsigned char));
	if( candidates_off_y == NULL ) { fprintf(stderr, "could not allocate memory for candidates_off_y\n"); exit(EXIT_FAILURE); }
	unsigned int       *candidates_len  = malloc(total_primary_tile_count * sizeof(unsigned int));
	if( candidates_len == NULL ) { fprintf(stderr, "could not allocate memory for candidates_len\n"); exit(EXIT_FAILURE); }
	unsigned int       *candidates_elect = malloc(total_primary_tile_count * sizeof(unsigned int));
	if( candidates_elect == NULL ) { fprintf(stderr, "could not allocate memory for candidates_elect\n"); exit(EXIT_FAILURE); }
	unsigned int       *candidates_ins  = malloc(total_primary_tile_count * sizeof(unsigned int));
	if( candidates_ins == NULL ) { fprintf(stderr, "could not allocate memory for candidates_ins\n"); exit(EXIT_FAILURE); }

	//intialize with -1
	for(uint32_t i=0;i<total_candidates_count;i++) {
		candidates_index[i] = 9u;
		candidates_costs[i] = ULLONG_MAX;
		candidates_off_x[i] = 0u;
		candidates_off_y[i] = 0u;
	}

/*	for(uint32_t i=0;i<total_primary_tile_count;i++) {
		candidates_len[i] = 0;
		candidates_ins[i] = 0;
	}*/
	memset( candidates_len, 0, total_primary_tile_count * sizeof(unsigned int));
	memset( candidates_elect, 0, total_primary_tile_count * sizeof(unsigned int));
	memset( candidates_ins, 0, total_primary_tile_count * sizeof(unsigned int));

	//memset( candidates_len, 0, total_primary_tile_count); // all candidates lengths have a zero length by default
	//memset( candidates_ins, 8u, total_primary_tile_count);
	//candidates_ins[0]=0;
	//fprintf(stdout,"\n***%u%\n", candidates_ins[0]);


	if(debug) {
		fprintf(stdout, "\ncandidates_ins:" );
		for( int i=0;i<total_primary_tile_count;i++) {
			fprintf(stdout, "%u,", candidates_ins[i]);
		}
		fprintf(stdout, "\n" );
	}
	if(out) printf("%04X %04X %02X %02X", width, height, tile_x_count, tile_y_count);
    
	double *colors_red   = malloc(total_tile_count * sizeof(double));
	if( colors_red == NULL ) { fprintf(stderr, "could not allocate memory for colors_red\n"); exit(EXIT_FAILURE); }
	double *colors_green = malloc(total_tile_count * sizeof(double));
	if( colors_green == NULL ) { fprintf(stderr, "could not allocate memory for colors_green\n"); exit(EXIT_FAILURE); }
	double *colors_blue  = malloc(total_tile_count * sizeof(double));
	if( colors_blue == NULL ) { fprintf(stderr, "could not allocate memory for colors_blue\n"); exit(EXIT_FAILURE); }
	long double *colors_abw_red  = malloc(total_tile_count * sizeof(long double));
	if( colors_abw_red == NULL ) { fprintf(stderr, "could not allocate memory for colors_abw_red\n"); exit(EXIT_FAILURE); }
	long double *colors_abw_green= malloc(total_tile_count * sizeof(long double));
	if( colors_abw_green == NULL ) { fprintf(stderr, "could not allocate memory for colors_abw_green\n"); exit(EXIT_FAILURE); }
	long double *colors_abw_blue = malloc(total_tile_count * sizeof(long double));
	if( colors_abw_blue == NULL ) { fprintf(stderr, "could not allocate memory for colors_abw_blue\n"); exit(EXIT_FAILURE); }

	int *colors_red_int = malloc(total_tile_count * sizeof(int));
	if( colors_red_int == NULL ) { fprintf(stderr, "could not allocate memory for colors_red_int\n"); exit(EXIT_FAILURE); }
	int *colors_green_int = malloc(total_tile_count * sizeof(int));
	if( colors_green_int == NULL ) { fprintf(stderr, "could not allocate memory for colors_green_int\n"); exit(EXIT_FAILURE); }
	int *colors_blue_int = malloc(total_tile_count * sizeof(int));
	if( colors_blue_int == NULL ) { fprintf(stderr, "could not allocate memory for colors_blue_int\n"); exit(EXIT_FAILURE); }
	int *stddev_red_int = malloc(total_tile_count * sizeof(int));
	if( stddev_red_int == NULL ) { fprintf(stderr, "could not allocate memory for stddev_red_int\n"); exit(EXIT_FAILURE); }
	int *stddev_green_int = malloc(total_tile_count * sizeof(int));
	if( stddev_green_int == NULL ) { fprintf(stderr, "could not allocate memory for stddev_green_int\n"); exit(EXIT_FAILURE); }
	int *stddev_blue_int = malloc(total_tile_count * sizeof(int));
	if( stddev_blue_int == NULL ) { fprintf(stderr, "could not allocate memory for stddev_blue_int\n"); exit(EXIT_FAILURE); }

	for(int i=0;i<total_tile_count;i++) {
		colors_red[i]=0.0;
		colors_green[i]=0.0;
		colors_blue[i]=0.0;
		colors_abw_red[i]=0.0;
		colors_abw_green[i]=0.0;
		colors_abw_blue[i]=0.0;
	}

	for(int j=0,j1=offset_y;j1<ly;j++,j1++){    
		//printf("%i\n",i1); 
		for(int i=0,i1=offset_x;i1<lx;i++,i1++){
			//i runs from 0 to length of cropped area
			//j also
			//i1 und j1 are corrected by the offset    

			int tile_x = i/pixel_per_tile;
			int tile_y = j/pixel_per_tile;
			int tile_idx = tile_y * tile_x_count + tile_x;
//			fprintf(stderr, "%i:%i off:%i %i lx:%i ly:%i i:%i j:%i i1:%i j1:%i\n", width, height, offset_x, offset_y, lx, ly, i,j,i1,j1);
			int color =  gdImageTrueColorPixel(im,i1,j1);
			//TODO one function 
			int red0 =   gdTrueColorGetRed(color);
			int green0 = gdTrueColorGetGreen(color);
			int blue0 =  gdTrueColorGetBlue(color);

			//int red0 = color >> 16 & 0xff;;
			//int green0 = color >> 8 & 0xff;;
			//int blue0 = color & 0xff;

			if(debug1)printf("%i,%i,[%i %i],x:%i,y:%i,idx:%i,tile_count:%i,%i,px_tile:%i,lxy:%i,%i  r:%i,g:%i,b:%i    r:%f,g:%f,b:%f\n",i,j,i1,j1,tile_x,tile_y,tile_idx,tile_x_count,tile_y_count,pixel_per_tile,lx,ly,red0,green0,blue0, red0/total_pixel_per_tile, green0/total_pixel_per_tile, blue0/total_pixel_per_tile); 
			//add only fractions to prevent overflow in very large pixel_per_tile settings
			colors_red[tile_idx] +=   red0   / (total_pixel_per_tile);
//		if(debug && tile_idx==0) printf("avg_green:%f ", colors_green[tile_idx]);
			colors_green[tile_idx] += green0 / (total_pixel_per_tile);
			colors_blue[tile_idx] +=  blue0  / (total_pixel_per_tile);
		}
	}
    





	//double total_tile_count_d = total_tile_count;
	for(int j=0,j1=offset_y;j1<ly;j++,j1++){
		for(int i=0,i1=offset_x;i1<lx;i++,i1++){
		//if(debug) printf("avg_blue0:%f i:%i i1:%i lx:%i\n", colors_blue[0],i ,i1, lx);        

			int tile_x = i/pixel_per_tile;
			int tile_y = j/pixel_per_tile;
			int tile_idx = tile_y * tile_x_count + tile_x;

			if(debug1) printf( "i:%i j:%i, tile_x:%i tile_y:%i tile_idx:%i c:%f %f %f\n", i,j,tile_x, tile_y, tile_idx, colors_red[tile_idx], colors_green[tile_idx],colors_blue[tile_idx]);
				int color =  gdImageTrueColorPixel(im,i1,j1);

				int red0 =   gdTrueColorGetRed(color);
				int green0 = gdTrueColorGetGreen(color);
				int blue0 =  gdTrueColorGetBlue(color);
			//					 int red0 = color >> 16 & 0xff;;
			//					int green0 = color >> 8 & 0xff;;
			//						int blue0 = color & 0xff;

				double red2 = colors_red[tile_idx] - red0;
				double green2 = colors_green[tile_idx] - green0;
				double blue2 = colors_blue[tile_idx] - blue0;

				colors_abw_red[tile_idx] += red2 * red2;
				colors_abw_green[tile_idx] += green2 * green2;
      	colors_abw_blue[tile_idx] += blue2 * blue2;
			}
		}

		if(debug)printf("dim:%i %i,off:%i %i,l:%i %i,primary_tile_cout:%i %i,tile_count:%i %i,pixel_per_tile:%i %f\n",width,height,offset_x,offset_y,lx,ly,primary_tile_x_count,primary_tile_y_count,tile_x_count,tile_y_count,pixel_per_tile,total_pixel_per_tile);
		if(html)printf("<html><head><style>table{ width:851px;height:566px; border-collapse: collapse;}td{padding:0;height:0.2em;width:0.2em;}</style></head><body><table>");



		if(out) {

			for(int y=0;y<tile_y_count;y++) {
				if(html)printf("<tr>");

				for(int x=0;x<tile_x_count;x++) {	
					int i= y*tile_x_count+x;
					if(debug1) printf("avg_blue0:%f ", colors_blue[i]);
					int red =   (int)round(colors_red[i]);
					int green = (int)round(colors_green[i]);
					int blue =  (int)round(colors_blue[i]);

					if(debug1) printf("avg_red:%i %f", red,colors_red[i]);
					if(debug1) printf("avg_green:%i %f", green,colors_green[i]);
					if(debug1) printf("avg_blue:%i %f", blue,colors_blue[i]);

		  		double abw_red = sqrt(colors_abw_red[i]/ ( total_pixel_per_tile - 1.0 ));
		  		double abw_green = sqrt(colors_abw_green[i]/ (total_pixel_per_tile - 1.0));
		  		double abw_blue = sqrt(colors_abw_blue[i]/ ( total_pixel_per_tile - 1.0));

					// to better utilize the byte space
					abw_red = abw_red * 2;
					abw_green = abw_green * 2;
					abw_blue*= abw_blue * 2;
			
					// in most cases this limit is not necessary..
					if(abw_red>255) abw_red=255;
					if(abw_green>255) abw_green=255;
					if(abw_blue>255) abw_blue=255;

					//colors_red[i] = red;
					//colors_green[i] = green;
					//colors_blue[i] = blue;

					colors_abw_red[i] = abw_red;
					colors_abw_green[i] = abw_green;
					colors_abw_blue[i] = abw_blue;


					if(html)printf("<td style='background-color:#%02x%02x%02x'>",red,green,blue);
					//if(html)printf("<td style='background-color:#%02x%02x%02x'>",(int)round(abw_red/2.0),(int)round(abw_green/2.0),(int)round(abw_blue/2.0));
					if(debug)printf( "%i:%i %i %i  %i %i %i\n",i,red,green,blue,(int)round(abw_red),(int)round(abw_green),(int)round(abw_blue));
					if(out) printf(" %02X%02X%02X %02X%02X%02X",red,green,blue, (int)round(abw_red),(int)round(abw_green),(int)round(abw_blue));
					if(html)printf("</td>");
				}
				if(html)printf("</tr>");
			}
			if(html)printf("</table></body></html>");
		}
	
	gdImageDestroy(im);

	for(uint32_t i=0;i<total_tile_count;i++) {
		colors_red_int  [i] = (int) round(colors_red  [i]);
		colors_green_int[i] = (int) round(colors_green[i]);
		colors_blue_int [i] = (int) round(colors_blue [i]);

		double sr = sqrt(colors_abw_red  [i] / ( total_pixel_per_tile - 1.0));
		double sg = sqrt(colors_abw_green[i] / ( total_pixel_per_tile - 1.0));
		double sb = sqrt(colors_abw_blue [i] / ( total_pixel_per_tile - 1.0));

		sr = sr * 2.0;
		sg = sg * 2.0;
		sb = sb * 2.0;

		if(sr > 255) sr = 255;
		if(sg > 255) sg = 255;
		if(sb > 255) sb = 255;

		stddev_red_int  [i] = (int) round(sr);
		stddev_green_int[i] = (int) round(sg);
		stddev_blue_int [i] = (int) round(sb);
		//stddev_red_int  [i] = (int) round(sqrt(colors_abw_red  [i] / ( total_pixel_per_tile - 1.0 )) * 2);
		//stddev_green_int[i] = (int) round(sqrt(colors_abw_green[i] / ( total_pixel_per_tile - 1.0 )) * 2);
		//stddev_blue_int [i] = (int) round(sqrt(colors_abw_blue [i] / ( total_pixel_per_tile - 1.0 )) * 2);

		//if(stddev_red_int  [i]>255) stddev_red_int  [i]=255;
		//if(stddev_green_int[i]>255) stddev_green_int[i]=255;
		//if(stddev_blue_int [i]>255) stddev_blue_int [i]=255;

		//if(stddev_red_int[i]<0) {printf("red under 0 ");exit(1); }
		//if(stddev_green_int[i]<0) {printf("green under 0 ");exit(1); }
		//if(stddev_blue_int[i]<0) {printf("blue under 0 ");exit(1); }

		if(debug1) {
			printf("%i:%f %f %f  %Lf %Lf %Lf\n",
				i,
				colors_red[i],     colors_green[i],     colors_blue[i],
				colors_abw_red[i], colors_abw_green[i], colors_abw_blue[i]
			);
			printf("%i:%i %i %i  %i %i %i\n",
				i,
				colors_red_int[i], colors_green_int[i], colors_blue_int[i],
				stddev_red_int[i], stddev_green_int[i], stddev_blue_int[i]
			);
		}
	}

	free( colors_red );
	free( colors_green );
	free( colors_blue );
	free( colors_abw_red );
	free( colors_abw_green );
	free( colors_abw_blue );

	printf("searching for candidates\n");

	//uint32_t thumbs_db_name_len = strlen(thumbs_db_name);
	//char filename[250];
	//memset(filename,0,250);
	
	//int candidate_id = 0;
	FILE *thumbs_db_tile_dims = fopen(md.tiledims_filename, "rb");
	if( thumbs_db_tile_dims == NULL) {
		fprintf(stderr, "thumbs db file with tile dimensions (%s) could not be opened\n", md.tiledims_filename);
		exit(EXIT_FAILURE);
	}

//	uint64_t thumbs_db_element_count = st.st_size/2;


	FILE *thumbs_db_image_colors = fopen(md.imagecolors_filename, "rb");
	if( thumbs_db_image_colors == NULL ) {
		fprintf(stderr, "thumbs db file with image colors (%s) could not be opened\n", md.imagecolors_filename);
		exit(EXIT_FAILURE);
	}

	FILE *thumbs_db_image_stddev = fopen(md.imagestddev_filename, "rb");
	if( thumbs_db_image_stddev == NULL ) {
		fprintf(stderr, "thumbs db file with image stddev colors (%s) could not be opened\n", md.imagestddev_filename);
		exit(EXIT_FAILURE);
	}

	FILE *thumbs_db_invalid = fopen(md.invalid_filename, "rb");
	if( thumbs_db_invalid == NULL ) {
		fprintf(stderr, "thumbs db file that marks the invalid images (%s) could not be opened\n", md.invalid_filename);
		exit(EXIT_FAILURE);
	}

	FILE *duplicates_file = fopen(md.duplicates_filename, "rb");
	if( duplicates_file == NULL ) {
		fprintf(stderr, "mosaik2 database file for duplicate images (%s) could not be opened\n", md.duplicates_filename);
		exit(EXIT_FAILURE);
	}

	

	const uint32_t SIZE_64 = 65536;//64KB
	const uint32_t SIZE_PRIMARY = total_primary_tile_count;

	// buffers for db file reading
	uint8_t tile_dims_buf[SIZE_64];
	unsigned char colors_buf[SIZE_64];
	unsigned char stddev_buf[SIZE_64];
	unsigned char invalid_buf[SIZE_64];
	unsigned char duplicates_buf[SIZE_64];





	//SAVING PRIMARY TILE DIMENSIONS
	FILE *primarytiledims_file = fopen(mp.dest_primarytiledims_filename, "w");
	if(primarytiledims_file == NULL ) {
		fprintf(stderr, "could not open file for save primary tile lengths (%s)\n", mp.dest_primarytiledims_filename);
		exit(EXIT_FAILURE);
	}
	fprintf(primarytiledims_file, "%i	%i", primary_tile_x_count, primary_tile_y_count);
	fclose(primarytiledims_file);

	
	
	uint64_t idx = 0;

	printf("compare candidates\n");


	int32_t percent = -1;
	while(1) {
		uint32_t len = fread(invalid_buf,1,SIZE_64/2,thumbs_db_invalid);
		if(len==0) {
			fprintf(stderr,"there is no data to read from invalid file\n");
			exit(EXIT_FAILURE);
		}
		len = fread(duplicates_buf,1,SIZE_64/2, duplicates_file);
		if(len==0) {
			fprintf(stderr, "could not read from duplicates file\n");
			exit(EXIT_FAILURE);
		}
		len = fread(tile_dims_buf,1,SIZE_64,thumbs_db_tile_dims);
		if(len==0) {
			fprintf(stderr,"there is no data to read int tile_dims file\n");
			exit(EXIT_FAILURE);
		}

		for(uint32_t i=0,len1=len-1;i<len1;i+=2,idx++) {

			// printf("%02X %02X tile dim\n", tile_dims_buf[i], tile_dims_buf[i+1]);
			uint8_t thumbs_db_tile_x_count = tile_dims_buf[i];
			uint8_t thumbs_db_tile_y_count = tile_dims_buf[i+1];
			uint32_t thumbs_db_total_tile_count = thumbs_db_tile_x_count * thumbs_db_tile_y_count;

			float new_percent_f = idx / (thumbs_count * 1.0); 
			uint8_t new_percent = round( new_percent_f * 100);				
			
			if( new_percent != percent) {

				time_t now;
				time(&now);
				percent = new_percent;
				printf("%i%%, %lu/%lu %s", percent, idx, thumbs_count, ctime(&now));
			} else {
			}

			if(invalid_buf[i/2]==1) {
				if(debug)fprintf(stderr,"thumb (%lu) is marked as invalid, will be skipped\n", idx);

				// in case of invalid entries move the file pointers without reading the colors data
				fseeko(thumbs_db_image_colors,thumbs_db_total_tile_count*3,SEEK_CUR);
				fseeko(thumbs_db_image_stddev,thumbs_db_total_tile_count*3,SEEK_CUR);

				continue;
			} 
			
			// this could be slow, but works definitly
			// if an thumb is valid read its image data
			uint32_t colors_len = fread(colors_buf,1,thumbs_db_total_tile_count*3,thumbs_db_image_colors);
			uint32_t stddev_len = fread(stddev_buf,1,thumbs_db_total_tile_count*3,thumbs_db_image_stddev);
			
			if(debug1)printf("colors_len:%i (%i %i)\n",colors_len, tile_dims_buf[i], tile_dims_buf[i+1]);
			if(debug1)printf("stddev_len:%i\n",stddev_len);
			if(debug1)printf("colors_buf1:%02x %02x %02x %02x\n", colors_buf[0], colors_buf[1], colors_buf[2], colors_buf[3]);
			
			/*printf("%i %i %i ",i,thumbs_db_tile_x_count,thumbs_db_tile_y_count);
			for(uint32_t x=0;x<thumbs_db_total_tile_count;x++) {
				printf("%i:%02X%02X%02X ",x,colors_buf[x*3],colors_buf[x*3+1],colors_buf[x*3+2]);
			}
			printf("\n");
			for(uint32_t x=0;x<thumbs_db_total_tile_count;x++) {
				printf("%i;%02x%02x%02x ",x,stddev_buf[x*3],stddev_buf[x*3+1],stddev_buf[x*3+2]);
			}
			printf("\n");*/

			uint8_t shift_x_len = thumbs_db_tile_x_count - thumbs_tile_count;
			uint8_t shift_y_len = thumbs_db_tile_y_count - thumbs_tile_count;

			if(shift_x_len != 0 && shift_y_len != 0) {
				fprintf(stderr,"one shift axis should be zero but: %i %i at thumb_idx: %lu\n", shift_x_len, shift_y_len, idx
				//, thumbs_db_tile_x_count, thumbs_db_tile_y_count, thumbs_tile_count);
				);
				exit(EXIT_FAILURE);
			}

			if(debug1) {fprintf(stderr,"%lu shift_len:%i %i\n", idx, shift_x_len,shift_y_len);  }
			
			//uint32_t candidates_costs_f = UINT32_MAX;
			//uint32_t candidates_index_f = 0;
			//uint32_t candidates_

			for(uint8_t shift_y=0;shift_y<=shift_y_len;shift_y++) {
				for(uint8_t shift_x=0;shift_x<=shift_x_len;shift_x++) {

					if(debug)fprintf(stderr,"1 shift:%i %i\n",shift_x,shift_y);

					for(uint32_t primary_y=0;primary_y<primary_tile_y_count;primary_y++) {
						for(uint32_t primary_x=0;primary_x<primary_tile_x_count;primary_x++) {

							uint32_t primary_tile_idx = primary_tile_x_count * primary_y + primary_x;
							if(debug1&&primary_x==0&&primary_y==0) printf("primary_tile_idx:%i\nk:",primary_tile_idx);
							
					if(debug1)fprintf(stderr,"2 shift:%i %i\n",shift_x,shift_y);
							
							unsigned long long diff_color0 = 0;
							
							for(uint32_t k=0;k<thumbs_tile_count*thumbs_tile_count;k++) {
								
								
						 // how to find 64 (8x8 for example) tiles of the primary tile?
             //--------PRIMARY TILE 0---------------------------PRIMARY TILE 1-----------|
             // 0     1   2   3   4   5   6   7|   8    9   10   11   12   13   14   15|
             // 400 401 402 403 404 405 406 407| 408  409  410  411  412  413  414  415|  ...
             // 800          ...               |                                       |
             //2800 2801                   2807|2808 2809                          2815|
             //--------PRIMARY TILE 50----------|---------------PRIMARY TILE 51--------  |
             //3200                        3207|3208                               3215|
             //3600                        3607|3608                               3615|  ...
             //                ...             |                   ...                 |
             //6000                        6007|6008                               6015|
             //------------------------------------------------------------------------
             //                               ...
       			// now getting difficult
						// trying compare 64 tiles of the thumb with corresponding 64 tiles of the primary image
						// but they are spread 
			 
			 			// 0..8 => k%8    0,...,7    
			 			//       PLUS
			 			// 0..400 => (k/8)*50*8 == 0,400,800
			 			//       PLUS
			 			// 0,8,16,...400  => PRIMARY_TILE_IDX*8+(PRIMARY_TILE_IDX/50)*50*8*8
			 



								uint32_t colors_idx = 
									k%thumbs_tile_count 
									+ (k/thumbs_tile_count)*primary_tile_x_count*thumbs_tile_count
									+ (primary_tile_idx%primary_tile_x_count)*thumbs_tile_count 
									+ (primary_tile_idx/primary_tile_x_count)*primary_tile_x_count*thumbs_tile_count*thumbs_tile_count;
								
								uint32_t j = 
									k%thumbs_tile_count 
									+ (k/thumbs_tile_count)*thumbs_db_tile_x_count
									+ shift_x 
									+ shift_y * thumbs_db_tile_x_count;

			//					if(j>=thumbs_db_total_tile_count) {
			//						fprintf(stderr,"Whoops, this should not have happend! Something did go wrong with tile calculations.. Sorry.");
			//						exit(EXIT_FAILURE);
			//					}

								if(debug1&&primary_x==0&&primary_y==0) 
										printf("%i:%i:%i ",k,colors_idx,j);
					
								int  diff_c_r = abs(colors_red_int  [colors_idx] - (int) colors_buf[j*3  ]);
								int  diff_c_g = abs(colors_green_int[colors_idx] - (int) colors_buf[j*3+1]);
								int  diff_c_b = abs(colors_blue_int [colors_idx] - (int) colors_buf[j*3+2]);

								int  diff_s_r = abs( ((int) stddev_red_int  [colors_idx]) - (int) stddev_buf[j*3  ]);
								int  diff_s_g = abs( ((int) stddev_green_int[colors_idx]) - (int) stddev_buf[j*3+1]);
								int  diff_s_b = abs( ((int) stddev_blue_int [colors_idx]) - (int) stddev_buf[j*3+2]);


								uint32_t  diff0 = image_ratio*(diff_c_r+diff_c_g+diff_c_b) + stddev_ratio * ((diff_s_r+diff_s_g+diff_s_b)/2.0);
								diff_color0 += diff0;
								
								if(debug1) {
									printf("c_r:%i %i - %i => %i\n",colors_idx, colors_red_int  [colors_idx],   colors_buf[j*3  ], diff_c_r);
									printf("c_g:%i %i - %i => %i\n",colors_idx, colors_green_int[colors_idx],   colors_buf[j*3+1], diff_c_g);
									printf("c_b:%i %i - %i => %i\n",colors_idx, colors_blue_int [colors_idx],   colors_buf[j*3+2], diff_c_b);

									printf("s_r:%i %i - %i => %i\n",colors_idx, stddev_red_int  [colors_idx],   stddev_buf[j*3  ], diff_s_r);
									printf("s_g:%i %i - %i => %i\n",colors_idx, stddev_green_int[colors_idx],   stddev_buf[j*3+1], diff_s_g);
									printf("s_b:%i %i - %i => %i\n",colors_idx, stddev_blue_int [colors_idx],   stddev_buf[j*3+2], diff_s_b);
								
									printf("diff-> %i %llu\n", diff0, diff_color0);
								}

								//printf("idxs: k:%i colors_idx:%i j:%i  %i %i %i\n", k, colors_idx, j, j*3, j*3+1, j*3+2);
								//printf("color-> %i %i %i  %i %i %i  %i %i %i  %02x %02x %02x\n",
								//	colors_red[colors_idx], colors_green[colors_idx], colors_blue[colors_idx],
								//	colors_buf[j*3], colors_buf[j*3+1], colors_buf[j*3+2],
								//	colors_buf[i*3], colors_buf[j*3+1], colors_buf[j*3+2],
								//	colors_buf[i*3], colors_buf[j*3+1], colors_buf[j*3+2]
								//);

								/*printf("color1: %i %i %i  %02x %02x %02x\n",
								  colors_red[colors_idx], colors_green[colors_idx], colors_blue[colors_idx],
									colors_buf[0], colors_buf[1], colors_buf[2]);
									if(debug)printf("colors_buf2:%02x %02x %02x %02x\n", colors_buf[0], colors_buf[1], colors_buf[2], colors_buf[3]);
									if(debug)printf("colors_buf2:%02x %02x %02x %02x\n", colors_buf[0], colors_buf[1], colors_buf[2], colors_buf[3]);
									printf("color1: %02x %02x %02x  %02x %02x %02x\n",
								  colors_red[colors_idx], colors_green[colors_idx], colors_blue[colors_idx],
									colors_buf[0], colors_buf[1], colors_buf[2]);


									printf("stddev: %02x %02x %02x  %02x %02x %02x\n", colors_abw_red[colors_idx], colors_abw_green[colors_idx], colors_abw_blue[colors_idx],
									stddev_buf[j*3], stddev_buf[j*3+1], stddev_buf[j*3+2]);
									printf("diffs: %i %i %i  %i %i %i => %i => %lli\n",  diff_c_r, diff_c_g, diff_c_b,  diff_s_r, diff_s_g, diff_s_b, diff0); */
									//printf("%ju => %i %i %" PRIu64 " %" PRIu32 " %" PRIu32 " %" PRIu32 " %" PRIu32 " %" PRIu32 " %" PRIu32 " -- %" PRIu8 " %" PRIu8 " %" PRIu8 "\n",diff_color0,colors_idx,j,diff0,diff_c_r,diff_c_g,diff_c_b,diff_s_r,diff_s_g,diff_s_b, colors_red[colors_idx], colors_green[colors_idx], colors_blue[colors_idx], colors_buf[j*3], colors_buf[j*3+1], colors_buf[j*3+2]);
								}

								if(debug1)printf(":::%lu  %llu\n\n",idx, diff_color0);
/*								if(diff_color0>=176093773879||diff_color0<0) {
									printf("%i \n",idx);
									exit(0);
								}*/
								if(debug1&&primary_x==0&&primary_y==0)
										printf("\n");
								/*candidates_index[iiiii] = 0;
								candidates_costs[iiiii] = 0.0;
								candidates_off_x[iiiii] = 0;
								candidates_off_y[iiiii] = 0;*/


								// THIS is the point where the calculated color difference
								// is compared to the already stored ones.
								// If its lower, it is more equal and it should be the new candidate
								//if(diff_color0 < candidates_costs[primary_tile_idx] ) {
								                 // badest costs is saved in the last position, and this position 
																 // will expand from 0 to total_primary_tile_count
								uint32_t offset = primary_tile_idx * max_candidates_len;
								unsigned long long old_costs=ULLONG_MAX;

								

								if(candidates_len[primary_tile_idx]>0 && candidates_len[primary_tile_idx] >= max_candidates_len)
									old_costs = candidates_costs[offset + candidates_len[primary_tile_idx] -1];

								if(diff_color0 < old_costs) {

									
									
									if(debug)
										printf("idx:%lu primary_tile_idx:%05i diff_color0:%05llu,old_costs:%05llu,len:%i %i\n",idx,primary_tile_idx,diff_color0,old_costs,candidates_len[primary_tile_idx], max_candidates_len);
									if(unique==1) {
										int continue_=0;
										//if there is the same candidate already with a lower costs in it
										// it is removed by shifting all worse one the left
										// so if the already one is better (lower) => the new one wont be inserted
										for(uint32_t i=0;i<candidates_len[primary_tile_idx];i++) {
											if(candidates_index[offset + i] == idx) {
												if(debug)fprintf(stdout, "same candidate already selected\n");
												if( candidates_costs[offset + i] <= diff_color0) {
													if(debug)fprintf(stdout, "ignore new candidate new_costs:%llu, old_costs:%llu\n",diff_color0,candidates_costs[offset+i]);
													continue_=1;
													break;
												} else {
													if(debug)
														fprintf(stdout, "removing old candidate, shift from %i to %i the right\n", i, candidates_len[primary_tile_idx]);
													for(uint32_t j=i;j<candidates_len[primary_tile_idx]-1;j++) {
														candidates_index[offset+j] = candidates_index[offset+j+1];
														candidates_costs[offset+j] = candidates_costs[offset+j+1];
														candidates_off_x[offset+j] = candidates_off_x[offset+j+1];
														candidates_off_y[offset+j] = candidates_off_y[offset+j+1];
														if(debug)fprintf(stdout, "remove old candidate %i\n", j);
													}
													candidates_len[primary_tile_idx]--;

												}
											}
										}

										if(continue_==1) {
											if(debug)
												fprintf(stdout, "continue\n");
											continue;
										}
									}
									candidates_ins[primary_tile_idx]++;
									// the better costs are the lower ones and they are stored at the beginning

									int insert_pos = 0;
									for(; insert_pos < candidates_len[primary_tile_idx]; insert_pos++) {
										if( diff_color0 < candidates_costs[offset + insert_pos] )
											// the right insert position is found
											break;
									}
				
									if(debug)
										fprintf(stdout, "thumbs_db_idx:%lu primary_tile_idx:%i insert pos found:%i\n", idx, primary_tile_idx, insert_pos);

									if(candidates_len[primary_tile_idx] <total_primary_tile_count 
										&& candidates_len[primary_tile_idx] < max_candidates_len) {
										// if there is any space left in the candidates list
										// it increases its usage space until the max len (total_primary_tile_count)
										// is reached.
										candidates_len[primary_tile_idx]++;
									}
									if(debug)
										fprintf(stdout,"primary_tile_idx:%i inspos:%i, diff_color0:%llu, idx:%lu, off:%i %i, len:%i\n", primary_tile_idx, insert_pos, diff_color0, idx, candidates_off_x[offset + insert_pos], candidates_off_y[offset + insert_pos], candidates_len[primary_tile_idx]);

									//position to insert new costs found
									//shift worse costs to the right
										
									for(int i=candidates_len[primary_tile_idx] - 1;i>insert_pos;i--) {
												
										candidates_index[offset + i] = candidates_index[offset + i - 1];
										candidates_costs[offset + i] = candidates_costs[offset + i - 1];
										candidates_off_x[offset + i] = candidates_off_x[offset + i - 1];
										candidates_off_y[offset + i] = candidates_off_y[offset + i - 1];

									/*if(candidates_off_x[offset + i]  != 0 && candidates_off_y[offset + i] != 0) {
										fprintf(stderr,"trying 0 to insert to offsets that are unequal 0, this should not be possible\n");
										exit(100);
										}*/
									}


									//set the new candidate
									candidates_index[offset + insert_pos] = idx;
									candidates_costs[offset + insert_pos] = diff_color0;
									candidates_off_x[offset + insert_pos] = shift_x;
									candidates_off_y[offset + insert_pos] = shift_y;

									/*if(shift_x != 0 && shift_y != 0) {
										fprintf(stderr,"trying to insert to offsets that are unequal 0, this should not be possible\n");
										exit(100);
										}*/

									if(debug) {
									fprintf(stdout, "candidates_index[%i:%i-%i]:{", primary_tile_idx, 0, max_candidates_len);
									for( int i=0;i<max_candidates_len;i++) {
										if(candidates_len[primary_tile_idx]==i)
											fprintf(stdout, "}");
										fprintf(stdout, "%lu,", candidates_index[offset + i]);
									}
									fprintf(stdout, "\ncandidates_costs[%i:%i-%i]:{", primary_tile_idx, 0, max_candidates_len);
//									fprintf(stdout, "%05lli ", candidates_costs[primary_tile_idx + insert_pos ], diff_color0);
									for( int i=0;i<max_candidates_len;i++) {
										if(candidates_len[primary_tile_idx]==i)
											fprintf(stdout, "}");
										fprintf(stdout, "%llu,", candidates_costs[offset + i]);
									}
									fprintf(stdout, "\ncandidates_off[%i:%i-%i]:{", primary_tile_idx, 0, max_candidates_len);
									for( int i=0;i<max_candidates_len;i++) {
										if(candidates_len[primary_tile_idx]==i)
											fprintf(stdout, "}");
										fprintf(stdout, "%i %i,", candidates_off_x[offset + i], candidates_off_y[offset + i]);
									}
									fprintf(stdout, "\ncandidates_ins:" );
									for( int i=0;i<total_primary_tile_count;i++) {
										fprintf(stdout, "%d,", candidates_ins[i]);
									}
									fprintf(stdout,"\n");
									}


								
									// this is for the impatient, an intermediate result will be save every 1000th candidate image.
									if(idx %1000 == 0 || debug ) {
										FILE *mosaik2_result = fopen(mp.dest_result_filename, "wb");
										if( mosaik2_result == NULL ) {
											fprintf(stderr, "result file (%s) could not be opened\n", mp.dest_result_filename);
											exit(EXIT_FAILURE);
										}
										
										for(uint32_t i=0;i<SIZE_PRIMARY;i++) {
										//TODO dont know why, somewhere in this program a routine seems to change the offsets. Can anybody identify this error source?
											if(candidates_off_x[i*max_candidates_len] != 0 && candidates_off_y[i*max_candidates_len] != 0 ) {
												fprintf(stderr,"*** #%lu one offset should be zero cid: %lu: off:%i %i\n", 
													idx, 
													candidates_index[i*max_candidates_len],
													candidates_off_x[i*max_candidates_len],
													candidates_off_y[i*max_candidates_len] );
												if(candidates_off_x[25]==8 && candidates_off_y[25]==8) {
													fprintf(stderr, "*******************************i:%i\n", i);
												}

												fprintf(stderr, "%i	%li	%llu	%i	%i\n",
													i,
													candidates_index[i*max_candidates_len],
													candidates_costs[i*max_candidates_len],
													candidates_off_x[i*max_candidates_len],
													candidates_off_y[i*max_candidates_len]);

												fclose(mosaik2_result);
												exit(99);
											}
											fprintf(mosaik2_result, "%i	%li	%llu	%i	%i\n",
												i,
												candidates_index[i*max_candidates_len],
												candidates_costs[i*max_candidates_len],
												candidates_off_x[i*max_candidates_len],
												candidates_off_y[i*max_candidates_len]);
										}
										fclose(mosaik2_result);
									} // save intermediate result
								} // better candidate found
							} // for thumb_tile_count
						} // for primary_x
					} // for primary_y
				} //for shift_x
			} //for shift_y
	
		if(len < SIZE_64) {
			break;
		}
	}

	fclose(thumbs_db_tile_dims);
	fclose(thumbs_db_image_colors);
	fclose(thumbs_db_image_stddev);
	fclose(thumbs_db_invalid);
	fclose(duplicates_file);

	free( colors_red_int );
	free( colors_green_int );
	free( colors_blue_int );
	free( stddev_red_int );
	free( stddev_green_int );
	free( stddev_blue_int );


									if(debug) {
									for(int primary_tile_idx=0;primary_tile_idx<total_primary_tile_count;primary_tile_idx++) {

										uint32_t offset = primary_tile_idx*max_candidates_len;

									fprintf(stdout, "candidates_index[%i]:{", primary_tile_idx);
									for( int i=0;i<max_candidates_len;i++) {
										if(candidates_len[primary_tile_idx]==i)
											fprintf(stdout, "}");
										fprintf(stdout, "%lu,", candidates_index[offset + i]);
									}
									fprintf(stdout, "\ncandidates_costs[%i]:{", primary_tile_idx);
//									fprintf(stdout, "%05lli ", candidates_costs[primary_tile_idx + insert_pos ], diff_color0);
									for( int i=0;i<max_candidates_len;i++) {
										if(candidates_len[primary_tile_idx]==i)
											fprintf(stdout, "}");
										fprintf(stdout, "%llu,", candidates_costs[offset + i]);
									}
									fprintf(stdout, "\ncandidates_off[%i]:{", primary_tile_idx);
									for( int i=0;i<max_candidates_len;i++) {
										if(candidates_len[primary_tile_idx]==i)
											fprintf(stdout, "}");
										fprintf(stdout, "%i %i,", candidates_off_x[offset + i], candidates_off_y[offset + i]);
									}
									fprintf(stdout,"\n");
									}
									}



	//search the best unique candidates
	//initial all best images are set. the array candidates_ele is set to zero in all spaces.
	// it works like this
	// every candidate weak candidate in the forest front is elimanted by increase the candidates_elect counter
	// until there are 
	if(unique==1) {
		int unique_run;
		
		do {

			
			unique_run = 1;
			for(uint32_t i = 0;i<total_primary_tile_count-1;i++) {
				for(uint32_t j = 0;j<total_primary_tile_count;j++) {
					if(i==j)continue;
					uint32_t offset_i = i*max_candidates_len+candidates_elect[i];
					uint32_t offset_j = j*max_candidates_len+candidates_elect[j];
					if(debug)fprintf(stdout, "i:%i:%i %i j:%i:%i %i idx:%lu %lu costs:%llu %llu off:%i,%i %i,%i\n", 
						i,i*max_candidates_len,offset_i,                        j, j*max_candidates_len,offset_j,
						
						candidates_index[offset_i], candidates_index[offset_j],
						candidates_costs[offset_i], candidates_costs[offset_j], 
						candidates_off_x[offset_i], candidates_off_y[offset_i],
						candidates_off_x[offset_j], candidates_off_y[offset_j]);

					if( candidates_index[offset_i] == candidates_index[offset_j] ) {
						if( candidates_costs[offset_i] >= candidates_costs[offset_j]) 
							candidates_elect[i]++;
						else
							candidates_elect[j]++;
						if(candidates_elect[i] == total_primary_tile_count) {
							fprintf(stderr, "there were more candidates compared as possible. EXIT\n");
							exit(EXIT_FAILURE);
						}
						unique_run = 0;
		if(debug){for(uint32_t i=0;i<total_primary_tile_count;i++) {
			fprintf(stdout, "%i,", candidates_elect[i]);
		}
		fprintf(stdout, "\n");}
						break; // if the current election is skipped to the next candidate
						//
					}
				}
			}
		// if a do while round has any replacement => it is not unique yet
			if(debug)fprintf(stdout, ".");
		} while( unique_run == 0 );
if(debug) {
	fprintf(stdout, "\ncandidates_elect:");
		
		for(uint32_t i=0;i<total_primary_tile_count;i++) {
			fprintf(stdout, "%i,", candidates_elect[i]);
		}
		fprintf(stdout, "\n");
		}
		
	}


	//intialize with -1
	FILE *mosaik2_result = fopen(mp.dest_result_filename, "wb");
	if( mosaik2_result == NULL ) {
		fprintf(stderr, "result file (%s) could not be opened\n", mp.dest_result_filename);
		exit(EXIT_FAILURE);
	}

	for(uint32_t i=0;i<SIZE_PRIMARY;i++) {
	// total_primary_tile_count * max_candidates_len
		uint32_t offset = i * max_candidates_len + candidates_elect[i];
		fprintf(mosaik2_result,"%i	%li	%llu	%i	%i\n",
				i,candidates_index[offset],
				candidates_costs[offset],candidates_off_x[offset],candidates_off_y[offset]);
	}
	fclose(mosaik2_result);
									unsigned long long sum_ins = 0u;

									for( int i=0;i<total_primary_tile_count;i++) {
										sum_ins += candidates_ins[i];
									}
									fprintf(stdout,"sorting operations to create uniqueness:%llu\n",sum_ins);

	free( candidates_index );
	free(candidates_costs);
	free(candidates_off_x);
	free(candidates_off_y);
	free(candidates_len);
	free(candidates_elect);
	free(candidates_ins);

	return 0;
}


