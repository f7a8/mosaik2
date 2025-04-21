
#include "libmosaik2.h"


int mosaik2_tiler(mosaik2_arguments *args, mosaik2_database *md, mosaik2_indextask *task) {
	//	print_usage("t read0");
	if( mosaik2_indextask_read_image(/*md, */task) ) {
		fprintf(stderr, "could not read image\n");
		exit(EXIT_FAILURE);
	}
	//print_usage("t read1");

	uint32_t file_size = task->filesize;
	//TODO to late?
	//check_resolution(md->database_image_resolution);

	uint32_t min_file_size = 10000;
	uint32_t max_file_size = 100000000;
	if(file_size < min_file_size) {
		fprintf(stderr, "image file_size (%i) must be at least %i bytes\n", file_size, min_file_size);
		exit(EXIT_FAILURE);
	}
	if(file_size > max_file_size) {
		fprintf(stderr, "image file_size (%i) must be below %i bytes\n", file_size, max_file_size);
		exit(EXIT_FAILURE);
	}

	int debug=0;
	int out  = 0;
	//unsigned char *buf = task->image_data;

	m2ftype file_type = get_file_type_from_buf(task->image_data, file_size);
	if(file_type == FT_ERR) {
		fprintf(stderr, "illegal image type (%s)\n", task->filename);
		exit(EXIT_FAILURE);
	}

	//print_usage("t_create0");
	gdImagePtr im = read_image_from_buf(task->image_data, file_size);

	mosaik2_tile_infos ti = {0};
	mosaik2_tiler_infos_init(&ti, md->database_image_resolution, (uint32_t)gdImageSX(im), (uint32_t)gdImageSY(im));

	task->imagedims[0] = ti.image_width;
	task->imagedims[1] = ti.image_height;
	task->tiledims[0]= ti.tile_x_count;
	task->tiledims[1]= ti.tile_y_count;
	task->total_tile_count = ti.total_tile_count;

	#if OPENSSL_VERSION_NUMBER >= 0x10100000L
	EVP_MD_CTX *mdctx = EVP_MD_CTX_new();
	#else
	EVP_MD_CTX *mdctx = EVP_MD_CTX_create();
	#endif
	if( mdctx==NULL ) {
		fprintf(stderr, "could not create digest context\n");
		exit(EXIT_FAILURE);
	}
	if(!EVP_DigestInit_ex(mdctx, EVP_md5(), NULL)) {
		fprintf(stderr, "could not iit digest content\n");
		exit(EXIT_FAILURE);
	}
	if(!EVP_DigestUpdate(mdctx, task->image_data, file_size)) {
		fprintf(stderr, "could not update digest\n");
		exit(EXIT_FAILURE);
	}
	unsigned int md5_digest_length = MD5_DIGEST_LENGTH;
	if(!EVP_DigestFinal_ex(mdctx, task->hash, &md5_digest_length)) {
		fprintf(stderr, "could finish digest\n");
		exit(EXIT_FAILURE);
	}
	#if OPENSSL_VERSION_NUMBER >= 0x10100000L
	EVP_MD_CTX_free(mdctx);
	#else
	EVP_MD_CTX_destroy(mdctx);
	#endif

	//check if already indexed TODO

	//print_usage("t_diggest");


	m_free((void**)&task->image_data); // no need anymore of image byte data


	if(args->verbose)printf("width:%i, height:%i, lx:%i, ly:%i, offx:%i, offy:%i tile_dims:%i %i, pixel_per_tile:%i\n", ti.image_width, ti.image_height,ti.lx,ti.ly,ti.offset_x,ti.offset_y,ti.tile_x_count,ti.tile_y_count,ti.pixel_per_tile);



	if(out) printf("%04X %04X %02X %02X", ti.image_width, ti.image_height, ti.tile_x_count, ti.tile_y_count);
	if(args->verbose) fprintf(stderr,"gathering imagedata\n");

	double colors[RGB*ti.total_tile_count];

	memset(&colors, 0, RGB*ti.total_tile_count*sizeof(double));

	task->colors = m_calloc(RGB*ti.total_tile_count, sizeof(uint8_t));

	int red0, green0, blue0;
	for(uint32_t j=0,j1=ti.offset_y;j1<ti.ly;j++,j1++){
		for(uint32_t i=0,i1=ti.offset_x;i1<ti.lx;i++,i1++){

			//i and j are running from zero to the length of the cropped area
			//i1 and j1 are dealing with the corrected offset, so they are pointig to the valid pixels

			uint32_t tile_x = i/ti.pixel_per_tile;
			uint32_t tile_y = j/ti.pixel_per_tile;
			uint32_t tile_idx = tile_y * ti.tile_x_count + tile_x;
			int color =  gdImageTrueColorPixel(im,i1,j1);

			red0 =   gdTrueColorGetRed(color);
			green0 = gdTrueColorGetGreen(color);
			blue0 =  gdTrueColorGetBlue(color);

			if(debug)printf("%i,%i,[%i %i],x:%i,y:%i,idx:%i,database_image_resolution:%i,%i,px_tile:%i,lxy:%i,%i  r:%i,g:%i,b:%i    r:%f,g:%f,b:%f\n",i,j,i1,j1,tile_x,tile_y,tile_idx,ti.tile_x_count,ti.tile_y_count,ti.pixel_per_tile,ti.lx,ti.ly,red0,green0,blue0, red0/ti.total_pixel_per_tile, green0/ti.total_pixel_per_tile, blue0/ti.total_pixel_per_tile);
			//add only fractions to prevent overflow in very large pixel_per_tile settings
			colors[tile_idx*RGB+R] +=   red0   / (ti.total_pixel_per_tile);
			colors[tile_idx*RGB+G] +=   green0 / (ti.total_pixel_per_tile);
			colors[tile_idx*RGB+B] +=   blue0  / (ti.total_pixel_per_tile);
		}
	}


	gdImageDestroy(im);

	if(args->verbose)printf("dim:%i %i,off:%i %i,l:%i %i,tile_count:%i %i,pixel_per_tile:%i %f\n",ti.image_width,ti.image_height,ti.offset_x,ti.offset_y,ti.lx,ti.ly,ti.tile_x_count,ti.tile_y_count,ti.pixel_per_tile,ti.total_pixel_per_tile);


	if(args->verbose)fprintf(stderr,"starting output\n");

	for(uint32_t y=0;y<ti.tile_y_count;y++) {
		for(uint32_t x=0;x<ti.tile_x_count;x++) {

 			uint32_t i= y*ti.tile_x_count+x;

			if(args->verbose) fprintf(stderr,"%i ", i);

			red0 =   (int)round(colors[i*RGB+R]);
			green0 = (int)round(colors[i*RGB+G]);
			blue0 =  (int)round(colors[i*RGB+B]);

			task->colors[i*RGB+R] = red0;
			task->colors[i*RGB+G] = green0;
			task->colors[i*RGB+B] = blue0;

			
			


			if(debug)printf( "%i:%02X%02X%02X %i,%i,%i\n",i,red0,green0,blue0,red0,green0,blue0);
			if(out) printf(" %02X%02X%02X",red0,green0,blue0);
		}
	}
	if(args->verbose) fprintf(stderr,"ending\n");
	//print_usage("t_final");

	//TODO
	//if(args->dry_run == 0)
	mosaik2_index_write_to_disk(md, task);

	m_free((void**)&task->colors);

	return 0;
}

