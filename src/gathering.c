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
	m2name dest_filename = args->dest_image;
	int ratio = args->color_stddev_ratio;
	int unique = args->unique;
	int fast_unique = args->fast_unique;
	m2name mosaik2_database_name = args->mosaik2db;
	uint8_t debug = 0;
	uint8_t debug1 = 0;
	const uint8_t html = 0;
	const uint8_t out = 0;
	float image_ratio;
	float stddev_ratio;
	int color_distance = args->color_distance;
	uint8_t database_image_resolution;
	m2elem element_count;
	uint32_t SIZE_PRIMARY; // alias for  ti.total_primary_tile_count
	uint32_t valid_md_element_count;
	uint32_t needed_md_element_count;
	size_t max_candidates_len;
	size_t total_candidates_count;
	double *colors;
	long double *stddev;
	int *colors_int;
	int *stddev_int;


	m2file thumbs_db_tiledims_file;
	m2file thumbs_db_tileoffsets_file;
	m2file thumbs_db_imagecolors_file;
	m2file thumbs_db_imagestddev_file;
	m2file thumbs_db_invalid_file;
	m2file thumbs_db_duplicates_file;
	uint8_t tile_dims_buf[BUFSIZ];
	unsigned char tileoffsets_buf[BUFSIZ];
	unsigned char colors_buf[256*256*RGB]; //maximal possible data amout per database candidate
	unsigned char stddev_buf[256*256*RGB];
	unsigned char invalid_buf[BUFSIZ];
	unsigned char duplicates_buf[BUFSIZ];
	m2file primarytiledims_file;
	uint32_t idx = 0;
	uint32_t candidates_insert=0;
	uint32_t candidates_pop=0;
	uint32_t candidates_toobad=0;
	float diff_best=FLT_MAX,diff_improv=FLT_MIN,diff_worst=0;

	m2file mosaik2_result;
	float candidate_best_costs = FLT_MAX, candidate_worst_costs=0;



	mosaik2_database md;
	mosaik2_database_init(&md, mosaik2_database_name);
	mosaik2_database_read_database_id(&md);

	mosaik2_database_check(&md);

	mosaik2_project_check_dest_filename(dest_filename);
	database_image_resolution = mosaik2_database_read_image_resolution(&md);
	element_count = mosaik2_database_read_element_count(&md);

	if (database_image_resolution * database_image_resolution * (2 * RGB * UINT8_MAX) > UINT32_MAX) {
		// can candidates_costs contain the badest possible costs?
		fprintf(stderr, "thumb tile size too high for internal data structure\n");
		exit(EXIT_FAILURE);
	}

	if (ratio < 0 || ratio > 100) {
		fprintf(stderr, "ratio is only allowed as 0<=ratio<=100\n");
		exit(EXIT_FAILURE);
	}

	if (unique < 0 || unique > 1) {
		fprintf(stderr, "unique is only allowed to be 0 (one image may be used mutiple times) or 1 (one image is just used once)\n");
		exit(EXIT_FAILURE);
	}

	image_ratio = ratio / 100.0;
	stddev_ratio = 1.0 - image_ratio;

	mosaik2_project mp = {.ratio = ratio, .unique = unique, .fast_unique = fast_unique, .primary_tile_count = primary_tile_count};

	mosaik2_project_init(&mp, md.id, dest_filename);

	if (debug)
		printf("primary_tile_count:%i,database_image_resolution:%i\n", primary_tile_count, database_image_resolution);

	unsigned char *buf = read_stdin(&mp.file_size);
	m_fclose(stdin);

	gdImagePtr im = read_image_from_buf(buf, mp.file_size);
	free(buf);

	mosaik2_tile_infos ti;
	memset(&ti, 0, sizeof(ti));
	mosaik2_tile_infos_init(&ti, database_image_resolution, args->num_tiles, gdImageSX(im), gdImageSY(im));

	if (ti.image_width < ti.tile_count || ti.image_height < ti.tile_count) {
		fprintf(stderr, "image too small\n");
		exit(EXIT_FAILURE);
	}

	if (debug)
		fprintf(stdout, "ti.pixel_per_tile=ti.short_dim / ti.tile_count => %i = %i / %i\n", ti.pixel_per_tile, ti.short_dim, ti.tile_count);

	SIZE_PRIMARY = ti.total_primary_tile_count;

	if (debug)
		printf("image_dims:%i %i, primary_tile_dims:%i %i(%i), tile_dims:%i %i, l:%i %i, off:%i %i pixel_per:%i %i\n", ti.image_width, ti.image_height, ti.primary_tile_x_count, ti.primary_tile_y_count, ti.total_primary_tile_count, ti.tile_x_count, ti.tile_y_count, ti.lx, ti.ly, ti.offset_x, ti.offset_y, ti.pixel_per_primary_tile, ti.pixel_per_tile);

	valid_md_element_count = mosaik2_database_read_valid_count(&md);
	needed_md_element_count = unique || fast_unique? ti.total_primary_tile_count : 1;
	if( valid_md_element_count < needed_md_element_count ) {
		fprintf(stderr, "there are too few valid candidates (%u) %sthan needed (%u)\n", valid_md_element_count, unique ? "for unique ":"", needed_md_element_count);
		exit(EXIT_FAILURE);
	}

	if(args->quiet == 0) {
		printf("src-image-resolution:%ix%i\n", ti.primary_tile_x_count, ti.primary_tile_y_count);
	}

	/*
	   The data structure of the candidates list.
	   For each tile are stored as many candidates as there are primarys tiles. With the largest possible number of multiple selected tiles, a later reduction procedure can lead to only once (best) used tiles, without there being too few alternatives.
	   */

	max_candidates_len = unique == 1 ? ti.total_primary_tile_count : 1;
	total_candidates_count = ti.total_primary_tile_count * max_candidates_len;

	if (debug)
		fprintf(stderr, "max_candidates_len:%li, total_candidates_count:%li\n", max_candidates_len, total_candidates_count);

	Heap heap[SIZE_PRIMARY];
	memset(&heap, 0, sizeof(heap));
	//allocate only one memory chunk and spread it over the heaps
	mosaik2_database_candidate *mdc = m_calloc(total_candidates_count+max_candidates_len, sizeof(mosaik2_database_candidate));

	for(uint32_t i=0;i<SIZE_PRIMARY;i++) {
		heap_init(&heap[i], mdc+i*(max_candidates_len)+1);
	}

	if (out)
		printf("%04X %04X %02X %02X", ti.image_width, ti.image_height, ti.tile_x_count, ti.tile_y_count);

	colors       = m_calloc(ti.total_tile_count * RGB, sizeof(double));
	colors_int   = m_calloc(ti.total_tile_count * RGB, sizeof(int));
	stddev       = m_calloc(ti.total_tile_count * RGB, sizeof(long double));
	stddev_int   = m_calloc(ti.total_tile_count * RGB, sizeof(int));

	for (int j = 0, j1 = ti.offset_y; j1 < ti.ly; j++, j1++) {
		for (int i = 0, i1 = ti.offset_x; i1 < ti.lx; i++, i1++) {
			// i runs from 0 to length of cropped area
			// j also
			// i1 und j1 are corrected by the offset

			int tile_x = i / ti.pixel_per_tile;
			int tile_y = j / ti.pixel_per_tile;
			int tile_idx = tile_y * ti.tile_x_count + tile_x;
			int color = gdImageTrueColorPixel(im, i1, j1);
			// TODO one function
			int red0 = gdTrueColorGetRed(color);
			int green0 = gdTrueColorGetGreen(color);
			int blue0 = gdTrueColorGetBlue(color);

			if (debug1)
				printf("%i,%i,[%i %i],x:%i,y:%i,idx:%i,ti.tile_count:%i,%i,px_tile:%i,lxy:%i,%i  r:%i,g:%i,b:%i    r:%f,g:%f,b:%f\n", i, j, i1, j1, tile_x, tile_y, tile_idx, ti.tile_x_count, ti.tile_y_count, ti.pixel_per_tile, ti.lx, ti.ly, red0, green0, blue0, red0 / ti.total_pixel_per_tile, green0 / ti.total_pixel_per_tile, blue0 / ti.total_pixel_per_tile);
			// add only fractions to prevent overflow in very large ti.pixel_per_tile settings
			colors[tile_idx *RGB+ R] += red0 / (ti.total_pixel_per_tile);
			colors[tile_idx *RGB+ G] += green0 / (ti.total_pixel_per_tile);
			colors[tile_idx *RGB+ B] += blue0 / (ti.total_pixel_per_tile);

		}
	}

	for (int j = 0, j1 = ti.offset_y; j1 < ti.ly; j++, j1++) {
		for (int i = 0, i1 = ti.offset_x; i1 < ti.lx; i++, i1++) {

			int tile_x = i / ti.pixel_per_tile;
			int tile_y = j / ti.pixel_per_tile;
			int tile_idx = tile_y * ti.tile_x_count + tile_x;

			if (debug1)
				printf("i:%i j:%i, tile_x:%i tile_y:%i tile_idx:%i c:%f %f %f\n", i, j, tile_x, tile_y, tile_idx, colors[tile_idx + R], colors[tile_idx+G], colors[tile_idx+B]);
			int color = gdImageTrueColorPixel(im, i1, j1);

			int red0 = gdTrueColorGetRed(color);
			int green0 = gdTrueColorGetGreen(color);
			int blue0 = gdTrueColorGetBlue(color);

			double red2 = colors[tile_idx +R] - red0;
			double green2 = colors[tile_idx + G] - green0;
			double blue2 = colors[tile_idx + B] - blue0;

			stddev[tile_idx *RGB + R] += red2 * red2;
			stddev[tile_idx *RGB + G] += green2 * green2;
			stddev[tile_idx *RGB + B] += blue2 * blue2;
		}
	}

	if (debug)
		printf("dim:%i %i,off:%i %i,l:%i %i,primary_tile_cout:%i %i,ti.tile_count:%i %i,ti.pixel_per_tile:%i %f\n", ti.image_width, ti.image_height, ti.offset_x, ti.offset_y, ti.lx, ti.ly, ti.primary_tile_x_count, ti.primary_tile_y_count, ti.tile_x_count, ti.tile_y_count, ti.pixel_per_tile, ti.total_pixel_per_tile);
	if (html)
		printf("<html><head><style>table{ width:851px;height:566px; border-collapse: collapse;}td{padding:0;height:0.2em;width:0.2em;}</style></head><body><table>");

	if (out) {

		for (int y = 0; y < ti.tile_y_count; y++) {
			if (html)
				printf("<tr>");

			for (int x = 0; x < ti.tile_x_count; x++) {
				int i = y * ti.tile_x_count + x;
				int red =   (int)round(colors[i*RGB+R]);
				int green = (int)round(colors[i*RGB+G]);
				int blue =  (int)round(colors[i*RGB+B]);

				if (debug1)
					printf("avg_red:%i %f", red, colors[i*RGB+R]);
				if (debug1)
					printf("avg_green:%i %f", green, colors[i*RGB+G]);
				if (debug1)
					printf("avg_blue:%i %f", blue, colors[i*RGB+B]);

				double abw_red = sqrt(stddev[i*RGB+R]   / (ti.total_pixel_per_tile - 1.0));
				double abw_green = sqrt(stddev[i*RGB+G] / (ti.total_pixel_per_tile - 1.0));
				double abw_blue = sqrt(stddev[i*RGB+B]  / (ti.total_pixel_per_tile - 1.0));

				// to better utilize the byte space
				abw_red = abw_red * 2;
				abw_green = abw_green * 2;
				abw_blue *= abw_blue * 2;

				// in most cases this limit is not necessary..
				if (abw_red > 255)
					abw_red = 255;
				if (abw_green > 255)
					abw_green = 255;
				if (abw_blue > 255)
					abw_blue = 255;

				stddev[i*RGB+R] = abw_red;
				stddev[i*RGB+G] = abw_green;
				stddev[i*RGB+B] = abw_blue;

				if (html)
					printf("<td style='background-color:#%02x%02x%02x'>", red, green, blue);
				if (debug)
					printf("%i:%i %i %i  %i %i %i\n", i, red, green, blue, (int)round(abw_red), (int)round(abw_green), (int)round(abw_blue));
				if (out)
					printf(" %02X%02X%02X %02X%02X%02X", red, green, blue, (int)round(abw_red), (int)round(abw_green), (int)round(abw_blue));
				if (html)
					printf("</td>");
			}
			if (html)
				printf("</tr>");
		}
		if (html)
			printf("</table></body></html>");
	}

	gdImageDestroy(im);

	for (uint32_t i = 0; i < ti.total_tile_count; i++) {
		colors_int[i*RGB+R] = (int)round(colors[i*RGB+R]);
		colors_int[i*RGB+G] = (int)round(colors[i*RGB+G]);
		colors_int[i*RGB+B] = (int)round(colors[i*RGB+B]);

		double sr = sqrt(stddev[i*RGB+R] / (ti.total_pixel_per_tile - 1.0));
		double sg = sqrt(stddev[i*RGB+G] / (ti.total_pixel_per_tile - 1.0));
		double sb = sqrt(stddev[i*RGB+B] / (ti.total_pixel_per_tile - 1.0));

		sr = sr * 2.0;
		sg = sg * 2.0;
		sb = sb * 2.0;

		if (sr > 255)
			sr = 255;
		if (sg > 255)
			sg = 255;
		if (sb > 255)
			sb = 255;

		stddev_int[i*RGB+R] = (int)round(sr);
		stddev_int[i*RGB+G] = (int)round(sg);
		stddev_int[i*RGB+B] = (int)round(sb);

		if (debug1) {
			printf("%i:%f %f %f  %Lf %Lf %Lf\n", i, colors[i*RGB+R], colors[i*RGB+G], colors[i*RGB+B], stddev[i*RGB+R], stddev[i*RGB+G], stddev[i*RGB+B]);
			printf("%i:%i %i %i  %i %i %i\n",    i, colors_int[i*RGB+R], colors_int[i*RGB+G], colors_int[i*RGB+B], stddev_int[i*RGB+R], stddev_int[i*RGB+G], stddev_int[i*RGB+B]);
		}
	}

	free(colors);
	free(stddev);

	thumbs_db_tiledims_file    = m_fopen(md.tiledims_filename, "rb");
	thumbs_db_tileoffsets_file = m_fopen(md.tileoffsets_filename, "rb");
	thumbs_db_imagecolors_file = m_fopen(md.imagecolors_filename, "rb");
	thumbs_db_imagestddev_file = m_fopen(md.imagestddev_filename, "rb");
	thumbs_db_invalid_file     = m_fopen(md.invalid_filename, "rb");
	thumbs_db_duplicates_file  = m_fopen(md.duplicates_filename, "rb");


	// SAVING PRIMARY TILE DIMENSIONS
	primarytiledims_file = m_fopen(mp.dest_primarytiledims_filename, "w");
	fprintf(primarytiledims_file, "%i	%i", ti.primary_tile_x_count, ti.primary_tile_y_count);
	m_fclose(primarytiledims_file);

	idx = 0;

	candidates_insert=0;
	candidates_pop=0;
	candidates_toobad=0;
	diff_best=FLT_MAX, diff_worst=0;

	uint32_t candidates_pop_uniqueness[SIZE_PRIMARY];
	uint32_t candidates_elect[SIZE_PRIMARY];
	memset(& candidates_pop_uniqueness,0, sizeof(candidates_pop_uniqueness));
	memset(&candidates_elect,0,sizeof(candidates_elect));

	int32_t old_percent = -1;
	time_t old_time = 0;

	while (1) {
		size_t len = fread(invalid_buf, 1, BUFSIZ / 2, thumbs_db_invalid_file);
		if (len == 0) {
			fprintf(stderr, "there is no data to read from invalid file\n");
			exit(EXIT_FAILURE);
		}
		len = fread(duplicates_buf, 1, BUFSIZ / 2, thumbs_db_duplicates_file);
		if (len == 0) {
			fprintf(stderr, "could not read from duplicates file\n");
			exit(EXIT_FAILURE);
		}
		len = fread(tile_dims_buf, 1, BUFSIZ, thumbs_db_tiledims_file);
		if (len == 0) {
			fprintf(stderr, "there is no data to read int tile_dims file\n");
			exit(EXIT_FAILURE);
		}
		len = fread(tileoffsets_buf, 1, BUFSIZ, thumbs_db_tileoffsets_file);
		if (len == 0) {
			fprintf(stderr, "there is no data to read tileoffsets file\n");
			exit(EXIT_FAILURE);
		}

		for (uint32_t i = 0, len1 = len - 1; i < len1; i += 2, idx++) {

			uint8_t thumbs_db_tile_x_count = tile_dims_buf[i];
			uint8_t thumbs_db_tile_y_count = tile_dims_buf[i + 1];
			uint8_t thumbs_db_tileoffset_x = tileoffsets_buf[i];
			uint8_t thumbs_db_tileoffset_y = tileoffsets_buf[i + 1];
			int thumbs_db_tileoffsets_unset = thumbs_db_tileoffset_x == 0xFF && thumbs_db_tileoffset_y == 0xFF;

			uint32_t thumbs_db_total_tile_count = thumbs_db_tile_x_count * thumbs_db_tile_y_count;

			if(args->quiet == 0) {

				float new_percent_f = idx / (element_count * 1.0);
				uint8_t new_percent = round(new_percent_f * 100);
				time_t new_time = time(NULL);

				if (new_percent != old_percent && new_time != old_time) {
					old_percent = new_percent;
					old_time = new_time;
					printf("%3i%%, %u/%u %s", old_percent, idx, element_count, ctime(&new_time));
				}
			}

			// ignore invalid entries and ignore duplicate entries if the image has unique option
			if (invalid_buf[i / 2] != 0 || ( duplicates_buf[i / 2] != 0)) { // valid entries must have 0
				if (debug)
					fprintf(stderr, "md entry (%u) is marked as invalid or duplicates, will be skipped\n", idx);

				// in case of invalid entries move the file pointers without reading the colors data
				m_fseeko(thumbs_db_imagecolors_file, thumbs_db_total_tile_count * RGB, SEEK_CUR);
				m_fseeko(thumbs_db_imagestddev_file, thumbs_db_total_tile_count * RGB, SEEK_CUR);

				continue;
			}

			// this could be slow, but works definitly
			// if an thumb is valid read its image data
			m_fread(colors_buf, thumbs_db_total_tile_count * RGB, thumbs_db_imagecolors_file);
			m_fread(stddev_buf, thumbs_db_total_tile_count * RGB, thumbs_db_imagestddev_file);

			uint8_t shift_x_len = thumbs_db_tile_x_count - database_image_resolution;
			uint8_t shift_y_len = thumbs_db_tile_y_count - database_image_resolution;

			if (shift_x_len != 0 && shift_y_len != 0) {
				fprintf(stderr, "one shift axis should be zero but: %i %i at thumb_idx: %u\n", shift_x_len, shift_y_len, idx
						//, thumbs_db_tile_x_count, thumbs_db_tile_y_count, database_image_resolution);
					);
				exit(EXIT_FAILURE);
			}

			unsigned char shift_y_0 = 0;
			unsigned char shift_x_0 = 0;
			if (!thumbs_db_tileoffsets_unset) {
				shift_y_0 = thumbs_db_tileoffset_y;
				shift_x_0 = thumbs_db_tileoffset_x;
				shift_y_len = shift_y_0 + 1;
				shift_x_len = shift_x_0 + 1;
			}

			if (debug1) {
				fprintf(stderr, "%u shift_len:%i %i\n", idx, shift_x_len, shift_y_len);
			}


			mosaik2_database_candidate mdc0, mdc_best_shift, mdc_improv, mdc_worst;
			memset(&mdc_best_shift, 0, sizeof(mdc_best_shift));
			memset(&mdc_improv, 0, sizeof(mdc_improv));
			memset(&mdc_worst, 0, sizeof(mdc_worst));

			mdc_best_shift.costs = FLT_MAX;
			mdc_worst.costs = 0;
			diff_improv = FLT_MIN;


			for (uint32_t primary_y = 0; primary_y < ti.primary_tile_y_count; primary_y++) {
				for (uint32_t primary_x = 0; primary_x < ti.primary_tile_x_count; primary_x++) {

					uint32_t primary_tile_idx = ti.primary_tile_x_count * primary_y + primary_x;

					memset(&mdc_best_shift, 0, sizeof(mdc_best_shift));
					memset(&mdc_worst, 0, sizeof(mdc_worst));
					mdc_best_shift.costs = FLT_MAX;
					mdc_worst.costs = 0;

					for (uint8_t shift_y = shift_y_0; shift_y <= shift_y_len; shift_y++) {
						for (uint8_t shift_x = shift_x_0; shift_x <= shift_x_len; shift_x++) {

							mdc0.candidate_idx = idx;
							mdc0.primary_tile_idx = primary_tile_idx;
							mdc0.costs = 0;
							mdc0.off_x = shift_x;
							mdc0.off_y = shift_y;



							int thumbs_total_tile_count = database_image_resolution * database_image_resolution;
							for (uint32_t k = 0; k < thumbs_total_tile_count; k++) {

								// how to find 64 (8x8 for example) tiles of the primary tile?
								//--------PRIMARY TILE 0---------------------------PRIMARY TILE 1-----------|
								// 0     1   2   3   4   5   6   7|   8    9   10   11   12   13   14   15|
								// 400 401 402 403 404 405 406 407| 408  409  410  411  412  413  414  415|  ...
								// 800          ...               |                                       |
								// 2800 2801                   2807|2808 2809                          2815|
								//--------PRIMARY TILE 50----------|---------------PRIMARY TILE 51--------  |
								// 3200                        3207|3208                               3215|
								// 3600                        3607|3608                               3615|  ...
								//                ...             |                   ...                 |
								// 6000                        6007|6008                               6015|
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

								uint32_t colors_idx = k % database_image_resolution + (k / database_image_resolution) * ti.primary_tile_x_count * database_image_resolution + (primary_tile_idx % ti.primary_tile_x_count) * database_image_resolution + (primary_tile_idx / ti.primary_tile_x_count) * ti.primary_tile_x_count * database_image_resolution * database_image_resolution;

								uint32_t j = k % database_image_resolution + (k / database_image_resolution) * thumbs_db_tile_x_count + shift_x + shift_y * thumbs_db_tile_x_count;

								if (color_distance == MOSAIK2_ARGS_COLOR_DISTANCE_MANHATTAN) {
									int diff_c_r = abs(colors_int[colors_idx*RGB+R] - (int)colors_buf[j * RGB + R]);
									int diff_c_g = abs(colors_int[colors_idx*RGB+G] - (int)colors_buf[j * RGB + G]);
									int diff_c_b = abs(colors_int[colors_idx*RGB+B] - (int)colors_buf[j * RGB + B]);

									int diff_s_r = abs(((int)stddev_int[colors_idx*RGB+R]) - (int)stddev_buf[j * RGB + R]);
									int diff_s_g = abs(((int)stddev_int[colors_idx*RGB+G]) - (int)stddev_buf[j * RGB + G]);
									int diff_s_b = abs(((int)stddev_int[colors_idx*RGB+B]) - (int)stddev_buf[j * RGB + B]);

									float diff0 = image_ratio * (diff_c_r + diff_c_g + diff_c_b) + stddev_ratio * ((diff_s_r + diff_s_g + diff_s_b) / 2.0);
									mdc0.costs += diff0 / ti.total_pixel_per_tile;

								} else if (color_distance == MOSAIK2_ARGS_COLOR_DISTANCE_EUCLIDIAN) {
									double diff_c = sqrt(
											  pow(colors_int[colors_idx*RGB+R] - (int)colors_buf[j * RGB + R], 2)
											+ pow(colors_int[colors_idx*RGB+G] - (int)colors_buf[j * RGB + G], 2)
											+ pow(colors_int[colors_idx*RGB+B] - (int)colors_buf[j * RGB + B], 2)
											);
									double diff_s = sqrt(
											  pow((int)stddev_int[colors_idx*RGB+R] - (int)stddev_buf[j * RGB + R], 2)
											+ pow((int)stddev_int[colors_idx*RGB+G] - (int)stddev_buf[j * RGB + G], 2)
											+ pow((int)stddev_int[colors_idx*RGB+B] - (int)stddev_buf[j * RGB + B], 2)
											);
									float diff0 = image_ratio * diff_c + stddev_ratio * diff_s;
									float diff1 = diff0 / ti.total_pixel_per_tile;
									mdc0.costs += diff1;

								} else if (color_distance == MOSAIK2_ARGS_COLOR_DISTANCE_CHEBYSHEV) {
									int diff_c_r = abs(colors_int[colors_idx*RGB+R] - (int)colors_buf[j * RGB + R]);
									int diff_c_g = abs(colors_int[colors_idx*RGB+G] - (int)colors_buf[j * RGB + G]);
									int diff_c_b = abs(colors_int[colors_idx*RGB+B] - (int)colors_buf[j * RGB + B]);
									int diff_c = diff_c_r;
									if (diff_c_g > diff_c)
										diff_c = diff_c_g;
									if (diff_c_b > diff_c)
										diff_c = diff_c_b;

									int diff_s_r = abs(((int)stddev_int[colors_idx*RGB+R]) - (int)stddev_buf[j * RGB + R]);
									int diff_s_g = abs(((int)stddev_int[colors_idx*RGB+G]) - (int)stddev_buf[j * RGB + G]);
									int diff_s_b = abs(((int)stddev_int[colors_idx*RGB+B]) - (int)stddev_buf[j * RGB * B]);
									int diff_s = diff_s_r;
									if (diff_s_g > diff_s)
										diff_s = diff_s_g;
									if (diff_s_b > diff_s)
										diff_s = diff_s_b;

									float diff0 = image_ratio * diff_c + stddev_ratio * diff_s;
									mdc0.costs += diff0 / ti.total_pixel_per_tile;

								} else {
									fprintf(stderr, "invalid color distance\n");
									exit(EXIT_FAILURE);
								}
							} // thumbs_total_tile_count end

							// only the best shift will be saved for this candidate in this primary tile
							// unique or none unique
							if( mdc0.costs < mdc_best_shift.costs) {
								memcpy(&mdc_best_shift, &mdc0, sizeof(mdc_best_shift));
							}
							if( mdc0.costs > mdc_worst.costs ) {
								memcpy(&mdc_worst, &mdc0, sizeof(mdc_worst));
							}
						}     // for shift_x
					}       // for shift_y


					if( mdc_worst.costs - mdc_best_shift.costs > diff_worst ) {
						diff_worst = mdc_worst.costs - mdc_best_shift.costs;

					}
					if( mdc_worst.costs - mdc_best_shift.costs < diff_best  &&( shift_x_0 < shift_x_len-1 || shift_y_0 < shift_y_len-1 )) { // only if theres more than one shift.. .
						diff_best = mdc_worst.costs - mdc_best_shift.costs;
					}


					if(fast_unique) {
						mosaik2_database_candidate mdc_old_heap;
						//memset(&mdc_old_heap,0,sizeof(mdc_old_heap));
						int empty = max_heap_peek( &heap[primary_tile_idx], &mdc_old_heap );
						if(empty)
							mdc_old_heap.costs = FLT_MAX;
						if(mdc_old_heap.costs - mdc_best_shift.costs >  diff_improv) {
							//fprintf(stderr, "better found %i:%i => %f - %f > %f\n", mdc_old_heap.primary_tile_idx, mdc_best_shift.primary_tile_idx, mdc_old_heap.costs, mdc_best_shift.costs,  diff_improv);
							diff_improv = mdc_old_heap.costs - mdc_best_shift.costs ;
							memcpy(&mdc_improv, &mdc_best_shift, sizeof(mdc_improv));
						}
						/*fprintf(stderr,"idx:%4i ptidx:%4i  best_shift ptidx:%4i costs:%7.2f  maxheap ptidx:%4i costs:%7.2f  improv ptidx:%4i costs:%7.2f  diff_improv:%7.2f  %7.2f\n",
								idx,
								primary_tile_idx,
								mdc_best_shift.primary_tile_idx,
								mdc_best_shift.costs,
								mdc_old_heap.primary_tile_idx,
								mdc_old_heap.costs,
								mdc_improv.primary_tile_idx,
								mdc_improv.costs,
								mdc_old_heap.costs - mdc_best_shift.costs,
								diff_improv);*/

					} else /* !fast_unique*/ {
						// THIS is the point where the calculated color difference
						// is compared to the already stored ones.
						// If its lower, it is more equal and it should be the new candidate
						// if(diff_color0 < candidates_costs[primary_tile_idx] )
						// worst costs is saved in the last position, and this position
						// will expand from 0 to ti.total_primary_tile_count
						mosaik2_database_candidate max_heap;

						int empty = max_heap_peek( &heap[primary_tile_idx], &max_heap );

						if(heap[primary_tile_idx].last < max_candidates_len || max_heap.costs > mdc_best_shift.costs || empty == 1) {
							if( heap[primary_tile_idx].count >= max_candidates_len ) {
								mosaik2_database_candidate c;
								max_heap_pop( &heap[primary_tile_idx], &c);
								candidates_pop++;
							}
							candidates_insert++;
							max_heap_insert(&heap[primary_tile_idx], &mdc_best_shift);
						}
						else {
							candidates_toobad++;
						}
					}
				}// for primary_x
			}// for primary_y

			if(fast_unique) {
				mosaik2_database_candidate max_heap;
				int empty = max_heap_peek( &heap[mdc_improv.primary_tile_idx], &max_heap );
				if( !empty ) {
					if(max_heap.costs > mdc_improv.costs) {
						if(debug)fprintf(stderr,"insert OK- idx:%i pti:%i costs:%f\n",idx, mdc_improv.primary_tile_idx, mdc_improv.costs);
						//remove old candidate
						max_heap_pop(  &heap[ mdc_improv.primary_tile_idx ], NULL );
						max_heap_insert( &heap[ mdc_improv.primary_tile_idx ], &mdc_improv );
					} else {
						if(debug)fprintf(stderr,"insert NO  idx:%i pti:%i costs:%f\n",idx, mdc_improv.primary_tile_idx, mdc_improv.costs);
					}
				} else {
					if(debug)fprintf(stderr,"insert OK+ idx:%i pti:%i costs:%f\n",idx, mdc_improv.primary_tile_idx, mdc_improv.costs);
					max_heap_insert( &heap[ mdc_improv.primary_tile_idx ], &mdc_improv );
				}
			}

		}// for idx

		if (len < BUFSIZ) {
			break;
		}
	}

	m_fclose(thumbs_db_tiledims_file);
	m_fclose(thumbs_db_tileoffsets_file);
	m_fclose(thumbs_db_imagecolors_file);
	m_fclose(thumbs_db_imagestddev_file);
	m_fclose(thumbs_db_invalid_file);
	m_fclose(thumbs_db_duplicates_file);

	free(colors_int);
	free(stddev_int);

	//transform the max_heap to an ordered array
	mosaik2_database_candidate mdc_tmp[max_candidates_len];
	for(uint32_t i=0;i<SIZE_PRIMARY;i++) {
		for(int32_t j=max_candidates_len-1;j>=0;j--) {
			max_heap_pop(&heap[i], &mdc_tmp[j]);
		}
		// with mdc it uses the old max heap memory
		memcpy(&mdc[max_candidates_len*i], &mdc_tmp, sizeof(mdc_tmp));
	}

	/*
	 * search the best unique candidate. this is not a very sophisticated algorithm, but is has not very much overhead and is not a huge bottleneck in the gathering program.
	 * candidates are checked with each other if there are duplicates. If there are the duplicate with the higher cost is popped, so the next candidate at its position will be compared also to all others. This is repeated until there is no duplicate any more.
	 */

	if (unique == 1) {
		int unique_run;
		do {
			unique_run = 1;
			for (uint32_t i = 0; i < SIZE_PRIMARY - 1; i++) {
				for (uint32_t j = 0; j < SIZE_PRIMARY; j++) { // starting at 0 is attended. 
					if (i == j)
						continue;
					uint32_t offset_i = i * max_candidates_len + candidates_elect[i];
					uint32_t offset_j = j * max_candidates_len + candidates_elect[j];

					if (mdc[offset_i].candidate_idx == mdc[offset_j].candidate_idx) {
						if (mdc[offset_i].costs >= mdc[offset_j].costs) {
							candidates_elect[i]++;
							candidates_pop_uniqueness[i]++;
						} else {
							candidates_elect[j]++;
							candidates_pop_uniqueness[j]++;
						}

						// if there were more candidates compared as possible
						assert( candidates_elect[i] != ti.total_primary_tile_count);
						unique_run = 0;
					}
				}
			}
			// if a do while round has any replacement => it is not unique yet
			// if a do while round has any replacement => it is not unique yet
		} while (unique_run == 0);
	}

	// intialize with -1
	mosaik2_result = m_fopen(mp.dest_result_filename, "wb");
	candidate_best_costs = FLT_MAX;
	candidate_worst_costs=0;
	for (uint32_t i = 0; i < SIZE_PRIMARY; i++) {
		uint32_t offset = i * max_candidates_len + candidates_elect[i];
		if(mdc[offset].costs<candidate_best_costs)
			candidate_best_costs = mdc[offset].costs;
		if(mdc[offset].costs>candidate_worst_costs)
			candidate_worst_costs = mdc[offset].costs;
		fprintf(mosaik2_result, "%i	%i	%f	%i	%i\n", i, mdc[offset].candidate_idx, mdc[offset].costs, mdc[offset].off_x, mdc[offset].off_y);
	}
	m_fclose(mosaik2_result);

	if(args->quiet != 1) {
		printf("candidate heap list insert:%i pop:%i toobad:%i\n", candidates_insert, candidates_pop, candidates_toobad);
		printf("best/worst candidate costs:%f, %f\n", candidate_best_costs, candidate_worst_costs);
		printf("shifted candidates best/worst difference between shifts:%f, %f\n", diff_best, diff_worst);
	}
	if(args->quiet != 1 && unique) {
		uint64_t sum_elect = 0u;
		uint32_t max_elect = 0u;
		for (int i = 0; i < ti.total_primary_tile_count; i++) {
			sum_elect += candidates_pop_uniqueness[i];
			if( max_elect < candidates_pop_uniqueness[i])
				max_elect = candidates_pop_uniqueness[i];
		}
		printf("create uniqueness pop:%li max primary tile value pop:%i possible maximum:%li\n", sum_elect, max_elect, max_candidates_len);
	}


	free(mdc);
	return 0;
}

