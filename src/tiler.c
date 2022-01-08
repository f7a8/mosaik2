
#include "libmosaik2.h"


int mosaik2_tiler(uint32_t tile_count, uint32_t file_size) {

	if(tile_count<1 || tile_count >= 256) {
		fprintf(stderr, "illegal tile_count (%i): accepted range is 0 < tile_count < 256\n", tile_count);
		exit(EXIT_FAILURE);
	}
	if(file_size < 40001) {
		fprintf(stderr, "image file_size (%i) must be at least 40001 bytes\n", file_size);
		exit(EXIT_FAILURE);
	}

	int debug=0;
	int debug1=0;
	int html = 0;
	int out = 1;


	if(debug) printf("tile_count:%i,file_size:%i\n",tile_count,file_size);

  unsigned char *buffer = malloc(file_size);
  if(!buffer) {
    fprintf(stderr,"memory (%i) could not be allocated\n", file_size);
    exit(EXIT_FAILURE);
  }


//  clock_t start, end;
//  start = clock();

  size_t bytes_read = fread(buffer, 1, file_size, stdin); //works
  
	if(bytes_read != file_size) {
    fprintf(stderr, "image could not be loaded bytes_should:%i, bytes_read:%li", file_size, bytes_read);
    exit(EXIT_FAILURE);
  }
	
	char buf2[1];
	if(fread(buf2, 1,1, stdin)>0) {
		fprintf(stderr, "after file_size (%i) is at least one byte further more to read\n", file_size);
		exit(EXIT_FAILURE);
	}
	
	fclose(stdin);

	int file_type = get_file_type_from_buf(buffer,file_size);
	if(file_type == FT_ERR) {
		fprintf(stderr, "illegal image type 0\n");
		exit(EXIT_FAILURE);
	}
//  end = clock();
//  printf( "fread cpu time used: %f\n", ((double) (end-start)) / CLOCKS_PER_SEC );


  gdImagePtr im;
  if(file_type == FT_JPEG) 
		im = gdImageCreateFromJpegPtrEx(file_size, buffer, 0);
	//else if(file_type == FT_PNG)
	//	im = gdImageCreateFromPngPtr(file_size, buffer);
	else {
		fprintf(stderr, "illegal image type 1\n");
		exit(EXIT_FAILURE);
	}

	if(im == NULL ) {
  	free(buffer);

		fprintf(stderr,"image could not be instanciated\n");
		exit(EXIT_FAILURE);
	}

  gdImagePtr im2;
	uint8_t orientation = get_image_orientation(buffer, file_size);
	if(orientation == ORIENTATION_BOTTOM_RIGHT) {
		im2 = gdImageRotate180(im); 
		gdImageDestroy(im);
		im = im2;
	} else if(orientation == ORIENTATION_RIGHT_TOP) { 
		im2 = gdImageRotate270(im); // 270
		gdImageDestroy(im);
		im = im2;
	//	case ORIENTATION_BOTTOM_RIGHT: im = gdImageRotate90(im,0); break;//180
	//	case ORIENTATION_LEFT_BOTTOM: im = gdImageRotate90(im,0); break;
	} else if( orientation == ORIENTATION_LEFT_BOTTOM) {
		im2 = gdImageRotate90(im);
		gdImageDestroy(im);
		im = im2;
	}

	int width = gdImageSX(im);
	int height = gdImageSY(im);

	if(width < tile_count || height < tile_count) {
  	free(buffer);
		gdImageDestroy(im);

		fprintf(stderr,"image is too small, at least one dimension is smaller than the tile_count\n");
		exit(EXIT_FAILURE);
	}

	int short_dim = width<height?width:height;
	int long_dim  = width<height?height:width;

	int pixel_per_tile = ( short_dim - (short_dim % tile_count) ) / tile_count;
	double total_pixel_per_tile = pixel_per_tile * pixel_per_tile;
  
	int tile_x_count;
	int tile_y_count;

	if(short_dim == width){
		tile_x_count = tile_count;
		tile_y_count = height / pixel_per_tile;
	} else {
		tile_x_count = width / pixel_per_tile;
		tile_y_count = tile_count;
	}

	int total_tile_count = tile_x_count * tile_y_count;
  
	if( tile_x_count >= 256 || tile_y_count >= 256 ) {
  	free(buffer);
		gdImageDestroy(im);

		fprintf(stderr,"any tile dimension must be < 256\n");
		exit(EXIT_FAILURE);
	}


	unsigned char hash[MD5_DIGEST_LENGTH];
	memset(hash,0,MD5_DIGEST_LENGTH);
	//for(int i=0;i<16;i++) digest[i]=0;
	MD5_CTX md5;
	MD5_Init (&md5);
	MD5_Update (&md5, buffer, file_size);
	MD5_Final ( hash, &md5);
	//
//void md5_hash(const uint8_t message[], size_t len, uint32_t hash[static STATE_LEN]);
	//uint32_t hash[STATE_LEN] ={0};
	//md5_hash( buffer, file_size, hash );
	//if(debug1) fprintf(stderr, "state_len:%i\n",hash[0]);


	//	if(out) printf(" %02X%02X%02X %02X%02X%02X",red,green,blue, (int)round(abw_red),(int)round(abw_green),(int)round(abw_blue));
  free(buffer); // no need anymore of image byte data

	int offset_x = ((width - tile_x_count * pixel_per_tile)/2);
	int offset_y = ((height - tile_y_count * pixel_per_tile)/2);

	int lx = offset_x + pixel_per_tile * tile_x_count;
	int ly = offset_y + pixel_per_tile * tile_y_count;

	if(debug1)printf("width:%i, height:%i, lx:%i, ly:%i, offx:%i, offy:%i tile_dims:%i %i, pixel_per_tile:%i\n", width, height,lx,ly,offset_x,offset_y,tile_x_count,tile_y_count,pixel_per_tile);

	

	if(out) printf("%04X %04X %02X %02X", width, height, tile_x_count, tile_y_count);
	if(debug1) fprintf(stderr,"gathering imagedata\n");
    
	double      colors_red      [total_tile_count];
	double      colors_green    [total_tile_count];
	double      colors_blue     [total_tile_count];
	long double colors_abw_red  [total_tile_count];
	long double colors_abw_green[total_tile_count];
	long double colors_abw_blue [total_tile_count];


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
            
			//i and j are running from zero to the length of the cropped area
			//i1 and j1 are dealing with the corrected offset, so they are pointig to the valid pixels

			int tile_x = i/pixel_per_tile;
			int tile_y = j/pixel_per_tile;
			int tile_idx = tile_y * tile_x_count + tile_x;
			int color =  gdImageTrueColorPixel(im,i1,j1);
			//TODO one function 
			int red0 =   gdTrueColorGetRed(color);
			int green0 = gdTrueColorGetGreen(color);
			int blue0 =  gdTrueColorGetBlue(color);
			//int red0 = color >> 16 & 0xff;;
			//int green0 = color >> 8 & 0xff;;
			//int blue0 = color & 0xff;

			if(debug)printf("%i,%i,[%i %i],x:%i,y:%i,idx:%i,tile_count:%i,%i,px_tile:%i,lxy:%i,%i  r:%i,g:%i,b:%i    r:%f,g:%f,b:%f\n",i,j,i1,j1,tile_x,tile_y,tile_idx,tile_x_count,tile_y_count,pixel_per_tile,lx,ly,red0,green0,blue0, red0/total_pixel_per_tile, green0/total_pixel_per_tile, blue0/total_pixel_per_tile); 
			//add only fractions to prevent overflow in very large pixel_per_tile settings
			colors_red[tile_idx] +=   red0   / (total_pixel_per_tile);
//		if(debug && tile_idx==0) printf("avg_green:%f ", colors_green[tile_idx]);
			colors_green[tile_idx] += green0 / (total_pixel_per_tile);
			colors_blue[tile_idx] +=  blue0  / (total_pixel_per_tile);
		}
	}
    
	if(debug1) printf("avg_blue0:%f \n", colors_blue[0]);        



	if(debug1) fprintf(stderr,"tiling stddev data\n");

	double total_tile_count_d = total_tile_count;
	for(int j=0,j1=offset_y;j1<ly;j++,j1++){
		for(int i=0,i1=offset_x;i1<lx;i++,i1++){
			//if(debug) printf("avg_blue0:%f i:%i i1:%i lx:%i\n", colors_blue[0],i ,i1, lx);        

			int tile_x = i/pixel_per_tile;
			int tile_y = j/pixel_per_tile;
			int tile_idx = tile_y * tile_x_count + tile_x;
		if(debug) printf( "i:%i j:%i, tile_x:%i tile_y:%i tile_idx:%i c:%f %f %f\n", i,j,tile_x, tile_y, tile_idx, colors_red[tile_idx], colors_green[tile_idx],colors_blue[tile_idx]);
			int color =  gdImageTrueColorPixel(im,i1,j1);

			int red0 =   gdTrueColorGetRed(color);
			int green0 = gdTrueColorGetGreen(color);
			int blue0 =  gdTrueColorGetBlue(color);
			//int red0 = color >> 16 & 0xff;;
			//int green0 = color >> 8 & 0xff;;
			//int blue0 = color & 0xff;

			double red2 = colors_red[tile_idx] - red0;
			double green2 = colors_green[tile_idx] - green0;
			double blue2 = colors_blue[tile_idx] - blue0;

			colors_abw_red[tile_idx] += red2 * red2;
			colors_abw_green[tile_idx] += green2 * green2;
      colors_abw_blue[tile_idx] += blue2 * blue2;
		}
	}



	if(debug1)printf("dim:%i %i,off:%i %i,l:%i %i,tile_count:%i %i,pixel_per_tile:%i %f\n",width,height,offset_x,offset_y,lx,ly,tile_x_count,tile_y_count,pixel_per_tile,total_pixel_per_tile);
	if(html)printf("<html><head><style>table{ width:851px;height:566px; border-collapse: collapse;}td{padding:0;height:0.2em;width:0.2em;}</style></head><body><table>");


	if(debug1)fprintf(stderr,"starting output\n");

	for(int y=0;y<tile_y_count;y++) {
		if(html)printf("<tr>");

		for(int x=0;x<tile_x_count;x++) {	
			int i= y*tile_x_count+x;
			
			if(debug1) fprintf(stderr,"%i ", i);

			if(debug) printf("avg_blue0:%f ", colors_blue[i]);
			
			int red =   (int)round(colors_red[i]);
			int green = (int)round(colors_green[i]);
			int blue =  (int)round(colors_blue[i]);

			if(debug) printf("avg_red:%i %f", red,colors_red[i]);
			if(debug) printf("avg_green:%i %f", green,colors_green[i]);
			if(debug) printf("avg_blue:%i %f", blue,colors_blue[i]);

		  double abw_red = sqrt(colors_abw_red[i]/ ( total_pixel_per_tile - 1.0 ));
		  double abw_green = sqrt(colors_abw_green[i]/ (total_pixel_per_tile - 1.0));
		  double abw_blue = sqrt(colors_abw_blue[i]/ ( total_pixel_per_tile - 1.0));

			// to better utilize the byte space
			abw_red = abw_red * 2;
			abw_green = abw_green * 2;
			abw_blue*= abw_blue * 2;
			
			// in most cases not needed..
			if(abw_red>255) abw_red=255;
			if(abw_green>255) abw_green=255;
			if(abw_blue>255) abw_blue=255;
			if(html)printf("<td style='background-color:#%02x%02x%02x'>",red,green,blue);
//			if(html)printf("<td style='background-color:#%02x%02x%02x'>",(int)round(abw_red/2.0),(int)round(abw_green/2.0),(int)round(abw_blue/2.0));
			if(debug)printf( "%i:%02X%02X%02X %i,%i,%i  %02X%02X%02X\n",i,red,green,blue,red,green,blue,(int)round(abw_red),(int)round(abw_green),(int)round(abw_blue));
			if(out) printf(" %02X%02X%02X %02X%02X%02X",red,green,blue, (int)round(abw_red),(int)round(abw_green),(int)round(abw_blue));
			if(html)printf("</td>");
		}
	if(html)printf("</tr>");
	}
	if(html)printf("</table></body></html>");
	if(debug1) fprintf(stderr,"ending\n");
	gdImageDestroy(im);

	if(out) printf(" %02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X",
		hash[0]         , hash[1]         , hash[2]         , hash[3]         ,
		hash[4]         , hash[5]         , hash[6]         , hash[7]         ,
		hash[8]         , hash[9]         , hash[10]        , hash[11]        ,
		hash[12]        , hash[13]        , hash[14]        , hash[15]        );

	if(out) fflush(stdout);
	return 0;

}

