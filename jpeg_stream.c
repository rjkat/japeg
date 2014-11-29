#include "jpeg_stream.h"
#include "jpeg_internal.h"
#include <assert.h>
#include <stdlib.h>

static void advance_one_byte(jpeg_stream *stream);

struct jpeg_stream_s {
   uint8_t *data;
   size_t   data_size_bytes;  
   size_t   bytes_read;
   size_t   bit_offset;
};

jpeg_stream *jpeg_stream_create(size_t  data_size_bytes
                               ,uint8_t *data
                               ) {
   jpeg_stream *s = malloc(sizeof(jpeg_stream));
   assert(s);
   s->data_size_bytes = data_size_bytes;
   s->data = data;
   s->bytes_read = 0;
   s->bit_offset = 0;
   return s;
}

uint8_t jpeg_stream_get_next_bit(jpeg_stream *stream) {
   uint8_t next_bit = !!(stream->data[stream->bytes_read] & (1 << (7 - stream->bit_offset)));
   stream->bit_offset += 1;
   if (stream->bit_offset == 8) {
      advance_one_byte(stream);
   }
   return next_bit;
}

void jpeg_stream_destroy(jpeg_stream *stream) {
   free(stream);
}

int jpeg_stream_get_state(jpeg_stream *stream) {
   int ret = JPEG_STREAM_STATE_MORE_DATA;
   if (stream->bytes_read >= stream->data_size_bytes) {
      ret = JPEG_STREAM_STATE_OUT_OF_DATA;
      /* Check for EOI marker */
   } else if ((stream->bytes_read + 2) < stream->data_size_bytes
            && stream->data[stream->bytes_read + 1] == JPEG_MARKER_MAGIC_BYTE
            && stream->data[stream->bytes_read + 2] == JPEG_MARKER_EOI) {
      ret = JPEG_STREAM_STATE_EOI;
   } 
   return ret;
}

void jpeg_stream_restart(jpeg_stream *stream) {
   /* If current byte is 0xFF then still need to skip stuff bytes */
   if (stream->bit_offset != 0) {
      advance_one_byte(stream);
   }
   /* Don't need to skip stuff bytes since we are reading a marker */
   assert(stream->bytes_read + 1 < stream->data_size_bytes);
   /* Assert that we actually see a restart marker */
   assert(stream->data[stream->bytes_read] == 0xFF);
   stream->bytes_read += 1;
   assert((stream->data[stream->bytes_read] & 0xF0) == 0xD0);
   stream->bytes_read += 1;
}

static void advance_one_byte(jpeg_stream *stream) {
   /* Check for stuff bytes */
   if (stream->data[stream->bytes_read] == 0xFF) {
      stream->bytes_read += 1;
      /* Not sure what to do with a marker in the middle of the huffman stream,
       * just assert for now */
      assert(stream->data[stream->bytes_read] == 0x00);
   }
   stream->bytes_read += 1;
   stream->bit_offset = 0;
}



