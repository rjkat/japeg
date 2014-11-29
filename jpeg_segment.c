#include "jpeg_segment.h"

uint16_t read_word(uint8_t *buf) {
   return (((uint16_t) buf[0]) << 8) + (uint16_t) buf[1]; 
}

jpeg_segment *jpeg_segment_create(uint8_t  marker
                                 ,uint8_t *data
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
