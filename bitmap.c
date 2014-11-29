#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <assert.h>
#include "bitmap.h"
#include "bitmap_internal.h"

/* Values for bitmap header */
#define BITMAP_FILETYPE       19778
#define BITMAP_RESERVED_BITS  0
#define BITMAP_PIXEL_OFFSET   0x36
#define BITMAP_HEADER_SIZE    4

/* Values for DIB header for bitmap image */
#define DIB_HEADER_SIZE           11
#define DIB_NUM_BYTES             0x28
#define BITMAP_NUM_COLOUR_PLANES  1
#define BITMAP_BITS_PER_PIXEL    (8 * BITMAP_BYTES_PER_PIXEL)
#define BITMAP_COMPRESSION        0

#define BITMAP_HORIZ_RESOLUTION   2835
#define BITMAP_VERT_RESOLUTION    2835
#define BITMAP_COLOURS_IN_PALETTE 0
#define BITMAP_IMPORTANT_COLOURS  0

#define BITMAP_HEADER_TOTAL_BYTES 54

#define BITMAP_HEADER_FILE_SIZE_POS     1
#define DIB_HEADER_WIDTH_POS            1
#define DIB_HEADER_HEIGHT_POS           2
#define DIB_HEADER_PIXEL_ARRAY_SIZE_POS 6

/* Used to index into the bitmap_header and dib_header arrays */
#define BITMAP_HEADER_VALUE     0
#define BITMAP_HEADER_NUM_BYTES 1

/* Contains all elements of bitmap header, together with their
* size in bytes */
static const uint32_t bitmap_header[BITMAP_HEADER_SIZE][2] 
                         = {{BITMAP_FILETYPE,      2}
                            /* File size */
                           ,{0,                    4}
                           ,{BITMAP_RESERVED_BITS, 4}
                           ,{BITMAP_PIXEL_OFFSET,  4}};

/* Contains all elements of DIB header, together with their
* size in bytes */
static const uint32_t dib_header[DIB_HEADER_SIZE][2] 
                         = {{DIB_NUM_BYTES,             4}
                           /* Width in pixels */
                           ,{0,                         4}
                           /* Height in pixels */
                           ,{0,                         4}
                           ,{BITMAP_NUM_COLOUR_PLANES,  2}
                           ,{BITMAP_BITS_PER_PIXEL,     2}
                           ,{BITMAP_COMPRESSION,        4}
                           /* Pixel array size */
                           ,{0,                         4}
                           ,{BITMAP_HORIZ_RESOLUTION,   4}
                           ,{BITMAP_VERT_RESOLUTION,    4}
                           ,{BITMAP_COLOURS_IN_PALETTE, 4}
                           ,{BITMAP_IMPORTANT_COLOURS,  4}};


/* Padding so each row is a multiple of 4 bytes */
static uint32_t get_num_padding_bytes(bitmap *b) {
   assert(b);
   return (4 - ((uint32_t) b->num_cols * BITMAP_BYTES_PER_PIXEL) % 4) % 4;
}

/* Total number of bytes (3 for each pixel) 
 * plus padding for each row */
static uint32_t get_pixel_array_size(bitmap *b) {
   assert(b);
   return ((uint32_t) b->num_rows * (uint32_t) b->num_cols * BITMAP_BYTES_PER_PIXEL 
           + (uint32_t) b->num_rows * get_num_padding_bytes(b));
}

/* Total number of bytes in the file */
static uint32_t get_file_size(bitmap *b) {
   assert(b);
   return BITMAP_HEADER_TOTAL_BYTES + get_pixel_array_size(b);
}

static int can_fit_in_four_bytes(size_t n) {
   return ((size_t) ((uint32_t) n) == n);
}

static uint8_t clip(float f) {
   if (f > 255.0f) {
      f = 255.0f;
   } else if (f < 0.0f) {
      f = 0.0f;
   }
   return (uint8_t) f;
}

int bitmap_write(bitmap *b, const char *filename) {
   FILE *fp;
   size_t i;
   size_t bytes_written;
   if (!filename || !b) {
      return (-1);
   }
   /* If the number of pixels in a row/column doesn't fit in four bytes,
    * give up. We can then safely cast num_rows and num_cols to uint32_t. */
   if (  !can_fit_in_four_bytes(b->num_cols) 
      || !can_fit_in_four_bytes(b->num_rows)) {
      return (-1);
   }
   fp = fopen(filename, "wb");
   if (!fp) {
      perror("Error opening file");
      return (-1);
   }
   for (i = 0; i < BITMAP_HEADER_SIZE; i++) {
      uint32_t value = bitmap_header[i][BITMAP_HEADER_VALUE];
      if (i == BITMAP_HEADER_FILE_SIZE_POS) {
         value = get_file_size(b);
      }
      bytes_written = fwrite(&value, 1, bitmap_header[i][BITMAP_HEADER_NUM_BYTES], fp);
      if (bytes_written != bitmap_header[i][BITMAP_HEADER_NUM_BYTES]) {
         perror("Error writing to bitmap file");
         fclose(fp);
         return (-1);
      }
   }
  
   for (i = 0; i < DIB_HEADER_SIZE; i++) {
      uint32_t value = dib_header[i][BITMAP_HEADER_VALUE];
      if (i == DIB_HEADER_HEIGHT_POS) {
         value = (uint32_t) b->num_rows;
      } else if (i == DIB_HEADER_WIDTH_POS) {
         value = (uint32_t) b->num_cols;
      } else if (i == DIB_HEADER_PIXEL_ARRAY_SIZE_POS) {
         value = get_pixel_array_size(b);
      }
      bytes_written = fwrite(&value, 1, dib_header[i][BITMAP_HEADER_NUM_BYTES], fp);
      if (bytes_written != dib_header[i][BITMAP_HEADER_NUM_BYTES]) {
         perror("Error writing to bitmap file");
         fclose(fp);
         return (-1);
      }
   }

   for (i = 0; i < b->num_rows; i++) {
      size_t j;
      for (j = 0; j < b->num_cols; j++) {
         size_t k;
         for (k = 0; k < BITMAP_NUM_CHANNELS; k++) {
            uint8_t val = clip(b->samples[k][(b->num_rows - i - 1) * b->num_cols + j]);
            if (fwrite(&val, 1, 1, fp) != 1) {
               perror("Error writing to bitmap file");
               fclose(fp);
               return (-1);
            }
         }
      }
      for (j = 0; j < get_num_padding_bytes(b); j++) {
         if (fputc('\0', fp) != '\0') {
            perror("Error writing to bitmap file");
            fclose(fp);
            return (-1);
         }
      }
   }
   return 0;
}


void bitmap_destroy(bitmap *b) {
   size_t i;
   for (i = 0; i < BITMAP_NUM_CHANNELS; i++) {
      free(b->samples[i]);
   }
}

