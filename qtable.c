#include <assert.h>
#include <stdio.h>
#include "jpeg_internal.h"
#include "qtable.h"
#include "jpeg_segment.h"

/* 4 bits for precision, 4 bits for ID */
#define QTABLE_METADATA_LENGTH_BYTES    1
#define QTABLE_DATA_LENGTH_8BIT        (JPEG_CHUNK_NUM_SAMPLES)
#define QTABLE_DATA_LENGTH_16BIT       (2 * JPEG_CHUNK_NUM_SAMPLES)
#define QTABLE_MAX_ID                   3

typedef enum {
   QTABLE_PRECISION_8BIT  = 0,
   QTABLE_PRECISION_16BIT = 1
} qtable_precision;

struct qtable_s {
   qtable_precision precision;
   unsigned int id;
   unsigned int values[JPEG_CHUNK_NUM_SAMPLES];
   size_t zigzag[JPEG_CHUNK_NUM_SAMPLES];
};

static size_t triangle(size_t n) {
   return (n * (n + 1)) / 2;
}

/* http://stackoverflow.com/questions/15201395/zig-zag-scan-an-n-x-n-array */
static void fill_elements(size_t zigzag[JPEG_CHUNK_NUM_SAMPLES], int is_odd) {
   int x, y;
   int n = JPEG_CHUNK_SIDE_LENGTH;
   for (y = 0; y < n; y++) {
      for (x = n - (y + 1 + is_odd); x >= 0; x -= 2) {
         zigzag[x * n + y] = triangle(x + y + 1) - (is_odd ? x : y) - 1;
         zigzag[n * (n - x - 1) + (n - y - 1)] = JPEG_CHUNK_NUM_SAMPLES - zigzag[x * n + y] - 1;
      }
   }
}

static void create_zigzag(size_t zigzag[JPEG_CHUNK_NUM_SAMPLES]) {
   fill_elements(zigzag, 0);
   fill_elements(zigzag, 1);
}

static qtable *qtable_create_internal(unsigned char *block_data
                                     ,size_t  *bytes_remaining) {
   qtable *table;
   unsigned char precision;
   unsigned char table_id;
   assert(block_data);
   assert(bytes_remaining);
   table_id = block_data[0] & 0xF;
   if (table_id > QTABLE_MAX_ID) {
      return NULL;
   }
   precision = (block_data[0] >> 4) & 0xF;
   if (  precision != QTABLE_PRECISION_8BIT 
      && precision != QTABLE_PRECISION_16BIT) {
      return NULL;
   }
   *bytes_remaining = *bytes_remaining - 1;
   if (precision == QTABLE_PRECISION_16BIT) {
      if (*bytes_remaining < QTABLE_DATA_LENGTH_16BIT) {
         return NULL;
      } else {
         *bytes_remaining = *bytes_remaining - QTABLE_DATA_LENGTH_16BIT;
      }
   } else {
      if (*bytes_remaining < QTABLE_DATA_LENGTH_8BIT) {
         return NULL;
      } else {
         *bytes_remaining = *bytes_remaining - QTABLE_DATA_LENGTH_8BIT;
      }
   }
   table = malloc(sizeof(struct qtable_s));
   assert(table);
   table->precision = precision;
   table->id        = table_id;
   create_zigzag(table->zigzag);
   size_t i;
   for (i = 0; i < JPEG_CHUNK_NUM_SAMPLES; i++) {
      if (table->precision == QTABLE_PRECISION_8BIT) {
         table->values[i] = block_data[QTABLE_METADATA_LENGTH_BYTES + i];
      } else {
         table->values[i] = read_word(&block_data[QTABLE_METADATA_LENGTH_BYTES + 2*i]);
      }
   }
   return table;
}

void qtable_dequantise(qtable *table, int chunk[JPEG_CHUNK_NUM_SAMPLES]) {
   size_t i;
   int chunk_copy[JPEG_CHUNK_NUM_SAMPLES];
   for (i = 0; i < JPEG_CHUNK_NUM_SAMPLES; i++) {
      chunk_copy[i] = chunk[i] * table->values[i];      
   } 
   for (i = 0; i < JPEG_CHUNK_NUM_SAMPLES; i++) {
      chunk[i] = chunk_copy[table->zigzag[i]];
   }
}

int qtable_create(const jpeg_segment *segment
                  ,qtable *tables[JPEG_MAX_QTABLES]
                  ,size_t *num_qtables) {
   size_t offset = 0;
   size_t bytes_remaining;
   int    error = 0;
   assert(segment);
   assert(segment->marker == JPEG_MARKER_DQT);
   assert(num_qtables);
   bytes_remaining = segment->data_size;
   while (bytes_remaining > 0 && *num_qtables < JPEG_MAX_QTABLES && !error) {
      tables[*num_qtables] = qtable_create_internal(segment->data + offset
                                                   ,&bytes_remaining);
      if (!tables[*num_qtables]) {
         error = 1;
      } else {
         offset = segment->data_size - bytes_remaining;
        *num_qtables = *num_qtables + 1;
      }
   }
   return error;
}

void qtable_destroy(qtable *table) {
   free(table);
}

unsigned int qtable_get(qtable *table, unsigned int pos) {
   assert(table);
   assert(pos < JPEG_CHUNK_NUM_SAMPLES);
   return table->values[pos];
}
