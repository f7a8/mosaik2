#include "mosaik21.h"
#include <gd.h>
#include <libexif/exif-data.h>

gdImagePtr gdImageRotate90 (gdImagePtr src);
gdImagePtr gdImageRotate180 (gdImagePtr src);
gdImagePtr gdImageRotate270 (gdImagePtr src);
static void trim_spaces(char *buf);

uint8_t ORIENTATION_TOP_LEFT=0;
uint8_t ORIENTATION_RIGHT_TOP=1;
uint8_t ORIENTATION_BOTTOM_RIGHT=2;
uint8_t ORIENTATION_LEFT_BOTTOM=3;

uint8_t get_image_orientation(unsigned char *buffer, size_t buf_size) {
	
	ExifData *ed;
	ExifEntry *entry;

  /* Load an ExifData object from an EXIF file */
	ed = exif_data_new_from_data(buffer, buf_size);
  if (ed==NULL) {
		printf("unable to create exif data\n");
		exit(EXIT_FAILURE);
	}

	entry = exif_content_get_entry(ed->ifd[EXIF_IFD_0], EXIF_TAG_ORIENTATION);
	if (entry) {
		char buf[64];
		if (exif_entry_get_value(entry, buf, sizeof(buf))) {
    	trim_spaces(buf);

      if (strcmp(buf, "Right-top")==0) {
				return ORIENTATION_RIGHT_TOP;
			} else if(strcmp(buf, "Bottom-right")==0) {
				return ORIENTATION_BOTTOM_RIGHT;
			} else if(strcmp(buf, "Left-Bottom")==0) {
				return ORIENTATION_LEFT_BOTTOM;
			}
			return ORIENTATION_TOP_LEFT;
    }
	}
}

gdImagePtr myLoadPng(char *filename, char *origin_name) {
   FILE *in;
   struct stat stat_buf;
   gdImagePtr im;
   in = fopen(filename, "rb");
   if (in==NULL) {
     fprintf(stderr,"image (%s) could not be loaded\n", filename);
     exit(EXIT_FAILURE);
   } 
   if (fstat(fileno(in), &stat_buf) != 0) {
     fprintf(stderr,"fstat error\n");
     exit(EXIT_FAILURE);
   } 
   /* Read the entire thing into a buffer
     that we allocate */
   char *buffer = malloc(stat_buf.st_size);
   if (!buffer) { 
     fprintf(stderr,"could not allocate memory\n");
     exit(EXIT_FAILURE);
   } 
   if (fread(buffer, 1, stat_buf.st_size, in)
     != stat_buf.st_size) {
     fprintf(stderr,"data could not be read\n");
     exit(EXIT_FAILURE);
   } 
	 if(EndsWith(origin_name,"png")|| EndsWith(origin_name,"PNG")) {
   	im = gdImageCreateFromPngPtr(    stat_buf.st_size, buffer);
	 }else {
   	im = gdImageCreateFromJpegPtr(    stat_buf.st_size, buffer);
	 }

	
	uint8_t orientation = get_image_orientation(buffer, stat_buf.st_size);
//uint8_t ORIENTATION_TOP_LEFT=0;
//uint8_t ORIENTATION_RIGHT_TOP=1; 270
//uint8_t ORIENTATION_BOTTOM_RIGHT=2; 180
//uint8_t ORIENTATION_LEFT_BOTTOM=3; 90
	if(orientation == ORIENTATION_BOTTOM_RIGHT ) {
		gdImagePtr im2 = gdImageRotate180(im);
		gdImageDestroy(im);
		im=im2;
	} else if(orientation == ORIENTATION_RIGHT_TOP) { 
  	gdImagePtr im2;
		im2 = gdImageRotate270(im); // 270
		gdImageDestroy(im);
		im = im2;
	//	case ORIENTATION_BOTTOM_RIGHT: im = gdImageRotate90(im,0); break;//180
	//	case ORIENTATION_LEFT_BOTTOM: im = gdImageRotate90(im,0); break;
	} else if(orientation == ORIENTATION_LEFT_BOTTOM ) {
		gdImagePtr im2 = gdImageRotate90(im);
		gdImageDestroy(im);
		im = im2;
	}

	

   free(buffer);
   fclose(in);
   return im;
 } 
