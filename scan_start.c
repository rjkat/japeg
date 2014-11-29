#include <assert.h>
#include <stdlib.h>
#include <stdint.h>
#include "scan_start.h"
#include "frame.h"
#include "jpeg_segment.h"

/* Number of bytes that are always present in SOS header */
#define SCAN_START_HEADER_FIXED_BYTES         4

/* Number of bytes each component takes up */
#define SCAN_START_HEADER_BYTES_PER_COMPONENT 2

/* Minimum possible length - fixed bytes plus the bytes for one component */
#define SCAN_START_HEADER_MIN_LENGTH_BYTES    (SCAN_START_HEADER_FIXED_BYTES \
                                             + SCAN_START_HEADER_BYTES_PER_COMPONENT)

scan_start *scan_start_create(const jpeg_segment *segment
                             ,frame              *f
                             ,size_t              file_bytes_remaining
                             ) {
   scan_start *s = NULL;
   size_t i = 0;
   uint8_t num_components;
   assert(segment);
   if (segment->data_size < SCAN_START_HEADER_MIN_LENGTH_BYTES) {
      return NULL;
   }
   s = malloc(sizeof(scan_start));
   assert(s);
   num_components = segment->data[i];
   i += 1;
   if (num_components != f->num_components) {
      free(s);
      return NULL;
   }
   if (segment->data_size != (SCAN_START_HEADER_FIXED_BYTES + 
                              num_components 
                              * SCAN_START_HEADER_BYTES_PER_COMPONENT)) {
      free(s);
      return NULL;
   }
   size_t n;
   for (n = 0; n < num_components; n++) {
      uint8_t component_id = segment->data[i];
      component *c         = frame_get_component_with_id(f, component_id);
      i += 1;
      c->ac_htable_id =  segment->data[i]       & 0xF;
      c->dc_htable_id = (segment->data[i] >> 4) & 0xF;
      i += 1;
   }
   s->spectral_selection_start =  segment->data[i];
   i += 1;
   s->spectral_selection_end   =  segment->data[i];
   i += 1;
   s->successive_approx_high   =  segment->data[i]       & 0xF;
   s->successive_approx_low    = (segment->data[i] >> 4) & 0xF;
   i += 1; 
   s->stream = jpeg_stream_create(file_bytes_remaining - i
                                 ,segment->data + segment->data_size
                                 );
   return s;
}

void scan_start_destroy(scan_start *s) {
   jpeg_stream_destroy(s->stream);
   free(s);
}

