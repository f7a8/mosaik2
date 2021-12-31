# mosaik2 - creates real photo mosaics

mosaik2 is a Command Line Interface program for Linux, especially designed for large amounts of data.

website at https://f7a8.github.io/mosaik2/
hosted at https://github.com/f7a8/mosaik2/




## Dependend software

1. gcc
2. nodejs in version >= 14.16
3. libexif
4. libgd
5. libssl

## USAGE

1. create file list (read README.file_list)

2. node mosaik2.js init first_mosaik2 16
3. node mosaik2.js index first_mosaik2 4 10 < first_mosaik2.file_list
4. gathering 40 6353080 my_first_mosaik2.jpeg 100 first_mosaik2 < source_image.jpeg
5. join my_first_mosaik2.jpeg 150 first_mosaik2

## Usage Description

2. initialize a mosaik2 database named first_mosaik2. You can create multiple mosaik2 databases to handle a large volume of images by processing them in parallel. 16 can be understood here as reduced resolution of the indexed images. Each image is reduced to n*16 or 16*n pixels in the same aspect ratio.The larger this number, the more exactly will fit canidata images in the mosaic, the longer the computation time. Example: with a resolution of 16 for all JPEG images from Wikimedia Commons (53 million images ~ 165 terabytes), the mosaik2 database occupies about 130 gigabytes. 

3. The actual indexing process. Here, for the mosaik2 database "first_mosaik2", all files from the "first_mosaik2.file_list" are indexed. The parameter with the value 4 means that 4 indexing processes (download + color analysis) run in parallel. The parameter with the value 10 means that at a system load avg > 10 the indexing is reduced until the value is fallen below.

4. Only here we are talking about the actual image from which to create a mosaic. Explanation of the parameters:
  1. number of tiles of the narrower side in which small photos are to be inserted.
  2. size of the actual image in bytes.
  3. name of the target file.
  4. ratio between color deviations and color values. 100 means evaluate color values only, 0 means evaluate color deviations only.
  5. name of the mosaik2 database.
  as STDIN the actual image from which a mosaic is to be created.

5. The results of (multiple) gatherings are merged here and the final candidate images are loaded and stored locally (~/.mosaik2/) and is then assembled into a mosaic image and stored as the target image. The parameters mean:
  1. target filename (same as in step before)
  2. size in pixels of the individual tiles 
  3. name of the mosaik2 database
  4. other names of mosaik2 databases (must have been indexed in the same resolution e.g. 16)
  The home directory is determined by the HOME environment variable. 

