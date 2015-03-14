#ifndef FRAME_H
#define FRAME_H

typedef struct frame_s frame;
#include "htable.h"
#include "qtable.h"
#include "jpeg_segment.h"

#define FRAME_MAX_COMPONENTS 255


typedef enum {
   COMPONENT_ID_Y  = 1,
   COMPONENT_ID_CB = 2,
   COMPONENT_ID_CR = 3
} component_id;

#define NUM_COMPONENTS 3

typedef struct component_s {
   component_id id;
   unsigned int sampling_factor_horizontal;
   unsigned int sampling_factor_vertical;
   qtable_id    qtable_id;
   htable_id    dc_htable_id;
   htable_id    ac_htable_id;
   int          prev_dc_coeff;
} component;

struct frame_s {
   unsigned int precision_bits;
   unsigned int num_lines;
   unsigned int samples_per_line;
   unsigned int num_components;
   unsigned int highest_sampling_factor;
   component components[FRAME_MAX_COMPONENTS];
};

frame     *frame_create(const jpeg_segment *segment);
component *frame_get_component_with_id(frame *f, component_id id);
void       frame_destroy(frame *f);

#endif
