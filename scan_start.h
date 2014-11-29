#ifndef SCAN_H
#define SCAN_H
#include <stdint.h>
#include <stdlib.h>

typedef struct scan_start_s scan_start;

#include "jpeg_internal.h"
#include "jpeg_segment.h"
#include "jpeg_stream.h"
#include "frame.h"

struct scan_start_s {
   uint8_t      spectral_selection_start;
   uint8_t      spectral_selection_end;
   uint8_t      successive_approx_high;
   uint8_t      successive_approx_low;
   jpeg_stream *stream;  
};

scan_start *scan_start_create(const jpeg_segment *segment
                             ,frame              *f
                             ,size_t              file_bytes_remaining
                             );

void scan_start_destroy(scan_start *s);

#endif
