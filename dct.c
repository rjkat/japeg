#include "dct.h"
#include "jpeg_internal.h"

#include <math.h>
#include <stdio.h>

void dct_inverse(int   chunk [JPEG_CHUNK_NUM_SAMPLES]
                ,float pixels[JPEG_CHUNK_SIDE_LENGTH][JPEG_CHUNK_SIDE_LENGTH]) {
   unsigned int x, y;
   /* Super naive and inefficient version for the moment */
   for (x = 0; x < JPEG_CHUNK_SIDE_LENGTH; x++) {
      for (y = 0; y < JPEG_CHUNK_SIDE_LENGTH; y++) {
         unsigned int u, v;
         pixels[x][y] = 0.0f;
         for (u = 0; u < JPEG_CHUNK_SIDE_LENGTH; u++) {
            float u_scaling_factor = 1.0f;
            if (u == 0) {
               u_scaling_factor = 1.0f / sqrt(2);
            }
            for (v = 0; v < JPEG_CHUNK_SIDE_LENGTH; v++) {
               float v_scaling_factor = 1.0f;
               if (v == 0) {
                  v_scaling_factor = 1.0f / sqrt(2);
               }
               pixels[x][y] += (float) chunk[u * JPEG_CHUNK_SIDE_LENGTH + v]
                              * cosf((2.0f * (float) x + 1.0f) / 16.0f * (float) u * M_PI)
                              * cosf((2.0f * (float) y + 1.0f) / 16.0f * (float) v * M_PI)
                              * u_scaling_factor
                              * v_scaling_factor;
            }
         }
         /* Do level shift */
         pixels[x][y] *= 0.25f;
         pixels[x][y] += 128.0f;
      }
   }
}
