/*
* japeg - a simple C JPEG decoder.
*/

#include <stdio.h>
#include <stdlib.h>
#include "jpeg.h"
#include "convert.h"
#include "bitmap.h"

#define NUM_ARGS     3
#define ARG_IN_FILE  1
#define ARG_OUT_FILE 2 

int main(int argc, char *argv[]) {
   int ret = EXIT_FAILURE;
   if (argc < NUM_ARGS) {
      printf("Usage: %s in_file.jpg out_file.bmp\n", argv[0]);
      return EXIT_FAILURE;
   }
   char *in_file  = argv[ARG_IN_FILE];
   char *out_file = argv[ARG_OUT_FILE]; 
   jpeg *j = jpeg_read(in_file);
   if (j) {
      bitmap *b = jpeg_to_bitmap(j);
      if (b) {
         int err = bitmap_write(b, out_file);
         if (!err) {
            ret = EXIT_SUCCESS;
         }
         bitmap_destroy(b);
      }
      jpeg_destroy(j);
   }
   return ret;
}