/* Remove spaces on the right of the string */
static void trim_spaces(char *buf) {
    char *s = buf-1;
    for (; *buf; ++buf) {
        if (*buf != ' ')
            s = buf;
    }
    *++s = 0; /* nul terminate the string on the first of the final spaces */
}



/* Show the tag name and contents if the tag exists */
static void show_tag(ExifData *d, ExifIfd ifd, ExifTag tag)
{
    /* See if this tag exists */
    ExifEntry *entry = exif_content_get_entry(d->ifd[ifd],tag);
    if (entry) {
        char buf[1024];

        /* Get the contents of the tag in human-readable form */
        exif_entry_get_value(entry, buf, sizeof(buf));

        /* Don't bother printing it if it's entirely blank */
        trim_spaces(buf);
        if (*buf) {
            printf("%s\t%s\t%02X\n", exif_tag_get_name_in_ifd(tag,ifd), buf, entry->data[0]);
        }
    }
}

/* Show the given MakerNote tag if it exists */
static void show_mnote_tag(ExifData *d, unsigned tag)
{
    ExifMnoteData *mn = exif_data_get_mnote_data(d);
    if (mn) {
        int num = exif_mnote_data_count(mn);
        int i;

        /* Loop through all MakerNote tags, searching for the desired one */
        for (i=0; i < num; ++i) {
            char buf[1024];
            if (exif_mnote_data_get_id(mn, i) == tag) {
                if (exif_mnote_data_get_value(mn, i, buf, sizeof(buf))) {
                    /* Don't bother printing it if it's entirely blank */
                    trim_spaces(buf);
                    if (*buf) {
                        printf("%s: %s\n", exif_mnote_data_get_title(mn, i),
                            buf);
                    }
                }
            }
        }
    }
}

/* Rotates an image by 90 degrees (counter clockwise) */
gdImagePtr gdImageRotate90 (gdImagePtr src) {
	fprintf(stderr,"gdImageRotate90\n");
	int uY, uX;
	int c;
	gdImagePtr dst;
	
	dst = gdImageCreateTrueColor(src->sy, src->sx);
	if (dst != NULL) {
		for (uY = 0; uY<src->sy; uY++) {
			for (uX = 0; uX<src->sx; uX++) {
				c =  src->tpixels[uY][uX];
				gdImageSetPixel(dst, uY, (dst->sy - uX - 1), c);
			}
		}
	}

	return dst;
}
/* Rotates an image by 180 degrees (counter clockwise) */
gdImagePtr gdImageRotate180 (gdImagePtr src) {
	fprintf(stderr,"rotate 180\n");
	int uY, uX;
	int c;
	gdImagePtr dst;
 
	dst = gdImageCreateTrueColor(src->sx, src->sy);
	if (dst != NULL) {
		for (uY = 0; uY<src->sy; uY++) {
			for (uX = 0; uX<src->sx; uX++) {
				c = src->tpixels[uY][uX];
				
			//	fprintf(stderr, "c:%i @ uX:%i uY:%i sx:%i sy:%i dx:%i dy:%i tx:%i ty:%i\n",c, uX, uY, src->sx, src->sy, dst->sx, dst->sy,(dst->sx - uX-1),(dst->sy-uY-1));
				gdImageSetPixel(dst, (dst->sx - uX - 1), (dst->sy - uY - 1), c);
			}
		}
	}
	return dst;
}	
/* Rotates an image by 90 degrees (counter clockwise) */
gdImagePtr gdImageRotate270 (gdImagePtr src) {
	fprintf(stderr,"rotate 270\n");
	int uY, uX;
	int c;
	gdImagePtr dst;

	dst = gdImageCreateTrueColor (src->sy, src->sx);

	if (dst != NULL) {
		for (uY = 0; uY<src->sy; uY++) {
			for (uX = 0; uX<src->sx; uX++) {
				c = src->tpixels[uY][uX];
			//	fprintf(stderr, "c:%i @ %i:%i sx:%i sy:%i\n",c, uX, uY, src->sy, src->sx);
				gdImageSetPixel(dst, (dst->sx - uY - 1), uX, c);
			}
		}
	}
	return dst;
}	
