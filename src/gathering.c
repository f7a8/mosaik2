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

float manhattan_distance(float *a, float *b, uint32_t size);
float euclidean_distance(float *a, float *b, uint32_t size);
float chebyshev_distance(float *a, float *b, uint32_t size);

						 
double sqrt2(double x);
float sqrtf2(float x);

int mosaik2_gathering(mosaik2_arguments *args) {
	m2rezo src_image_resolution = args->src_image_resolution;
	//m2name dest_filename = args->dest_image;
	int unique = args->unique;
	int fast_unique = args->fast_unique;
	m2name mosaik2_database_name = args->mosaik2db;
	const uint8_t debug = 0;
	const uint8_t debug1 = 0;
	const uint8_t html = 0;
	const uint8_t out = 0;
	int color_distance = args->color_distance;
	m2rezo database_image_resolution;
	m2elem element_count;
	uint32_t SIZE_PRIMARY; // alias for  ti.total_primary_tile_count
	uint32_t valid_md_element_count;
	uint32_t needed_md_element_count;
	size_t max_candidates_len;
	size_t total_candidates_count;
	float *colors;
	int *colors_int;

	

	uint8_t *exclude; //bitmap over all primary tiles, 1 means exclude,0 include


	m2file thumbs_db_tiledims_file;
	m2file thumbs_db_tileoffsets_file;
	m2file thumbs_db_imagecolors_file;
	m2file thumbs_db_invalid_file;
	m2file thumbs_db_duplicates_file;
	uint8_t tile_dims_buf[BUFSIZ];
	unsigned char tileoffsets_buf[BUFSIZ];
	unsigned char colors_buf[256*256*RGB]; //maximal possible data amout per database candidate
	unsigned char invalid_buf[BUFSIZ];
	unsigned char duplicates_buf[BUFSIZ];
	//m2file primarytiledims_file;
	uint32_t idx = 0;
	uint32_t candidates_insert=0;
	uint32_t candidates_pop=0;
	uint32_t candidates_toobad=0;
	float diff_best=FLT_MAX,diff_improv=FLT_MIN,diff_worst=0;

	m2file mosaik2_dest_result_file;
	m2file mosaik2_dest_tile_infos_file;
	float candidate_best_costs = FLT_MAX, candidate_worst_costs=0;



	mosaik2_database md;
	mosaik2_database_init(&md, mosaik2_database_name);
	mosaik2_database_check(&md);
	mosaik2_database_lock_reader(&md);
	mosaik2_database_read_database_id(&md);
	mosaik2_project_check_dest_filename(args->dest_image);
	database_image_resolution = mosaik2_database_read_image_resolution(&md);
	element_count = mosaik2_database_read_element_count(&md);


	float /*__attribute__((aligned(32)))*/ colors_sorted_src[RGB*database_image_resolution*database_image_resolution];
	float /*__attribute__((aligned(32)))*/ colors_sorted_db[ RGB*database_image_resolution*database_image_resolution];

	float /*__attribute__((aligned(32)))*/ colors_buf_f[256*256*RGB];


	if (database_image_resolution * database_image_resolution * (2 * RGB * UINT8_MAX) > UINT32_MAX) {
		// can candidates_costs contain the badest possible costs?
		fprintf(stderr, "thumb tile size too high for internal data structure\n");
		exit(EXIT_FAILURE);
	}

	if (unique < 0 || unique > 1) {
		fprintf(stderr, "unique is only allowed to be 0 (one image may be used mutiple times) or 1 (one image is just used once)\n");
		exit(EXIT_FAILURE);
	}

	mosaik2_project mp = {.unique = unique, .fast_unique = fast_unique, .primary_tile_count = src_image_resolution};
	mosaik2_project_init(&mp, md.id, args->dest_image/*, args->src_image*/);
	//strncpy(mp.src_filename, args->src_image, MAX_FILENAME_LEN);
	concat(mp.src_filename, args->src_image);



	if (debug)
		printf("src_image_resolution:%i,database_image_resolution:%i\n", src_image_resolution, database_image_resolution);
	gdImagePtr im = read_image_from_file(mp.src_filename);

	mosaik2_tile_infos ti = {0};
	mosaik2_tile_infos_init(&ti, database_image_resolution, args->src_image_resolution, (uint32_t)gdImageSX(im), (uint32_t)gdImageSY(im));

	if (ti.image_width < ti.tile_count || ti.image_height < ti.tile_count) {
		fprintf(stderr, "image too small\n");
		exit(EXIT_FAILURE);
	}
	mosaik2_project_read_exclude_area(&mp, &ti, args);

	if (debug)
		fprintf(stdout, "ti.pixel_per_tile=ti.short_dim / ti.tile_count => %i = %i / %i\n", ti.pixel_per_tile, ti.short_dim, ti.tile_count);

	SIZE_PRIMARY = ti.total_primary_tile_count;

	if (debug)
		printf("image_dims:%i %i, primary_tile_dims:%i %i(%i), tile_dims:%i %i, l:%i %i, off:%i %i pixel_per:%i %i\n", ti.image_width, ti.image_height, ti.primary_tile_x_count, ti.primary_tile_y_count, ti.total_primary_tile_count, ti.tile_x_count, ti.tile_y_count, ti.lx, ti.ly, ti.offset_x, ti.offset_y, ti.pixel_per_primary_tile, ti.pixel_per_tile);

	valid_md_element_count = mosaik2_database_read_valid_count(&md);
	needed_md_element_count = unique || fast_unique? ti.total_primary_tile_count - mp.exclude_count : 1;
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

	colors       = m_calloc(ti.total_tile_count * RGB, sizeof(float));
	colors_int   = m_calloc(ti.total_tile_count * RGB, sizeof(int));
	//colors_int_f = m_calloc(ti.total_tile_count * RGB, sizeof(int));
	exclude	     = m_calloc(ti.total_primary_tile_count, sizeof(uint8_t));

	for(m2elem a=0;a<mp.exclude_count;a++) {
		for(m2elem tile_x=mp.exclude_area[a].start_x;tile_x<=mp.exclude_area[a].end_x;tile_x++) {
			for(m2elem tile_y=mp.exclude_area[a].start_y;tile_y<=mp.exclude_area[a].end_y;tile_y++) {
				m2elem tile_idx = tile_y * ti.primary_tile_x_count + tile_x;
				exclude[tile_idx]=1;
				//fprintf(stderr, "exclude area:%i, tile_id:%u\n", a, tile_idx);
			}
		}
	}
	m_free((void**)&mp.exclude_area);
	for (uint32_t j = 0, j1 = ti.offset_y; j1 < ti.ly; j++, j1++) {
		for (uint32_t i = 0, i1 = ti.offset_x; i1 < ti.lx; i++, i1++) {
			// i runs from 0 to length of cropped area
			// j also
			// i1 and j1 are corrected by the offset

			uint32_t tile_x = i / ti.pixel_per_tile;
			uint32_t tile_y = j / ti.pixel_per_tile;
			uint32_t tile_idx = tile_y * ti.tile_x_count + tile_x;
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

	if (debug)
		printf("dim:%i %i,off:%i %i,l:%i %i,primary_tile_cout:%i %i,ti.tile_count:%i %i,ti.pixel_per_tile:%i %f\n", ti.image_width, ti.image_height, ti.offset_x, ti.offset_y, ti.lx, ti.ly, ti.primary_tile_x_count, ti.primary_tile_y_count, ti.tile_x_count, ti.tile_y_count, ti.pixel_per_tile, ti.total_pixel_per_tile);
	if (html)
		printf("<html><head><style>table{ width:851px;height:566px; border-collapse: collapse;}td{padding:0;height:0.2em;width:0.2em;}</style></head><body><table>");

	if (out) {

		for (uint32_t y = 0; y < ti.tile_y_count; y++) {
			if (html)
				printf("<tr>");

			for (uint32_t x = 0; x < ti.tile_x_count; x++) {
				uint32_t tile_idx = y * ti.tile_x_count + x;
				int red =   (int)round(colors[tile_idx*RGB+R]);
				int green = (int)round(colors[tile_idx*RGB+G]);
				int blue =  (int)round(colors[tile_idx*RGB+B]);

				if (debug1)
					printf("avg_red:%i %f", red, colors[tile_idx*RGB+R]);
				if (debug1)
					printf("avg_green:%i %f", green, colors[tile_idx*RGB+G]);
				if (debug1)
					printf("avg_blue:%i %f", blue, colors[tile_idx*RGB+B]);

				if (html)
					printf("<td style='background-color:#%02x%02x%02x'>", red, green, blue);
				if (debug)
					printf("%i:%i %i %i\n", tile_idx, red, green, blue);
				if (out)
					printf(" %02X%02X%02X", red, green, blue);
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


	}

	

	thumbs_db_tiledims_file    = m_fopen(md.tiledims_filename, "rb");
	thumbs_db_tileoffsets_file = m_fopen(md.tileoffsets_filename, "rb");
	thumbs_db_imagecolors_file = m_fopen(md.imagecolors_filename, "rb");
	thumbs_db_invalid_file     = m_fopen(md.invalid_filename, "rb");
	thumbs_db_duplicates_file  = m_fopen(md.duplicates_filename, "rb");


	/*primarytiledims_file = m_fopen(mp.dest_primarytiledims_filename, "w");
	fprintf(primarytiledims_file, "%i	%i	%s", ti.primary_tile_x_count, ti.primary_tile_y_count, "TODOfilename");
	m_fclose(primarytiledims_file);*/

	// SAVING PRIMARY TILE DIMENSIONS
	mosaik2_dest_tile_infos_file = m_fopen(mp.dest_tile_infos_filename, "wb");
	//void m_fwrite(const void *ptr, size_t nmemb, m2file stream) {
	m_fwrite(&ti, sizeof(ti), mosaik2_dest_tile_infos_file);
	m_fclose(mosaik2_dest_tile_infos_file);



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

				continue;
			}

			// this could be slow, but works definitly
			// if an thumb is valid read its image data
				m_fread(colors_buf, thumbs_db_total_tile_count * RGB, thumbs_db_imagecolors_file);
				// using float32 instead of unsigned char allows us to use memcpy later on
				u8_to_f32(colors_buf,colors_buf_f,thumbs_db_total_tile_count * RGB);


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

					if(exclude[primary_tile_idx] == 1)
						continue;

					memset(&mdc_best_shift, 0, sizeof(mdc_best_shift));
					memset(&mdc_worst, 0, sizeof(mdc_worst));
					mdc_best_shift.costs = FLT_MAX;
					mdc_worst.costs = 0;

					uint32_t thumbs_total_tile_count = database_image_resolution * database_image_resolution;
					
					

					// crop colors from src image to one matrix, faster for comparison
					for(uint32_t k=0;k<thumbs_total_tile_count;k++) {

						uint32_t _idx = k % database_image_resolution 
								+ (k / database_image_resolution) * ti.primary_tile_x_count * database_image_resolution 
								+ (primary_tile_idx % ti.primary_tile_x_count) * database_image_resolution 
								+ (primary_tile_idx / ti.primary_tile_x_count) * ti.primary_tile_x_count * database_image_resolution * database_image_resolution;

							memcpy(colors_sorted_src+k*RGB, colors+_idx*RGB, RGB*sizeof(float));
						
					}

					for (uint8_t shift_y = shift_y_0; shift_y <= shift_y_len; shift_y++) {
						for (uint8_t shift_x = shift_x_0; shift_x <= shift_x_len; shift_x++) {

							mdc0.candidate_idx = idx;
							mdc0.primary_tile_idx = primary_tile_idx;
							mdc0.costs = 0;
							mdc0.off_x = shift_x;
							mdc0.off_y = shift_y;



							
			

							// for (uint32_t k = 0; k < thumbs_total_tile_count; k+=vector_size) {

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


							// crop colors from db candidate to one matrix, faster for comparison
							for(uint32_t k=0;k<thumbs_total_tile_count;k++) {

								uint32_t _idx = k % database_image_resolution 
										+ (k / database_image_resolution) * thumbs_db_tile_x_count 
										+ shift_x 
										+ shift_y * thumbs_db_tile_x_count;
										
								//j[k] = _idx;
								memcpy(colors_sorted_db+k*RGB, colors_buf_f+_idx*RGB, RGB*sizeof(float));
							}


/*							uint32_t colors_idx = k % database_image_resolution 
											+ (k / database_image_resolution) * ti.primary_tile_x_count * database_image_resolution 
											+ (primary_tile_idx % ti.primary_tile_x_count) * database_image_resolution 
											+ (primary_tile_idx / ti.primary_tile_x_count) * ti.primary_tile_x_count * database_image_resolution * database_image_resolution;

							uint32_t j = k % database_image_resolution + (k / database_image_resolution) * thumbs_db_tile_x_count + shift_x + shift_y * thumbs_db_tile_x_count;
*/
							if (color_distance == MOSAIK2_ARGS_COLOR_DISTANCE_MANHATTAN) {
								mdc0.costs += manhattan_distance(colors_sorted_src, colors_sorted_db,thumbs_total_tile_count);
							} else if (color_distance == MOSAIK2_ARGS_COLOR_DISTANCE_EUCLIDIAN) {
								mdc0.costs += euclidean_distance(colors_sorted_src, colors_sorted_db,thumbs_total_tile_count);
							} else if (color_distance == MOSAIK2_ARGS_COLOR_DISTANCE_CHEBYSHEV) {
								mdc0.costs += chebyshev_distance(colors_sorted_src, colors_sorted_db,thumbs_total_tile_count);
							} else {
								fprintf(stderr, "invalid color distance\n");
								exit(EXIT_FAILURE);
							}

							// only the best shift will be saved for this candidate in this primary tile
							// unique or none unique
							if( mdc0.costs < mdc_best_shift.costs) {
								/*if(primary_tile_idx==0)
								fprintf(stderr, "NEW BEST: idx: %4i primary_tile_idx:%i shift:%2i %2i costs:%f %f\n", mdc0.candidate_idx, mdc0.primary_tile_idx,mdc0.off_x,mdc0.off_y,mdc0.costs,mdc_best_shift.costs);*/
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

			/* the concept of fast unique has two steps. as long as not every 
			tile has one candidate put those candidates on empty tiles where 
			they fit best. If every tile was set, put only tiles on those positions
			where they bring the best improvments */
			if(fast_unique) {
				mosaik2_database_candidate max_heap={0};

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
	m_fclose(thumbs_db_invalid_file);
	m_fclose(thumbs_db_duplicates_file);

	m_free((void**)&colors_int);
	//transform the max_heap to an ordered array
	mosaik2_database_candidate mdc_tmp[max_candidates_len];
	for(uint32_t i=0;i<SIZE_PRIMARY;i++) {
		if(exclude[i] == 1)
			continue;
		for(int32_t j=max_candidates_len-1;j>=0;j--) {
			max_heap_pop(&heap[i], &mdc_tmp[j]);
		}
		// with mdc it uses the old max heap memory
		memcpy(&mdc[max_candidates_len*i], &mdc_tmp, sizeof(mdc_tmp));
	}

	/*
	 * search the best unique candidate. this is not a very sophisticated
	 * algorithm, but is has not very much overhead and is not a huge bottleneck
	 * in the gathering program. Candidates are checked with each other if there
	 * are duplicates. If there are the duplicate with the higher cost is popped,
	 * so the next candidate at its position will be compared also to all others.
	 * This is repeated until there is no duplicate any more.
	 */
	if (unique == 1) {
		int unique_run;
		do {
			unique_run = 1;
			for (uint32_t i = 0; i < SIZE_PRIMARY - 1; i++) {
				for (uint32_t j = 0; j < SIZE_PRIMARY; j++) { // starting at 0 is attended. 
					if (i == j || exclude[i] || exclude[j])
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

	m_free((void**)&colors);


	// intialize with -1
	mosaik2_dest_result_file = m_fopen(mp.dest_result_filename, "wb");
	candidate_best_costs = FLT_MAX;
	candidate_worst_costs=0;
	for (uint32_t i = 0; i < SIZE_PRIMARY; i++) {
			uint32_t offset = i * max_candidates_len + candidates_elect[i];
			if(mdc[offset].costs<candidate_best_costs)
				candidate_best_costs = mdc[offset].costs;
			if(mdc[offset].costs>candidate_worst_costs)
				candidate_worst_costs = mdc[offset].costs;
			fprintf(mosaik2_dest_result_file, "%i	%i	%f	%i	%i	%i\n", i, mdc[offset].candidate_idx, mdc[offset].costs, mdc[offset].off_x, mdc[offset].off_y, exclude[i]);
	}
	m_fclose(mosaik2_dest_result_file);
	m_free((void**)&exclude);


	if(args->quiet != 1) {
		printf("candidate heap list insert:%i pop:%i toobad:%i\n", candidates_insert, candidates_pop, candidates_toobad);
		printf("best/worst candidate costs:%f, %f\n", candidate_best_costs, candidate_worst_costs);
		printf("shifted candidates best/worst difference between shifts:%f, %f\n", diff_best, diff_worst);
	}
	if(args->quiet != 1 && unique) {
		uint64_t sum_elect = 0u;
		uint32_t max_elect = 0u;
		for (uint32_t i = 0; i < ti.total_primary_tile_count; i++) {
			sum_elect += candidates_pop_uniqueness[i];
			if( max_elect < candidates_pop_uniqueness[i])
				max_elect = candidates_pop_uniqueness[i];
		}
		printf("create uniqueness pop:%li max primary tile value pop:%i possible maximum:%li\n", sum_elect, max_elect, max_candidates_len);
	}


	m_free((void**)&mdc);

	return 0;
}









float sum(const float* data, size_t size) {
#ifdef HAVE_AVX
    __m256 sum = _mm256_setzero_ps();  

    size_t i = 0;
    for (; i + 7 < size; i += 8) {
        __m256 chunk = _mm256_loadu_ps(&data[i]);  
        sum = _mm256_add_ps(sum, chunk);
    }

    
    float temp[8];
    _mm256_storeu_ps(temp, sum);
    float total = temp[0] + temp[1] + temp[2] + temp[3] +
                  temp[4] + temp[5] + temp[6] + temp[7];

    
    for (; i < size; ++i) {
        total += data[i];
    }

    return total;

#else
    float _sum = .0f;

    size_t i = 0;
    for (; i<size; i++) {
		_sum += data[i];
    }
    return _sum;
#endif
}


double sqrt2(double x) {
	#ifdef HAVE_AVX
    __m256d vec = _mm256_set1_pd(x); // Lade x in alle 4 Elemente des Vektors
    __m256d result = _mm256_sqrt_pd(vec); // Berechne die Quadratwurzel
    double output[4];
    _mm256_storeu_pd(output, result);
    return output[0]; // Gibt den ersten Wert zurück, da er für alle gleich ist
	#else
	return sqrt(x);
	#endif
}

float sqrtf2(float x) {
	#ifdef HAVE_AVX
		__m256 vec = _mm256_set1_ps(x); // Lade x in alle 4 Elemente des Vektors
		__m256 result = _mm256_sqrt_ps(vec); // Berechne die Quadratwurzel
		float output[8];
		_mm256_storeu_ps(output, result);
		return output[0]; // Gibt den ersten Wert zurück, da er für alle gleich ist
	#else
		return sqrtf(x);
	#endif
}

float manhattan_distance(float *a, float *b, uint32_t size) {

	const uint32_t len = size * RGB; 

	#ifdef HAVE_AVX

	__m256 va, vb, vdiff, vabs;
    __m256 vsum = _mm256_setzero_ps();
	const __m256 abs_bitmask = _mm256_set1_ps(-0.0f);

    uint32_t i = 0;
    for (; i + 7 < len; i += 8) {
        va = _mm256_loadu_ps(a + i);
        vb = _mm256_loadu_ps(b + i);
        vdiff = _mm256_sub_ps(va, vb);                   // float diff
        vabs  = _mm256_andnot_ps(abs_bitmask, vdiff);  // abs via bitmask

        vsum = _mm256_add_ps(vsum, vabs);                // accumulate
    }

    float temp[8];
    _mm256_storeu_ps(temp, vsum);
    float costs = temp[0] + temp[1] + temp[2] + temp[3] +
                 temp[4] + temp[5] + temp[6] + temp[7];

    // Rest
    for (; i < len; ++i) {
        costs += abs((int)(a[i] - b[i]));
    }
#else
	int costs=0;
	for(uint32_t i=0;i<len;i++) {
		costs += abs( (int)(a[i] - b[i]) );
	}
	
#endif

	return costs / size;
}

float chebyshev_distance(float *a, float *b, uint32_t size) {
	
	int costs = 0;

	const uint32_t len = size*RGB;
	for(uint32_t i=0;i<len;i+=RGB) {

		int diff_r = abs( (int)(a[i+R] - b[i+R]));
		int diff_g = abs( (int)(a[i+G] - b[i+G]));
		int diff_b = abs( (int)(a[i+B] - b[i+B]));
		
		int diff = diff_r;
		if (diff_g > diff)
			diff= diff_g;
		if (diff_b > diff)
			diff= diff_b;

		costs += diff;
	}

	return costs / size;
}

/*
  a src_image, b candidate image, size elements in src_image and candidate_images. must be the same.
  in a_idx and b_idx are 
*/
float euclidean_distance(float *a, float *b, uint32_t size) {

    float costs=.0f;

#ifdef HAVE_AVX

	uint32_t i=0;

	float v0[8] __attribute__((aligned(32)));
	float v1[8] __attribute__((aligned(32)));
	float v2[8] __attribute__((aligned(32)));
	
	for(;i+23<size*RGB;i+=24) {

		__m256 _a0 = _mm256_load_ps(a+i);
		__m256 _a1 = _mm256_load_ps(a+i+8);
		__m256 _a2 = _mm256_load_ps(a+i+16);

		__m256 _b0 = _mm256_load_ps(b+i);
		__m256 _b1 = _mm256_load_ps(b+i+8);
		__m256 _b2 = _mm256_load_ps(b+i+16);
		
		_a0 = _mm256_sub_ps(_a0,_b0);
		_a1 = _mm256_sub_ps(_a1,_b1);
		_a2 = _mm256_sub_ps(_a2,_b2);

		_a0 = _mm256_mul_ps(_a0, _a0);
		_a1 = _mm256_mul_ps(_a1, _a1);
		_a2 = _mm256_mul_ps(_a2, _a2);

		_mm256_storeu_ps(v0, _a0);
		_mm256_storeu_ps(v1, _a1);
		_mm256_storeu_ps(v2, _a2);

		float vv[] = {
					   v0[0]+v0[1]+v0[2],
					   v0[3]+v0[4]+v0[5],
					   v0[6]+v0[7]+v1[0],
					   v1[1]+v1[2]+v1[3],
					   v1[4]+v1[5]+v1[6],
					   v1[7]+v2[0]+v2[1],
					   v2[2]+v2[3]+v2[4],
					   v2[5]+v2[6]+v2[7]
		 			};
		_a0 = _mm256_loadu_ps(vv);
		_a0 = _mm256_sqrt_ps(_a0);
		_a0 = _mm256_hadd_ps(_a0, _a0); 
		_a0 = _mm256_hadd_ps(_a0, _a0); 
		_a0 = _mm256_hadd_ps(_a0, _a0); 
		_mm256_storeu_ps(v0, _a0);

		costs += v0[0];
	}
	//calculate the rest of the loop passages euclidean
	for(;i<size;i++) {
		float diff = sqrtf2(
				pow(*(a+i*RGB+R)-*(b+i*RGB+R), 2)
			  + pow(*(a+i*RGB+G)-*(b+i*RGB+G), 2)
			  + pow(*(a+i*RGB+B)-*(b+i*RGB+B), 2)
		);

		costs += diff;
	}

	return costs / size;

#else

	for( uint32_t i=0;i<size*RGB;i++) {
		costs += sqrtf2(
				  pow( *(a+i+R) - *(b+i+R), 2)
				+ pow( *(a+i+G) - *(b+i+G), 2)
				+ pow( *(a+i+B) - *(b+i+B), 2));
	}
	return costs / size ;
#endif
}