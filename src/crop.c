#include "libmosaik2.h"

int mosaik2_crop(mosaik2_arguments *args) {

	m2name mosaik2_database_name = args->mosaik2db;
	m2elem element_number = args->element_number;

	if(args->has_element_identifier == ELEMENT_NUMBER && element_number < 1  ) {
		fprintf(stderr, "illegal value of element_number. exit\n");
		exit(EXIT_FAILURE);
	}

	mosaik2_database md;
	mosaik2_database_init(&md, mosaik2_database_name);
	mosaik2_database_check(&md);
	int database_image_resolution = mosaik2_database_read_image_resolution(&md); // standard value should be 16

	if (args->has_element_identifier == ELEMENT_FILENAME ) {
		int val = mosaik2_database_find_element_number(&md, args->element_filename, &element_number);
		if(val != 0) {
			fprintf(stderr, "element not found\n");
			exit(EXIT_FAILURE);
		}
	} else {
		element_number--;
	}

	m2elem element_count = mosaik2_database_read_element_count(&md);
	if( element_number >= element_count ) {
		fprintf(stderr, "element number out of range\n");
		exit(EXIT_FAILURE);
	}

	unsigned char tiledims[2];
	read_entry(md.tiledims_filename, &tiledims, sizeof(tiledims), element_number*sizeof(tiledims));
	
	unsigned char tiledim_x = tiledims[0];
	unsigned char tiledim_y = tiledims[1];
	
	// one side of tiledims must be 16
	if(!(tiledims[0] == database_image_resolution || tiledims[1] == database_image_resolution)) {
		fprintf(stderr, "invalid tiledim values %ix%i, one should be database-image-resolution:%i\n", tiledims[0], tiledims[1], database_image_resolution);
		exit(EXIT_FAILURE);
	}

	//then common image formats could be 16x22 or 22x16
	//now preventing tile offset could be to high

	unsigned char tileoffset_x = args->num_tiles;/* because only one offset is submitted, one must be 0  */
	unsigned char tileoffset_y = args->num_tiles;

	if( tiledim_x == database_image_resolution )
		tileoffset_x = 0; // no scrolling possible
	if( tiledim_y == database_image_resolution ) 
		tileoffset_y = 0;


	if(tiledim_x == database_image_resolution) { // x is narrow side
		//when user offset + db_image_res overlaps the tile dimension => exit
		if(tileoffset_y + database_image_resolution >= tiledim_y ) {
			fprintf(stderr, "tileoffset y (%i) is out of range (%i)\n", tileoffset_y, tiledim_y);
			exit(EXIT_FAILURE);
		}
	} else { // y is narrow side
		if(tileoffset_x + database_image_resolution >= tiledim_x ) {
			fprintf(stderr, "tileoffset x (%i) is out of range (%i)\n", tileoffset_x, tiledim_x);
			exit(EXIT_FAILURE);
		}
	}
		
	unsigned char tileoffsets[2];
	tileoffsets[0] = tileoffset_x;
	tileoffsets[1] = tileoffset_y;
	write_entry(md.tileoffsets_filename, &tileoffsets, sizeof(tileoffsets), element_number*sizeof(tileoffsets));

	return 0;
}
