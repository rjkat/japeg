#include <assert.h>
#include "frame.h"
#include "jpeg_segment.h"

#define FRAME_MIN_LENGTH_BYTES         6
#define FRAME_SUPPORTED_PRECISION_BITS 8
#define COMPONENT_LENGTH_BYTES         3

static void read_component(uint8_t *buf, component *c) {
   assert(c);
   assert(buf);
   c->id                         =  buf[0];
   c->sampling_factor_horizontal =  buf[1]       & 0xF;
   c->sampling_factor_vertical   = (buf[1] >> 4) & 0xF;
   c->qtable_id                  =  buf[2];
   c->prev_dc_coeff              =  0;
}

component *frame_get_component_with_id(frame *f, component_id id) {
   unsigned int i;
   component *c = NULL;
   for (i = 0; i < f->num_components; i++) {
      if (f->components[i].id == id) {
         c = &f->components[i];
      }
   }
   return c;
}

frame *frame_create(const jpeg_segment *segment) {
   size_t i = 0;
   frame *f;
   assert(segment);
   if (segment->data_size < FRAME_MIN_LENGTH_BYTES) {
      return NULL;
   }
   f = malloc(sizeof(frame));
   assert(f);
   f->precision_bits = segment->data[i];
   f->highest_sampling_factor = 0;
   i += 1;
   if (f->precision_bits != FRAME_SUPPORTED_PRECISION_BITS) {
      return NULL;
   }
   f->num_lines = read_word(&segment->data[i]);
   i += 2;
   f->samples_per_line = read_word(&segment->data[i]);
   i += 2;
   f->num_components = segment->data[i];
   i += 1;
   if ((i + f->num_components * COMPONENT_LENGTH_BYTES) > segment->data_size) {
      return NULL;
   }
   size_t n = 0;
   for (n = 0; n < f->num_components; n++) {
      read_component(&segment->data[i], &f->components[n]);
      if (f->components[n].sampling_factor_horizontal > f->highest_sampling_factor) {
         f->highest_sampling_factor = f->components[n].sampling_factor_horizontal;
      }
      if (f->components[n].sampling_factor_vertical > f->highest_sampling_factor) {
         f->highest_sampling_factor = f->components[n].sampling_factor_vertical;
      }
      i += COMPONENT_LENGTH_BYTES;
   }
   return f;
}

void frame_destroy(frame *f) {
   free(f);
}

