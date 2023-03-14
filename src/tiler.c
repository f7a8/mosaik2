
#include "libmosaik2.h"


int mosaik2_tiler(mosaik2_arguments *args, mosaik2_database *md, mosaik2_indextask *task) {
	//	print_usage("t read0");
	if( mosaik2_indextask_read_image(task) ) {
		fprintf(stderr, "could not read image\n");
		exit(EXIT_FAILURE);
	}
	//print_usage("t read1");

	uint32_t file_size = task->filesize;
	check_resolution(md->database_image_resolution);

	int min_file_size = 10000;
	int max_file_size = 100000000;
	if(file_size < min_file_size) {
		fprintf(stderr, "image file_size (%i) must be at least %i bytes\n", file_size, min_file_size);
		exit(EXIT_FAILURE);
	}
	if(file_size > max_file_size) {
		fprintf(stderr, "image file_size (%i) must be below %i bytes\n", file_size, max_file_size);
		exit(EXIT_FAILURE);
	}

	int debug=0;
	int debug1=0;
	int out  = 0;
	unsigned char *buffer = task->image_data;

	int file_type = get_file_type_from_buf(buffer,file_size);
	if(file_type == FT_ERR) {
		fprintf(stderr, "illegal image type 0\n");
		exit(EXIT_FAILURE);
	}

	//print_usage("t_create0");
	gdImagePtr im = read_image_from_buf(buffer, file_size);

	mosaik2_tile_infos ti;
	memset(&ti, 0, sizeof(ti));
	mosaik2_tiler_infos_init(&ti, md->database_image_resolution, gdImageSX(im), gdImageSY(im));


	task->tiledims[0]= ti.tile_x_count;
	task->tiledims[1]= ti.tile_y_count;
	task->total_tile_count = ti.total_tile_count;

	MD5_CTX md5;
	MD5_Init (&md5);
	MD5_Update (&md5, buffer, file_size);
	MD5_Final ( task->hash, &md5);

	//check if already indexed TODO

	//print_usage("t_diggest");


	//	if(out) printf(" %02X%02X%02X %02X%02X%02X",red,green,blue, (int)round(abw_red),(int)round(abw_green),(int)round(abw_blue));
	free(buffer); // no need anymore of image byte data


	if(debug1)printf("width:%i, height:%i, lx:%i, ly:%i, offx:%i, offy:%i tile_dims:%i %i, pixel_per_tile:%i\n", ti.image_width, ti.image_height,ti.lx,ti.ly,ti.offset_x,ti.offset_y,ti.tile_x_count,ti.tile_y_count,ti.pixel_per_tile);



	if(out) printf("%04X %04X %02X %02X", ti.image_width, ti.image_height, ti.tile_x_count, ti.tile_y_count);
	if(debug1) fprintf(stderr,"gathering imagedata\n");

	double colors[RGB*ti.total_tile_count];
	double stddev[RGB*ti.total_tile_count];

	memset(&colors, 0, RGB*ti.total_tile_count*sizeof(double));
	memset(&stddev, 0, RGB*ti.total_tile_count*sizeof(double));

	task->colors = m_calloc(RGB*ti.total_tile_count, sizeof(uint8_t));
	task->stddev = m_calloc(RGB*ti.total_tile_count, sizeof(uint8_t));

	int red0, green0, blue0;
	for(int j=0,j1=ti.offset_y;j1<ti.ly;j++,j1++){
		for(int i=0,i1=ti.offset_x;i1<ti.lx;i++,i1++){

			//i and j are running from zero to the length of the cropped area
			//i1 and j1 are dealing with the corrected offset, so they are pointig to the valid pixels

			int tile_x = i/ti.pixel_per_tile;
			int tile_y = j/ti.pixel_per_tile;
			int tile_idx = tile_y * ti.tile_x_count + tile_x;
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


	if(debug1) fprintf(stderr,"tiling stddev data\n");

	double red2, green2, blue2;

	//double total_tile_count_d = total_tile_count;
	for(int j=0,j1=ti.offset_y;j1<ti.ly;j++,j1++){
		for(int i=0,i1=ti.offset_x;i1<ti.lx;i++,i1++){
			//if(debug) printf("avg_blue0:%f i:%i i1:%i lx:%i\n", colors_blue[0],i ,i1, lx);

			int tile_x = i/ti.pixel_per_tile;
			int tile_y = j/ti.pixel_per_tile;
			int tile_idx = tile_y * ti.tile_x_count + tile_x;
			int color =  gdImageTrueColorPixel(im,i1,j1);

			int red0 =   gdTrueColorGetRed(color);
			int green0 = gdTrueColorGetGreen(color);
			int blue0 =  gdTrueColorGetBlue(color);

			red2 =   colors[tile_idx*RGB+R] - red0;
			green2 = colors[tile_idx*RGB+G] - green0;
			blue2 =  colors[tile_idx*RGB+B] - blue0;

			stddev[tile_idx*RGB+R] += pow(red2,2);// * red2;
			stddev[tile_idx*RGB+G] += pow(green2,2);// * green2;
			stddev[tile_idx*RGB+B] += pow(blue2,2);// * blue2;
		}
	}
	//print_usage("t_stddev");

	gdImageDestroy(im);

	if(debug1)printf("dim:%i %i,off:%i %i,l:%i %i,tile_count:%i %i,pixel_per_tile:%i %f\n",ti.image_width,ti.image_height,ti.offset_x,ti.offset_y,ti.lx,ti.ly,ti.tile_x_count,ti.tile_y_count,ti.pixel_per_tile,ti.total_pixel_per_tile);


	if(debug1)fprintf(stderr,"starting output\n");
	double stddev_red, stddev_green, stddev_blue;

	for(int y=0;y<ti.tile_y_count;y++) {

		for(int x=0;x<ti.tile_x_count;x++) {
			int i= y*ti.tile_x_count+x;

			if(debug1) fprintf(stderr,"%i ", i);

			red0 =   (int)round(colors[i*RGB+R]);
			green0 = (int)round(colors[i*RGB+G]);
			blue0 =  (int)round(colors[i*RGB+B]);

			task->colors[i*RGB+R] = red0;
			task->colors[i*RGB+G] = green0;
			task->colors[i*RGB+B] = blue0;

			stddev_red =   sqrt(stddev[i*RGB+R]/ ( ti.total_pixel_per_tile - 1.0 ));
			stddev_green = sqrt(stddev[i*RGB+G]/ ( ti.total_pixel_per_tile - 1.0 ));
			stddev_blue =  sqrt(stddev[i*RGB+B]/ ( ti.total_pixel_per_tile - 1.0 ));
			// to better utilize the byte space
			// in most cases below 255 ..
			task->stddev[i*RGB+R] = stddev_red   * 2 > 255 ? 255 : stddev_red   * 2;
			task->stddev[i*RGB+G] = stddev_green * 2 > 255 ? 255 : stddev_green * 2;
			task->stddev[i*RGB+B] = stddev_blue  * 2 > 255 ? 255 : stddev_blue  * 2;


			if(debug)printf( "%i:%02X%02X%02X %i,%i,%i  %02X%02X%02X\n",i,red0,green0,blue0,red0,green0,blue0,(int)round(stddev_red),(int)round(stddev_green),(int)round(stddev_blue));
			if(out) printf(" %02X%02X%02X %02X%02X%02X",red0,green0,blue0, (int)round(stddev_red),(int)round(stddev_green),(int)round(stddev_blue));
		}
	}
	if(debug1) fprintf(stderr,"ending\n");
	//print_usage("t_final");

	//TODO
	//if(args->dry_run == 0)
	mosaik2_index_write_to_disk(md, task);

	free(task->colors);
	free(task->stddev);

	return 0;

}

