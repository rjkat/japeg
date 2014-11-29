
#ifndef JPEG_STREAM_H
#define JPEG_STREAM_H

#include <stdint.h>
#include <stdlib.h>

/* There is more data to read */
#define JPEG_STREAM_STATE_MORE_DATA   0
/* End of image */
#define JPEG_STREAM_STATE_EOI         1
/* Data ran out before end of image */
#define JPEG_STREAM_STATE_OUT_OF_DATA 2

typedef struct jpeg_stream_s jpeg_stream;

jpeg_stream *jpeg_stream_create(size_t  data_size_bytes
                               ,uint8_t *data
                               );

void jpeg_stream_destroy(jpeg_stream *stream);

uint8_t jpeg_stream_get_next_bit(jpeg_stream *stream);

/* Return one of JPEG_STREAM_STATE_XXX */
int jpeg_stream_get_state(jpeg_stream *stream);

/* Handle restart marker */
void jpeg_stream_restart(jpeg_stream *stream);

#endif