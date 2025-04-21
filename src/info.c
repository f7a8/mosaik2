#include "libmosaik2.h"

void print_database(/*mosaik2_arguments *args,*/m2name mosaik2_db_name, mosaik2_database *md);
void print_element  (mosaik2_arguments *args, mosaik2_database *md, m2elem element_number);
void print_src_image(mosaik2_arguments *args, m2name mosaik2_db_name, mosaik2_database *md);

int mosaik2_info(mosaik2_arguments *args) {

	m2name mosaik2_database_name = args->mosaik2db;
	m2elem element_number = args->element_number;

	if(args->has_element_identifier == ELEMENT_NUMBER && element_number < 1  ) {
		fprintf(stderr, "illegal value of element_number %i. exit\n", element_number);
		exit(EXIT_FAILURE);
	}

	mosaik2_database md;
	mosaik2_database_init(&md, mosaik2_database_name);
	mosaik2_database_check(&md);
	mosaik2_database_lock_reader(&md);
	md.database_image_resolution = mosaik2_database_read_image_resolution(&md);


		if( args->has_element_identifier == ELEMENT_NUMBER ) {
		print_element(args, &md, element_number-1);
	} else if (args->has_element_identifier == ELEMENT_FILENAME ) {
		m2elem element_number1;
		int val = mosaik2_database_find_element_number(&md, args->element_filename, &element_number1);
		if(val==0)
			print_element(args, &md, element_number1);
		else {
			fprintf(stderr, "no filename found, must be equal with the original path and filename\n");
			exit(EXIT_FAILURE);
		}

	} else if( args->src_image != NULL ) {
		mosaik2_database_read_database_id(&md);
		print_src_image(args, mosaik2_database_name, &md);
	} else {
		mosaik2_database_read_database_id(&md);
		print_database(/*args,*/ mosaik2_database_name, &md);
	}

	return 0;
}

void print_database(/*mosaik2_arguments *args, */m2name mosaik2_db_name, mosaik2_database *md) {

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

	printf("histogram-color=%f %f %f\n",
			md->histogram_color[R], md->histogram_color[G], md->histogram_color[B]);
}

void print_element(mosaik2_arguments *args, mosaik2_database *md, m2elem element_number) {
	m2elem element_count = mosaik2_database_read_element_count(md);
	if( args->has_element_identifier == ELEMENT_NUMBER && element_number >= element_count) {
		fprintf(stderr, "element number out of range\n");
		exit(EXIT_FAILURE);
	}
	mosaik2_database_element mde = {0};
	mosaik2_database_read_element(md, &mde, element_number);

	m2rezo database_image_resolution = mosaik2_database_read_image_resolution(md);
	mosaik2_tile_infos ti = {0};
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
		case INVALID_NONE:
			printf("none\n");
			break;
		case INVALID_AUTOMATIC:
			printf("got_invalid\n");
			break;
		case INVALID_MANUAL:
			printf("invalidated_manually\n");
			break;
	}
	printf("duplicate=");
	switch(mde.duplicate) {
		case DUPLICATE_NONE: 
			printf("none\n");
			break;
		case DUPLICATE_MD5: 
			printf("md5_hash\n");
			break;
		case DUPLICATE_PHASH:
			printf("perceptual_hash\n");
	}

	#ifdef HAVE_PHASH
	if(mde.has_phash == HAS_PHASH) {
		printf("phash=%llx\n", mde.phash.hash);
	} else {
		printf("phash=none\n");
	}
	#endif

	if(mde.tileoffsets[0] == TILEOFFSET_UNSET && mde.tileoffsets[1] == TILEOFFSET_UNSET) {
		printf("tileoffsets=unset\n");
	} else if( mde.tileoffsets[0] != TILEOFFSET_INVALID && mde.tileoffsets[1] != TILEOFFSET_INVALID) {
		printf("tileoffsets=invalid\n");
	} else {
		printf("tileoffsets=%i,%i\n", mde.tileoffsets[0], mde.tileoffsets[1]);
	}
	m_free((void**)&mde.filename);

	printf("histogram-color=%f %f %f\n", mde.histogram_color[R], mde.histogram_color[G], mde.histogram_color[B]);
}

void print_src_image(mosaik2_arguments *args, m2name mosaik2_db_name, mosaik2_database *md) {
	printf("mosaik2db=%s\n", mosaik2_db_name);
	printf("src-image=%s\n", args->src_image);

	gdImagePtr im = read_image_from_file(args->src_image); // inefficient but solid, TODO from exif

	uint32_t image_width = (uint32_t)gdImageSX(im);
	uint32_t image_height = (uint32_t)gdImageSY(im);
	m2rezo database_image_resolution = mosaik2_database_read_image_resolution(md);
	m2rezo src_image_resolution = args->src_image_resolution;

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

	memset(&histogram_color,  0, sizeof(histogram_color));

	for(uint32_t x = ti.offset_x; x< ti.lx; x++) {
		for(uint32_t y= ti.offset_y;y<ti.ly;y++) {

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


	gdImageDestroy(im);

	mosaik2_database_read_histogram(md);

	printf("histogram-color-similarity=%f\n",
			sqrt(
				pow(histogram_color[R] - md->histogram_color[R],2) +
				pow(histogram_color[G] - md->histogram_color[G],2) +
				pow(histogram_color[B] - md->histogram_color[B],2)));

}
