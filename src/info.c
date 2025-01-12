#include "libmosaik2.h"

void print_database (mosaik2_arguments *args, m2name mosaik2_db_name, mosaik2_database *md);
void print_element  (mosaik2_arguments *args, m2name mosaik2_db_name, mosaik2_database *md, m2elem element_number);
void print_src_image(mosaik2_arguments *args, m2name mosaik2_db_name, mosaik2_database *md);

int mosaik2_info(mosaik2_arguments *args) {

	m2name mosaik2_database_name = args->mosaik2db;
	m2elem element_number = args->element_number;

	if(args->has_element_identifier == ELEMENT_NUMBER && element_number < 1  ) {
		fprintf(stderr, "illegal value of element_number. exit\n");
		exit(EXIT_FAILURE);
	}

	mosaik2_database md;
	mosaik2_database_init(&md, mosaik2_database_name);
	mosaik2_database_check(&md);
	md.database_image_resolution = mosaik2_database_read_image_resolution(&md);

	if( args->has_element_identifier == ELEMENT_NUMBER ) {
		print_element(args, mosaik2_database_name, &md, element_number-1);
	} else if (args->has_element_identifier == ELEMENT_FILENAME ) {
		m2elem element_number;
		int val = mosaik2_database_find_element_number(&md, args->element_filename, &element_number);
		if(val==0)
			print_element(args, mosaik2_database_name, &md, element_number);
		else {
			fprintf(stderr, "no filename found, must be equal with the original path and filename\n");
			exit(EXIT_FAILURE);
		}

	} else if( args->src_image != NULL ) {
		mosaik2_database_read_database_id(&md);
		print_src_image(args, mosaik2_database_name, &md);
	} else {
		mosaik2_database_read_database_id(&md);
		print_database(args, mosaik2_database_name, &md);
	}

	return 0;
}

void print_database(mosaik2_arguments *args, m2name mosaik2_db_name, mosaik2_database *md) {

	printf("path=%s\n", mosaik2_db_name);
	printf("id=%s\n", md->id);
	printf("db-format-version=%i\n", MOSAIK2_DATABASE_FORMAT_VERSION);
	printf("database-image-resolution=%i\n", mosaik2_database_read_image_resolution(md));
	time_t createdat = mosaik2_database_read_createdat(md);
	time_t lastindexed = mosaik2_database_read_lastindexed(md);
	time_t lastmodified = mosaik2_database_read_lastmodified(md);

	printf("db-size=%li\n", mosaik2_database_read_size(md));
	printf("created-at=%s", ctime( &createdat));
	printf("last-indexed=%s", lastindexed>0?ctime( &lastindexed):"-\n");
	printf("last-modified=%s", lastmodified>0?ctime( &lastmodified):"-\n");
	printf("element-count=%i\n", mosaik2_database_read_element_count(md));
	printf("duplicates-count=%i\n", mosaik2_database_read_duplicates_count(md));
	printf("invalid-count=%i\n", mosaik2_database_read_invalid_count(md));
	printf("valid-count=%i\n", mosaik2_database_read_valid_count(md));
	printf("tileoffsets-count=%i\n", mosaik2_database_read_tileoffset_count(md));

	mosaik2_database_read_histogram(md);

	printf("histogram-color=%f %f %f\nhistogram-stddev=%f %f %f\n",
			md->histogram_color[R], md->histogram_color[G], md->histogram_color[B],
			md->histogram_stddev[R], md->histogram_stddev[G], md->histogram_stddev[B]);
}

