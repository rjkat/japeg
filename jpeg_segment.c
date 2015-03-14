#include "jpeg_segment.h"

unsigned int read_word(unsigned char *buf) {
   return (((unsigned int) buf[0]) << 8) + (unsigned int) buf[1]; 
}

jpeg_segment *jpeg_segment_create(unsigned char  marker
                                 ,unsigned char *data
                                 ,size_t   data_size) {
   jpeg_segment *new = malloc(sizeof(jpeg_segment));
   if (!new) {
      return NULL;
   }
   new->marker    = marker;
   new->data      = data;
   new->data_size = data_size;
   return new;
}

void jpeg_segment_destroy(jpeg_segment *segment) {
   free(segment);
}
