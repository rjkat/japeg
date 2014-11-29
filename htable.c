#include <assert.h>
#include <stdio.h>
#include "htable.h"
#include "htree.h"
#include "scan_start.h"

#define HTABLE_METADATA_LENGTH_BYTES 1
#define HTABLE_MAX_STRING_BITS       16
#define HTABLE_CODE_BITS             8
#define HTABLE_MIN_LENGTH_BYTES     (HTABLE_METADATA_LENGTH_BYTES + HTABLE_MAX_STRING_BITS)

static int16_t htable_read_bitstream_value(jpeg_stream *stream, size_t total_bits, htable_type type);

struct htable_s {
   htable_type type;
   htable_id   id;
   htree      *tree;
   size_t      num_bit_codes[HTABLE_MAX_STRING_BITS];
   uint8_t    *bit_codes[HTABLE_MAX_STRING_BITS];
};

htable *htable_get_table(htable *const htables[JPEG_MAX_HTABLES]
                        ,htable_type   type
                        ,htable_id     id) {
   unsigned int i;
   htable *h = NULL;
   for (i = 0; i < JPEG_MAX_HTABLES; i++) {
      if (   htables[i]       != NULL
          && htables[i]->type == type
          && htables[i]->id   == id) {
         h = htables[i];
      }
   }
   return h;
}

htable *htable_create_internal(uint8_t *data
                              ,size_t  *bytes_remaining) {
   size_t   i = 0;
   htable  *table;
   assert(data);
   assert(bytes_remaining);
   if (*bytes_remaining < HTABLE_MIN_LENGTH_BYTES) {
      return NULL;
   }
   table = malloc(sizeof(htable));
   assert(table);
   table->type = (data[i] >> 4) & 0xF;
   table->id   =  data[i]       & 0xF;
   i += 1;
   *bytes_remaining -= 1;
   if (table->type != HTABLE_TYPE_AC && table->type != HTABLE_TYPE_DC) {
      free(table);
      return NULL;
   }
   size_t n;
   size_t total_code_bytes = 0;
   for (n = 0; n < HTABLE_MAX_STRING_BITS; n++) {
      table->num_bit_codes[n] = data[i];
      if (table->num_bit_codes[n] > 0) {
         table->bit_codes[n] = malloc(table->num_bit_codes[n] * sizeof(uint8_t));
         assert(table->bit_codes[n]);
         total_code_bytes += table->num_bit_codes[n];
      } else {
         table->bit_codes[n] = NULL;
      }
      i += 1;
      *bytes_remaining -= 1;
   }
   if (total_code_bytes > *bytes_remaining) {
      free(table);
      return NULL;
   }
   for (n = 0; n < HTABLE_MAX_STRING_BITS; n++) {
      size_t m;
      for (m = 0; m < table->num_bit_codes[n]; m++) {
         table->bit_codes[n][m] = data[i];
         i += 1;
         *bytes_remaining -= 1;
      }
   }
   table->tree = htree_create(HTABLE_MAX_STRING_BITS
                             ,HTABLE_CODE_BITS
                             ,table->num_bit_codes
                             ,table->bit_codes);
   
   return table;
}

int htable_create(const jpeg_segment *segment
                 ,htable *tables[JPEG_MAX_HTABLES]
                 ,size_t *num_htables) {
   size_t offset = 0;
   size_t bytes_remaining;
   int    error = 0;
   assert(segment);
   assert(segment->marker == JPEG_MARKER_DHT);
   assert(num_htables);
   bytes_remaining = segment->data_size;
   while (bytes_remaining > 0 && *num_htables < JPEG_MAX_QTABLES && !error) {
      tables[*num_htables] = htable_create_internal(segment->data + offset
                                                   ,&bytes_remaining);
      if (!tables[*num_htables]) {
         error = 1;
      } else {
         offset = segment->data_size - bytes_remaining;
        *num_htables += 1;
      }
   }
   return error;
}

void htable_destroy(htable *table) {
   if (table) {
      size_t n;
      htree_destroy(table->tree);
      for (n = 0; n < HTABLE_MAX_STRING_BITS; n++) {
         free(table->bit_codes[n]);
      }
      free(table);
   }
}

static int16_t htable_read_bitstream_value(jpeg_stream *stream
                                          ,size_t total_bits
                                          ,htable_type type) {
   int32_t value = 0;
   size_t bits_read = 0;
   while (bits_read < total_bits) {
      value |= jpeg_stream_get_next_bit(stream) << (total_bits - bits_read - 1);
      bits_read += 1;
   }
   int16_t result = (int16_t) value;
   if (total_bits > 0 && !(value & (1 << (total_bits - 1)))) {
      result -= (1 << total_bits) - 1;
   }
   return result;
}

int htable_decode(jpeg_stream  *stream
                 ,htable       *table
                 ,int16_t      *result
                 ,size_t       *num_previous_zeros
                 ) {
   int     status;
   uint8_t code;
   *num_previous_zeros = 0;
   status = htree_get(table->tree
                     ,stream
                     ,&code);
   if (status != 0) {
      return HTABLE_ERR_DECODE;
   }
   if (code == 0) {
      return HTABLE_END_OF_BLOCK;
   }
   size_t total_bits;
   if (table->type == HTABLE_TYPE_DC) {
      total_bits = code;
      *result    = htable_read_bitstream_value(stream, total_bits, table->type);
   } else {
      total_bits          =  code & 0x0F;
      *num_previous_zeros = (code & 0xF0) >> 4;
      *result             = htable_read_bitstream_value(stream, total_bits, table->type);
   }
   return HTABLE_OK;
}