void print_element(mosaik2_arguments *args, m2name mosaik2_db_name, mosaik2_database *md, m2elem element_number) {
	m2elem element_count = mosaik2_database_read_element_count(md);
	if( args->has_element_identifier == ELEMENT_NUMBER && (element_number < 0 || element_number >= element_count )) {
		fprintf(stderr, "element number out of range\n");
		exit(EXIT_FAILURE);
	}
	mosaik2_database_element mde;
	memset(&mde, 0, sizeof(mde));
	mosaik2_database_read_element(md, &mde, element_number);

	uint8_t database_image_resolution = mosaik2_database_read_image_resolution(md);
	//int src_image_resolution = database_image_resolution;
	mosaik2_tile_infos ti;
	memset(&ti, 0, sizeof(ti));
	mosaik2_tiler_infos_init(&ti, database_image_resolution, mde.imagedims[0], mde.imagedims[1]);

	printf("mosaik2db=%s\n", md->thumbs_db_name);
	printf("database-image-resolution=%i\n", database_image_resolution);
	printf("element-number=%i\n", element_number+1);
	printf("md5-hash="); for(int i=0;i<MD5_DIGEST_LENGTH;i++) { printf("%02x", mde.hash[i]); }
	printf("\nfilename=%s\n", mde.filename);
	printf("filesize=%li\n", mde.filesize);

	printf("image-dims=%ix%i\n", mde.imagedims[0], mde.imagedims[1]);
	printf("used-image-dims=%ix%i\n", ti.pixel_per_tile * ti.tile_x_count, ti.pixel_per_tile*ti.tile_y_count);
	printf("image-offset=%ix%i\n", ti.offset_x, ti.offset_y);
	printf("used-image-pixels=%i\n", ti.total_pixel_count);
	printf("ignored-image-pixels=%i\n", ti.ignored_pixel_count);

	printf("tile-dims=%ix%i\n", mde.tiledims[0], mde.tiledims[1]);
	printf("pixel-per-tile=%i^2\n",ti.pixel_per_tile);
	printf("timestamp=%s", ctime(&mde.timestamp));
	printf("invalid=");
	switch(mde.invalid) {
		case 0:
			printf("none\n");
			break;
		case 1:
			printf("got_invalid\n");
			break;
		case 2:
			printf("invalidated_manually\n");
			break;
	}

	printf("duplicate=");
	switch(mde.duplicate) {
		case 0: 
			printf("none\n");
			break;
		case 1: 
			printf("md5_hash\n");
			break;
		case 2:
			printf("perceptual_hash\n");
	}

	if(mde.tileoffsets[0] == 0xFF && mde.tileoffsets[1] == 0xFF) {
		printf("tileoffsets=unset\n");
	} else if( mde.tileoffsets[0] != 0 && mde.tileoffsets[1] != 0) {
		printf("tileoffsets=invalid\n");
	} else {
		printf("tileoffsets=%i,%i\n", mde.tileoffsets[0], mde.tileoffsets[1]);
	}
	free(mde.filename);

	printf("histogram-color=%f %f %f\n", mde.histogram_color[R], mde.histogram_color[G], mde.histogram_color[B]);
	printf("histogram-stddev=%f %f %f\n", mde.histogram_stddev[R], mde.histogram_stddev[G], mde.histogram_stddev[B]);
}

