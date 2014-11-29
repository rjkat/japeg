#include <stdint.h>
#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include "convert.h"
#include "jpeg.h"
#include "dct.h"
#include "jpeg_internal.h"
#include "bitmap.h"
#include "bitmap_internal.h"
#include "scan_start.h"

static float contribution(unsigned int bitmap_channel, component_id component, float value);
static int read_data_unit(const jpeg *j
                         ,component *c
                         ,int16_t chunk[JPEG_CHUNK_NUM_SAMPLES]);

static int convert_mcu(const jpeg *j, bitmap *b, int restart, int row, int col);
static void write_pixels_to_bitmap(float pixels[JPEG_CHUNK_SIDE_LENGTH][JPEG_CHUNK_SIDE_LENGTH]
                                  ,const jpeg *j
                                  ,component *component 
                                  ,unsigned int row
                                  ,unsigned int col
                                  ,unsigned int h
                                  ,unsigned int v
                                  ,bitmap *b);

typedef struct scale_s {
   float factor;
   float offset;
} scale;

bitmap *jpeg_to_bitmap(const jpeg *j) {
   jpeg_stream *stream;
   bitmap *b = malloc(sizeof(bitmap));
   int error = 0;
   int done = 0;
   unsigned int i;
   unsigned int row = 0;
   unsigned int col = 0;
   size_t mcus_read = 0;
   int restart = 0;
   assert(j);
   stream = j->scan_start->stream;
   b->num_cols = j->frame->samples_per_line;
   b->num_rows = j->frame->num_lines;
   for (i = 0; i < BITMAP_NUM_CHANNELS; i++) {
      b->samples[i] = calloc(b->num_rows * b->num_cols, sizeof(float));
   }
   while (!done && !error) {
      int state;
      if (restart) {
         jpeg_stream_restart(stream);
      }
      error = convert_mcu(j, b, restart, row, col);      
      col += j->frame->highest_sampling_factor * JPEG_CHUNK_SIDE_LENGTH;
      if (col >= b->num_cols) {
         col  = 0;
         row += j->frame->highest_sampling_factor * JPEG_CHUNK_SIDE_LENGTH;
      } 
      mcus_read += 1;
      restart = 0;
      if (j->has_restart_interval && mcus_read % j->restart_interval == 0) {
         restart = 1;
      }
      state = jpeg_stream_get_state(stream);
      if (state == JPEG_STREAM_STATE_OUT_OF_DATA) {
         printf("Error reading image.\n");
         error = 1;
      /* Check for EOI marker */
      } else if (state == JPEG_STREAM_STATE_EOI) {
         printf("Finished reading image.\n");
         done = 1;
      } 
   }
   return b;
}

static int convert_mcu(const jpeg *j, bitmap *b, int restart, int row, int col) {
   unsigned int c;
   int error = 0;
   for (c = 0; c < j->frame->num_components && !error; c++) {
      unsigned int v;
      component *component = &j->frame->components[c];
      if (restart) {
         component->prev_dc_coeff = 0;
      }
      for (v = 0; v < component->sampling_factor_vertical && !error; v++) {
         unsigned int h;
         for (h = 0; h < component->sampling_factor_horizontal && !error; h++) {
            int16_t chunk[JPEG_CHUNK_NUM_SAMPLES];
            float   pixels[JPEG_CHUNK_SIDE_LENGTH][JPEG_CHUNK_SIDE_LENGTH];
            error = read_data_unit(j, component, chunk);
            if (!error) {
               qtable_dequantise(j->qtables[component->qtable_id], chunk);
               dct_inverse(chunk, pixels);
               write_pixels_to_bitmap(pixels, j, component, row, col, h, v, b);
            }
         }
      }
   }
   return error;
}

