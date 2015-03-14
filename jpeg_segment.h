#ifndef JPEG_SEGMENT_H
#define JPEG_SEGMENT_H
#include <stdlib.h>

#define JPEG_SEGMENT_SIZE_LENGTH_BYTES 2

#define JPEG_MARKER_LENGTH_BYTES       2

#define JPEG_MARKER_MAGIC_BYTE         0xFF

#define JPEG_HEADER_MAGIC_1            0xD8
#define JPEG_HEADER_MAGIC_2            0xE0

#define JPEG_MARKER_SOI                0xD8
#define JPEG_MARKER_DQT                0xDB
#define JPEG_MARKER_DHT                0xC4
#define JPEG_MARKER_SOF                0xC0
#define JPEG_MARKER_SOS                0xDA
#define JPEG_MARKER_COMMENT            0xFE
#define JPEG_MARKER_EOI                0xD9
#define JPEG_MARKER_DRI                0xDD

typedef struct jpeg_segment_s {
   unsigned char  marker;
   unsigned char *data;
   size_t         data_size;
} jpeg_segment;

/* Read a 2 byte word from a big-endian buffer */
unsigned int  read_word(unsigned char *buf);

jpeg_segment *jpeg_segment_create(unsigned char  marker
                                 ,unsigned char *data
                                 ,size_t   data_size);

void          jpeg_segment_destroy(jpeg_segment *segment);

#endif