void print_src_image(mosaik2_arguments *args, m2name mosaik2_db_name, mosaik2_database *md) {
	printf("mosaik2db=%s\n", mosaik2_db_name);
	printf("src-image=%s\n", args->src_image);

	gdImagePtr im = read_image_from_file(args->src_image); // inefficient but solid, TODO from exif

	int image_width = gdImageSX(im);
	int image_height = gdImageSY(im);
	uint8_t database_image_resolution = mosaik2_database_read_image_resolution(md);
	int src_image_resolution = args->num_tiles;

	mosaik2_tile_infos ti;
	memset(&ti, 0, sizeof(ti));
	mosaik2_tile_infos_init(&ti, database_image_resolution, src_image_resolution, image_width, image_height);

	printf("database-image-resolution=%i\n", database_image_resolution);
	printf("source-image-resolution=%i\n", src_image_resolution);
	printf("image-dims=%ix%i\n", image_width, image_height);
	printf("used-image-dims=%ix%i\n", ti.pixel_per_tile * ti.tile_x_count, ti.pixel_per_tile*ti.tile_y_count);
	printf("image-offset=%ix%i\n", ti.offset_x, ti.offset_y);
	printf("image-pixels=%i\n", image_width * image_height);
	printf("used-image-pixels=%i\n", ti.total_pixel_count);
	printf("ignored-image-pixels=%i\n", ti.ignored_pixel_count);
	printf("primary-tiles=%ix%i\n", ti.primary_tile_x_count, ti.primary_tile_y_count);
	printf("total-primary-tiles=%i\n", ti.primary_tile_x_count * ti.primary_tile_y_count);
	printf("tile-dims=%ix%i\n", ti.tile_x_count, ti.tile_y_count);
	printf("pixel-per-primary-tile=%i\n", ti.pixel_per_primary_tile);

	printf("pixel-per-tile=%i^2\n",ti.pixel_per_tile);

	double total_pixel_count_f = (double) ti.total_pixel_count;
	double histogram_color[RGB];
	double histogram_stddev[RGB];

	memset(&histogram_color,  0, sizeof(histogram_color));
	memset(&histogram_stddev, 0, sizeof(histogram_stddev));

	for(int x = ti.offset_x; x< ti.lx; x++) {
		for(int y= ti.offset_y;y<ti.ly;y++) {

			int color =  gdImageTrueColorPixel(im,x,y);
			int red0 =   gdTrueColorGetRed(color);
			int green0 = gdTrueColorGetGreen(color);
			int blue0 =  gdTrueColorGetBlue(color);

			histogram_color[R] += red0 / total_pixel_count_f;
			histogram_color[G] += green0 / total_pixel_count_f;
			histogram_color[B] += blue0 / total_pixel_count_f;
			//printf("%i : %i %i %i / %f = %f %f %f\n", color, red0, green0, blue0, total_pixel_count_f, histogram_color[R], histogram_color[G], histogram_color[B]);
			//printf("histogram-color:%f %f %f\n", histogram_color[R], histogram_color[G], histogram_color[B]);
		}
	}
	printf("histogram-color:%f %f %f\n", histogram_color[R], histogram_color[G], histogram_color[B]);


	for(int x = ti.offset_x; x < ti.lx; x++ ) {
		for(int y = ti.offset_y;y<ti.ly;y++) {
			int color = gdImageTrueColorPixel(im,x,y);
			int red0  = gdTrueColorGetRed(color);
			int green0= gdTrueColorGetGreen(color);
			int blue0 = gdTrueColorGetBlue(color);

			histogram_stddev[R] += pow(histogram_color[R] - red0,2);
			histogram_stddev[G] += pow(histogram_color[G] - green0,2);
			histogram_stddev[B] += pow(histogram_color[B] - blue0,2);
		}
	}
	gdImageDestroy(im);

	histogram_stddev[R] = sqrt(histogram_stddev[R] / (total_pixel_count_f - 1));
	histogram_stddev[G] = sqrt(histogram_stddev[G] / (total_pixel_count_f - 1));
	histogram_stddev[B] = sqrt(histogram_stddev[B] / (total_pixel_count_f - 1));
	printf("histogram-stddev:%f %f %f\n", histogram_stddev[R], histogram_stddev[G], histogram_stddev[B]);
	mosaik2_database_read_histogram(md);

	printf("histogram-color-similarity=%f\n",
			sqrt(
				pow(histogram_color[R] - md->histogram_color[R],2) +
				pow(histogram_color[G] - md->histogram_color[G],2) +
				pow(histogram_color[B] - md->histogram_color[B],2)));
	printf("histogram-stddev-similarity=%f\n",
			sqrt(
				pow(histogram_stddev[R] - md->histogram_stddev[R],2) +
				pow(histogram_stddev[G] - md->histogram_stddev[G],2) +
				pow(histogram_stddev[B] - md->histogram_stddev[B],2)));

}
