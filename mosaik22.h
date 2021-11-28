#include "mosaik21.h"
#include <gd.h>

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
   free(buffer);
   fclose(in);
   return im;
 } 
