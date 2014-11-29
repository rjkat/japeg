#ifndef BITMAP_INTERNAL_H
#define BITMAP_INTERNAL_H

#include "bitmap.h"
#include <stdlib.h>

#define BITMAP_CHANNEL_B          0
#define BITMAP_CHANNEL_G          1
#define BITMAP_CHANNEL_R          2
#define BITMAP_NUM_CHANNELS       3

#define BITMAP_BYTES_PER_CHANNEL  1
#define BITMAP_BYTES_PER_PIXEL   (BITMAP_BYTES_PER_CHANNEL * BITMAP_NUM_CHANNELS)

struct bitmap_s {
   size_t  num_rows;
   size_t  num_cols;
   float  *samples[BITMAP_NUM_CHANNELS];
};

#endif
