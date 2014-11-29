#ifndef DCT_H
#define DCT_H

#include "jpeg_internal.h"

void dct_inverse(int16_t chunk [JPEG_CHUNK_NUM_SAMPLES]
                ,float   pixels[JPEG_CHUNK_SIDE_LENGTH][JPEG_CHUNK_SIDE_LENGTH]);

#endif
