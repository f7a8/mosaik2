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
  mosaik2_database_read_database_id(&md);

  check_thumbs_db(&md);

  check_dest_filename(dest_filename);
  uint8_t thumbs_tile_count = read_thumbs_conf_tilecount(&md);
  uint64_t thumbs_count = read_thumbs_db_count(&md);

  if (thumbs_tile_count * thumbs_tile_count * (6 * 256) > UINT32_MAX) {
    // can candidates_costs contain the worst possible costs?
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

  float image_ratio = ratio / 100;
  float stddev_ratio = 1.0 - image_ratio;

  int color_distance = args->color_distance;

  mosaik2_project mp = {.ratio = ratio, .unique = unique, .primary_tile_count = primary_tile_count};

  init_mosaik2_project(&mp, md.id, dest_filename);

  //	printf("analyze master image\n");

  uint8_t debug = 0;
  uint8_t debug1 = 0;
  const uint8_t html = 0;
  const uint8_t out = 0;
  // const uint8_t duplicates_allowed = 0;

  if (debug)
    printf("primary_tile_count:%i,thumbs_tile_count:%i\n", primary_tile_count, thumbs_tile_count);

  unsigned char *buffer = read_stdin(&mp.file_size);
  m_fclose(stdin);

  gdImagePtr im;
  int ft = get_file_type_from_buf(buffer, mp.file_size);

  if (ft == FT_JPEG)
    im = gdImageCreateFromJpegPtrEx(mp.file_size, buffer, 0);
  else if (ft == FT_PNG)
    im = gdImageCreateFromPngPtr(mp.file_size, buffer);
  else {
    free(buffer);
    fprintf(stderr, "image could not be instanciated\n");
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
  if (width < tile_count || height < tile_count) {
    fprintf(stderr, "image too small\n");
    exit(EXIT_FAILURE);
  }

  uint32_t short_dim; //, long_dim;
  if (width < height) {
    short_dim = width;
    // long_dim = height;
  } else {
    short_dim = height;
    // long_dim = width;
  }

  //       6                    = ( 4000      - (4000      % 640       ) ) / 640       ;
  //       6                    =   3480 / 640
  uint32_t pixel_per_tile = short_dim / tile_count; // automatically floored
  if (debug)
    fprintf(stdout, "pixel_per_tile=short_dim / tile_count => %i = %i / %i\n", pixel_per_tile, short_dim, tile_count);
  //     36                   = 6              * 6             ;
  double total_pixel_per_tile = pixel_per_tile * pixel_per_tile;

  //                          96 = 6 * 16
  uint32_t pixel_per_primary_tile = pixel_per_tile * thumbs_tile_count;
  //                            9216 = 96 * 96
  // double total_pixel_per_primary_tile = pixel_per_primary_tile * pixel_per_primary_tile;

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
  total_primary_tile_count = (tile_x_count / thumbs_tile_count) * (tile_y_count / thumbs_tile_count);
  const uint32_t SIZE_PRIMARY = total_primary_tile_count;

  lx = offset_x + pixel_per_tile * tile_x_count;
  ly = offset_y + pixel_per_tile * tile_y_count;

  if (debug)
    printf("image_dims:%i %i, primary_tile_dims:%i %i(%i), tile_dims:%i %i, l:%i %i, off:%i %i pixel_per:%i %i\n", width, height, primary_tile_x_count, primary_tile_y_count, total_primary_tile_count, tile_x_count, tile_y_count, lx, ly, offset_x, offset_y, pixel_per_primary_tile, pixel_per_tile);

  if (unique == 1 && total_primary_tile_count > thumbs_count) {
    fprintf(stderr, "there are too few candidates (%lu) for unique than needed (%i)\n", thumbs_count, total_primary_tile_count);
    exit(EXIT_FAILURE);
  }
  if(args->quiet == 0) {
    printf("src-image-resolution:%ix%i\n", primary_tile_x_count, primary_tile_y_count);
  }

  /*
          The data structure of the candidates list.
          For each tile are stored as many candidates as there are primarys tiles. With the largest possible number of multiple selected tiles, a later reduction procedure can lead to only once (best) used tiles, without there being too few alternatives.
  */

  size_t max_candidates_len = unique == 1 ? total_primary_tile_count : 1;
  size_t total_candidates_count = total_primary_tile_count * max_candidates_len;

  if (debug)
    fprintf(stdout, "max_candidates_len:%li, total_candidates_count:%li\n", max_candidates_len, total_candidates_count);

		Heap heap[SIZE_PRIMARY];
		memset(&heap, 0, sizeof(heap));
		//allocate only one memory chunk and spread it over the heaps
		mosaik2_database_candidate *mdc = m_calloc(total_candidates_count+max_candidates_len, sizeof(mosaik2_database_candidate));

		for(uint32_t i=0;i<SIZE_PRIMARY;i++) {
			heap_init(&heap[i], mdc+i*(max_candidates_len)+1);
		}

  if (out)
    printf("%04X %04X %02X %02X", width, height, tile_x_count, tile_y_count);

  double *colors_red            = m_calloc(total_tile_count, sizeof(double));
  double *colors_green          = m_calloc(total_tile_count, sizeof(double));
  double *colors_blue           = m_calloc(total_tile_count, sizeof(double));
  long double *colors_abw_red   = m_calloc(total_tile_count, sizeof(long double));
  long double *colors_abw_green = m_calloc(total_tile_count, sizeof(long double));
  long double *colors_abw_blue  = m_calloc(total_tile_count, sizeof(long double));

  int *colors_red_int   = m_calloc(total_tile_count, sizeof(int));
  int *colors_green_int = m_calloc(total_tile_count, sizeof(int));
  int *colors_blue_int  = m_calloc(total_tile_count, sizeof(int));
  int *stddev_red_int   = m_calloc(total_tile_count, sizeof(int));
  int *stddev_green_int = m_calloc(total_tile_count, sizeof(int));
  int *stddev_blue_int  = m_calloc(total_tile_count, sizeof(int));

  for (int j = 0, j1 = offset_y; j1 < ly; j++, j1++) {
    for (int i = 0, i1 = offset_x; i1 < lx; i++, i1++) {
      // i runs from 0 to length of cropped area
      // j also
      // i1 und j1 are corrected by the offset

      int tile_x = i / pixel_per_tile;
      int tile_y = j / pixel_per_tile;
      int tile_idx = tile_y * tile_x_count + tile_x;
      int color = gdImageTrueColorPixel(im, i1, j1);
      // TODO one function
      int red0 = gdTrueColorGetRed(color);
      int green0 = gdTrueColorGetGreen(color);
      int blue0 = gdTrueColorGetBlue(color);

      if (debug1)
        printf("%i,%i,[%i %i],x:%i,y:%i,idx:%i,tile_count:%i,%i,px_tile:%i,lxy:%i,%i  r:%i,g:%i,b:%i    r:%f,g:%f,b:%f\n", i, j, i1, j1, tile_x, tile_y, tile_idx, tile_x_count, tile_y_count, pixel_per_tile, lx, ly, red0, green0, blue0, red0 / total_pixel_per_tile, green0 / total_pixel_per_tile, blue0 / total_pixel_per_tile);
      // add only fractions to prevent overflow in very large pixel_per_tile settings
      colors_red[tile_idx] += red0 / (total_pixel_per_tile);
      colors_green[tile_idx] += green0 / (total_pixel_per_tile);
      colors_blue[tile_idx] += blue0 / (total_pixel_per_tile);
    }
  }

  for (int j = 0, j1 = offset_y; j1 < ly; j++, j1++) {
    for (int i = 0, i1 = offset_x; i1 < lx; i++, i1++) {

      int tile_x = i / pixel_per_tile;
      int tile_y = j / pixel_per_tile;
      int tile_idx = tile_y * tile_x_count + tile_x;

      if (debug1)
        printf("i:%i j:%i, tile_x:%i tile_y:%i tile_idx:%i c:%f %f %f\n", i, j, tile_x, tile_y, tile_idx, colors_red[tile_idx], colors_green[tile_idx], colors_blue[tile_idx]);
      int color = gdImageTrueColorPixel(im, i1, j1);

      int red0 = gdTrueColorGetRed(color);
      int green0 = gdTrueColorGetGreen(color);
      int blue0 = gdTrueColorGetBlue(color);

      double red2 = colors_red[tile_idx] - red0;
      double green2 = colors_green[tile_idx] - green0;
      double blue2 = colors_blue[tile_idx] - blue0;

      colors_abw_red[tile_idx] += red2 * red2;
      colors_abw_green[tile_idx] += green2 * green2;
      colors_abw_blue[tile_idx] += blue2 * blue2;
    }
  }

  if (debug)
    printf("dim:%i %i,off:%i %i,l:%i %i,primary_tile_cout:%i %i,tile_count:%i %i,pixel_per_tile:%i %f\n", width, height, offset_x, offset_y, lx, ly, primary_tile_x_count, primary_tile_y_count, tile_x_count, tile_y_count, pixel_per_tile, total_pixel_per_tile);
  if (html)
    printf("<html><head><style>table{ width:851px;height:566px; border-collapse: collapse;}td{padding:0;height:0.2em;width:0.2em;}</style></head><body><table>");

  if (out) {

    for (int y = 0; y < tile_y_count; y++) {
      if (html)
        printf("<tr>");

      for (int x = 0; x < tile_x_count; x++) {
        int i = y * tile_x_count + x;
        int red = (int)round(colors_red[i]);
        int green = (int)round(colors_green[i]);
        int blue = (int)round(colors_blue[i]);

        if (debug1)
          printf("avg_red:%i %f", red, colors_red[i]);
        if (debug1)
          printf("avg_green:%i %f", green, colors_green[i]);
        if (debug1)
          printf("avg_blue:%i %f", blue, colors_blue[i]);

        double abw_red = sqrt(colors_abw_red[i] / (total_pixel_per_tile - 1.0));
        double abw_green = sqrt(colors_abw_green[i] / (total_pixel_per_tile - 1.0));
        double abw_blue = sqrt(colors_abw_blue[i] / (total_pixel_per_tile - 1.0));

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

        colors_abw_red[i] = abw_red;
        colors_abw_green[i] = abw_green;
        colors_abw_blue[i] = abw_blue;

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

  for (uint32_t i = 0; i < total_tile_count; i++) {
    colors_red_int[i] = (int)round(colors_red[i]);
    colors_green_int[i] = (int)round(colors_green[i]);
    colors_blue_int[i] = (int)round(colors_blue[i]);

    double sr = sqrt(colors_abw_red[i] / (total_pixel_per_tile - 1.0));
    double sg = sqrt(colors_abw_green[i] / (total_pixel_per_tile - 1.0));
    double sb = sqrt(colors_abw_blue[i] / (total_pixel_per_tile - 1.0));

    sr = sr * 2.0;
    sg = sg * 2.0;
    sb = sb * 2.0;

    if (sr > 255)
      sr = 255;
    if (sg > 255)
      sg = 255;
    if (sb > 255)
      sb = 255;

    stddev_red_int[i] = (int)round(sr);
    stddev_green_int[i] = (int)round(sg);
    stddev_blue_int[i] = (int)round(sb);

    if (debug1) {
      printf("%i:%f %f %f  %Lf %Lf %Lf\n", i, colors_red[i], colors_green[i], colors_blue[i], colors_abw_red[i], colors_abw_green[i], colors_abw_blue[i]);
      printf("%i:%i %i %i  %i %i %i\n", i, colors_red_int[i], colors_green_int[i], colors_blue_int[i], stddev_red_int[i], stddev_green_int[i], stddev_blue_int[i]);
    }
  }

  free(colors_red);
  free(colors_green);
  free(colors_blue);
  free(colors_abw_red);
  free(colors_abw_green);
  free(colors_abw_blue);

  FILE *thumbs_db_tiledims_file    = m_fopen(md.tiledims_filename, "rb");
  FILE *thumbs_db_tileoffsets_file = m_fopen(md.tileoffsets_filename, "rb");
  FILE *thumbs_db_imagecolors_file = m_fopen(md.imagecolors_filename, "rb");
  FILE *thumbs_db_imagestddev_file = m_fopen(md.imagestddev_filename, "rb");
  FILE *thumbs_db_invalid_file     = m_fopen(md.invalid_filename, "rb");
  FILE *thumbs_db_duplicates_file  = m_fopen(md.duplicates_filename, "rb");


  // buffers for db file reading
  uint8_t tile_dims_buf[BUFSIZ];
  unsigned char tileoffsets_buf[BUFSIZ];
  unsigned char colors_buf[BUFSIZ];
  unsigned char stddev_buf[BUFSIZ];
  unsigned char invalid_buf[BUFSIZ];
  unsigned char duplicates_buf[BUFSIZ];

  // SAVING PRIMARY TILE DIMENSIONS
  FILE *primarytiledims_file = m_fopen(mp.dest_primarytiledims_filename, "w");
  fprintf(primarytiledims_file, "%i	%i", primary_tile_x_count, primary_tile_y_count);
  m_fclose(primarytiledims_file);

  uint64_t idx = 0;

	uint32_t candidates_insert=0;
	uint32_t candidates_pop=0;
	uint32_t candidates_pop_uniqueness[SIZE_PRIMARY];
	memset(& candidates_pop_uniqueness,0, sizeof(candidates_pop_uniqueness));
	uint32_t candidates_toobad=0;
	float diff_best=FLT_MAX, diff_worst=0;
	uint32_t candidates_elect[SIZE_PRIMARY];
	memset(&candidates_elect,0,sizeof(candidates_elect));

  int32_t percent = -1;
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
      unsigned char thumbs_db_tileoffset_x = tileoffsets_buf[i];
      unsigned char thumbs_db_tileoffset_y = tileoffsets_buf[i + 1];
      int thumbs_db_tileoffsets_unset = thumbs_db_tileoffset_x == 0xFF && thumbs_db_tileoffset_y == 0xFF;

      uint32_t thumbs_db_total_tile_count = thumbs_db_tile_x_count * thumbs_db_tile_y_count;

      if(args->quiet == 0) {

	      float new_percent_f = idx / (thumbs_count * 1.0);
	      uint8_t new_percent = round(new_percent_f * 100);

	      if (new_percent != percent) {
		      time_t now;
		      time(&now);
		      percent = new_percent;
		      printf("%i%%, %lu/%lu %s", percent, idx, thumbs_count, ctime(&now));
		}
      }

      if (invalid_buf[i / 2] != 0) { // valid entries must have 0
        if (debug)
          fprintf(stderr, "thumb (%lu) is marked as invalid, will be skipped\n", idx);

        // in case of invalid entries move the file pointers without reading the colors data
        m_fseeko(thumbs_db_imagecolors_file, thumbs_db_total_tile_count * RGB, SEEK_CUR);
        m_fseeko(thumbs_db_imagestddev_file, thumbs_db_total_tile_count * RGB, SEEK_CUR);

        continue;
      }

      // this could be slow, but works definitly
      // if an thumb is valid read its image data
      m_fread(colors_buf, thumbs_db_total_tile_count * RGB, thumbs_db_imagecolors_file);
      m_fread(stddev_buf, thumbs_db_total_tile_count * RGB, thumbs_db_imagestddev_file);

      uint8_t shift_x_len = thumbs_db_tile_x_count - thumbs_tile_count;
      uint8_t shift_y_len = thumbs_db_tile_y_count - thumbs_tile_count;

      if (shift_x_len != 0 && shift_y_len != 0) {
        fprintf(stderr, "one shift axis should be zero but: %i %i at thumb_idx: %lu\n", shift_x_len, shift_y_len, idx
                //, thumbs_db_tile_x_count, thumbs_db_tile_y_count, thumbs_tile_count);
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
        fprintf(stderr, "%lu shift_len:%i %i\n", idx, shift_x_len, shift_y_len);
      }


      mosaik2_database_candidate mdc0, mdc_best, mdc_worst;

      for (uint32_t primary_y = 0; primary_y < primary_tile_y_count; primary_y++) {
        for (uint32_t primary_x = 0; primary_x < primary_tile_x_count; primary_x++) {

          uint32_t primary_tile_idx = primary_tile_x_count * primary_y + primary_x;


          memset(&mdc_best, 0, sizeof(mdc_best));
	  memset(&mdc_worst, 0, sizeof(mdc_worst));

	  mdc_best.costs = FLT_MAX;
	  mdc_worst.costs = 0;

          for (uint8_t shift_y = shift_y_0; shift_y <= shift_y_len; shift_y++) {
            for (uint8_t shift_x = shift_x_0; shift_x <= shift_x_len; shift_x++) {

	      mdc0.candidate_idx = idx;
	      mdc0.primary_tile_idx = primary_tile_idx;
	      mdc0.costs = 0;
	      mdc0.off_x = shift_x;
	      mdc0.off_y = shift_y;



              int thumbs_total_tile_count = thumbs_tile_count * thumbs_tile_count;
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

                uint32_t colors_idx = k % thumbs_tile_count + (k / thumbs_tile_count) * primary_tile_x_count * thumbs_tile_count + (primary_tile_idx % primary_tile_x_count) * thumbs_tile_count + (primary_tile_idx / primary_tile_x_count) * primary_tile_x_count * thumbs_tile_count * thumbs_tile_count;

                uint32_t j = k % thumbs_tile_count + (k / thumbs_tile_count) * thumbs_db_tile_x_count + shift_x + shift_y * thumbs_db_tile_x_count;

                if (color_distance == MOSAIK2_ARGS_COLOR_DISTANCE_MANHATTAN) {
                  int diff_c_r = abs(colors_red_int[colors_idx]   - (int)colors_buf[j * RGB + R]);
                  int diff_c_g = abs(colors_green_int[colors_idx] - (int)colors_buf[j * RGB + G]);
                  int diff_c_b = abs(colors_blue_int[colors_idx]  - (int)colors_buf[j * RGB + B]);

                  int diff_s_r = abs(((int)stddev_red_int[colors_idx])   - (int)stddev_buf[j * RGB + R]);
                  int diff_s_g = abs(((int)stddev_green_int[colors_idx]) - (int)stddev_buf[j * RGB + G]);
                  int diff_s_b = abs(((int)stddev_blue_int[colors_idx])  - (int)stddev_buf[j * RGB + B]);

                  float diff0 = image_ratio * (diff_c_r + diff_c_g + diff_c_b) + stddev_ratio * ((diff_s_r + diff_s_g + diff_s_b) / 2.0);
                  mdc0.costs += diff0 / total_pixel_per_tile;

                } else if (color_distance == MOSAIK2_ARGS_COLOR_DISTANCE_EUCLIDIAN) {
			double diff_c = sqrt(
				  pow(colors_red_int[colors_idx]   - (int)colors_buf[j * RGB + R], 2)
				+ pow(colors_green_int[colors_idx] - (int)colors_buf[j * RGB + G], 2)
				+ pow(colors_blue_int[colors_idx]  - (int)colors_buf[j * RGB + B], 2)
			);
			double diff_s = sqrt(
				  pow((int)stddev_red_int[colors_idx]   - (int)stddev_buf[j * RGB + R], 2)
				+ pow((int)stddev_green_int[colors_idx] - (int)stddev_buf[j * RGB + G], 2)
				+ pow((int)stddev_blue_int[colors_idx]  - (int)stddev_buf[j * RGB + B], 2)
			);
			float diff0 = image_ratio * diff_c + stddev_ratio * diff_s;
			float diff1 = diff0 / total_pixel_per_tile;
			mdc0.costs += diff1;

                } else if (color_distance == MOSAIK2_ARGS_COLOR_DISTANCE_CHEBYSHEV) {
                  int diff_c_r = abs(colors_red_int[colors_idx]   - (int)colors_buf[j * RGB + R]);
                  int diff_c_g = abs(colors_green_int[colors_idx] - (int)colors_buf[j * RGB + G]);
                  int diff_c_b = abs(colors_blue_int[colors_idx]  - (int)colors_buf[j * RGB + B]);
                  int diff_c = diff_c_r;
                  if (diff_c_g > diff_c)
                    diff_c = diff_c_g;
                  if (diff_c_b > diff_c)
                    diff_c = diff_c_b;

                  int diff_s_r = abs(((int)stddev_red_int[colors_idx])   - (int)stddev_buf[j * RGB + R]);
                  int diff_s_g = abs(((int)stddev_green_int[colors_idx]) - (int)stddev_buf[j * RGB + G]);
                  int diff_s_b = abs(((int)stddev_blue_int[colors_idx])  - (int)stddev_buf[j * RGB * B]);
                  int diff_s = diff_s_r;
                  if (diff_s_g > diff_s)
                    diff_s = diff_s_g;
                  if (diff_s_b > diff_s)
                    diff_s = diff_s_b;

                  float diff0 = image_ratio * diff_c + stddev_ratio * diff_s;
                  mdc0.costs += diff0 / total_pixel_per_tile;

                } else {
                  fprintf(stderr, "invalid color distance\n");
                  exit(EXIT_FAILURE);
                }
              } // thumbs_total_tile_count end

	      // only the best shift will be saved for this candidate in this primary tile
	      // unique or none unique
	      if( mdc0.costs < mdc_best.costs) {
		      memcpy(&mdc_best, &mdc0, sizeof(mdc_best));
	      }
	      if( mdc0.costs > mdc_worst.costs ) {
		      memcpy(&mdc_worst, &mdc0, sizeof(mdc_worst));
	      }
            }     // for shift_x
          }       // for shift_y

	  if( mdc_worst.costs - mdc_best.costs > diff_worst )
		  diff_worst = mdc_worst.costs - mdc_best.costs;
	  if( mdc_worst.costs - mdc_best.costs < diff_best  &&( shift_x_0 < shift_x_len-1 || shift_y_0 < shift_y_len-1 )) // only if theres more than one shift..
		  diff_best = mdc_worst.costs - mdc_best.costs;

              // THIS is the point where the calculated color difference
              // is compared to the already stored ones.
              // If its lower, it is more equal and it should be the new candidate
              // if(diff_color0 < candidates_costs[primary_tile_idx] ) 
              // worst costs is saved in the last position, and this position
              // will expand from 0 to total_primary_tile_count

	      mosaik2_database_candidate max_heap;
	      int empty = max_heap_peek( &heap[primary_tile_idx], &max_heap );

	      if(heap[primary_tile_idx].last < max_candidates_len || max_heap.costs > mdc_best.costs || empty == 1) {
		      if( heap[primary_tile_idx].count >= max_candidates_len ) {
				mosaik2_database_candidate c;
				max_heap_pop( &heap[primary_tile_idx], &c);
				candidates_pop++;
			}
		      candidates_insert++;
			max_heap_insert(&heap[primary_tile_idx], &mdc_best);
		}
	      else {
		      candidates_toobad++;
	      }
                 // better candidate found
        }         // for primary_x
      }           // for primary_y
    }             // for idx

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

  free(colors_red_int);
  free(colors_green_int);
  free(colors_blue_int);
  free(stddev_red_int);
  free(stddev_green_int);
  free(stddev_blue_int);

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
						assert( candidates_elect[i] != total_primary_tile_count);
						unique_run = 0;
					}
        			}
			}
			// if a do while round has any replacement => it is not unique yet
			// if a do while round has any replacement => it is not unique yet
		} while (unique_run == 0);
	}

  // intialize with -1
  FILE *mosaik2_result = m_fopen(mp.dest_result_filename, "wb");
  float candidate_best_costs = FLT_MAX, candidate_worst_costs=0;
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
      for (int i = 0; i < total_primary_tile_count; i++) {
        sum_elect += candidates_pop_uniqueness[i];
        if( max_elect < candidates_pop_uniqueness[i])
          max_elect = candidates_pop_uniqueness[i];
      }
      printf("create uniqueness pop:%li max primary tile value pop:%i possible maximum:%li\n", sum_elect, max_elect, max_candidates_len);
    }


  free(mdc);
  return 0;
}