static void write_pixels_to_bitmap(float pixels[JPEG_CHUNK_SIDE_LENGTH][JPEG_CHUNK_SIDE_LENGTH]
                                  ,const jpeg *j
                                  ,component *component 
                                  ,unsigned int row
                                  ,unsigned int col
                                  ,unsigned int h
                                  ,unsigned int v
                                  ,bitmap *b) {
   unsigned int n, m;
   float pixel;
   for (n = 0; n < (JPEG_CHUNK_SIDE_LENGTH * j->frame->highest_sampling_factor) 
                    / component->sampling_factor_horizontal; n++) {
      for (m = 0; m < (JPEG_CHUNK_SIDE_LENGTH * j->frame->highest_sampling_factor) 
                     / component->sampling_factor_vertical; m++) {
         pixel = pixels[n * component->sampling_factor_horizontal / j->frame->highest_sampling_factor]
                       [m * component->sampling_factor_vertical   / j->frame->highest_sampling_factor];
         unsigned int real_col = col + h * JPEG_CHUNK_SIDE_LENGTH;
         unsigned int real_row = row + v * JPEG_CHUNK_SIDE_LENGTH;
         if ((real_row + n) < b->num_rows && (real_col + m) < b->num_cols) {
            unsigned int channel;
            for (channel = 0; channel < BITMAP_NUM_CHANNELS; channel++) {
               b->samples[channel][(real_row + n) * b->num_cols + (real_col + m)] 
                  += contribution(channel, component->id, pixel);
            }
         } 
      }
   }
}

static int read_data_unit(const jpeg *j
                         ,component *c
                         ,int16_t chunk[JPEG_CHUNK_NUM_SAMPLES]) {
   int status;
   int error = 0;
   jpeg_stream *stream;
   size_t num_previous_zeros = 0;
   stream = j->scan_start->stream;
   /* Read DC coefficient */
   htable *table = htable_get_table(j->htables
                                   ,HTABLE_TYPE_DC
                                   ,c->dc_htable_id);
   int16_t dc_delta = 0;
   size_t sample = 0;
   status = htable_decode(stream, table, &dc_delta, &num_previous_zeros);
   if (status == HTABLE_OK || status == HTABLE_END_OF_BLOCK) {
      c->prev_dc_coeff += dc_delta;
      chunk[sample] = c->prev_dc_coeff;
      sample += 1;
      /* Read AC coefficients */
      status = HTABLE_OK;
      while (status == HTABLE_OK && sample < JPEG_CHUNK_NUM_SAMPLES) {
         int16_t ac_coeff;
         num_previous_zeros = 0;
         table = htable_get_table(j->htables
                                 ,HTABLE_TYPE_AC
                                 ,c->ac_htable_id);
         status = htable_decode(stream, table, &ac_coeff, &num_previous_zeros);
         if (status == HTABLE_OK) {
            unsigned int i;
            for (i = 0; i < num_previous_zeros; i++) {
               if ((sample + i) < JPEG_CHUNK_NUM_SAMPLES) {
                  chunk[sample + i] = 0;
               } else {
                  status = HTABLE_ERR_DECODE;
               }
            }
            if (status == HTABLE_OK) {
               sample += num_previous_zeros;
               chunk[sample] = ac_coeff;
               sample += 1;
            }
         } 
      }
      /* Fill in zeros at end of block */
      if (status == HTABLE_END_OF_BLOCK) {
         while (sample < JPEG_CHUNK_NUM_SAMPLES) {
            chunk[sample] = 0;
            sample += 1;
         }
      } else if (sample < JPEG_CHUNK_NUM_SAMPLES) {
         printf("Error during huffman decoding\n");
         error = 1;
      } 
   } else {
      printf("Error during huffman decoding\n");
      error = 1;
   }
   return error;
}


static float contribution(unsigned int bitmap_channel
                         ,component_id component
                         ,float value) {
   scale s;   
   scale scale_y[BITMAP_NUM_CHANNELS]  = {{ 1.0f,        0.0f}
                                         ,{ 1.0f,        0.0f}
                                         ,{ 1.0f,        0.0f}};
   scale scale_cb[BITMAP_NUM_CHANNELS] = {{ 1.772f,   -128.0f}
                                         ,{-0.34414f, -128.0f}
                                         ,{ 0.0f,        0.0f}}; 
   scale scale_cr[BITMAP_NUM_CHANNELS] = {{ 0.0f,        0.0f}
                                         ,{-0.71414f, -128.0f}
                                         ,{ 1.402f,   -128.0f}}; 
   switch (component) {
      case COMPONENT_ID_CB:
         s = scale_cb[bitmap_channel];
         break;
      case COMPONENT_ID_CR:
         s = scale_cr[bitmap_channel];
         break;
      case COMPONENT_ID_Y:
         s = scale_y[bitmap_channel];
         break;
      default:
      assert(0);
      break;
   }
   return (value + s.offset) * s.factor;
} 

