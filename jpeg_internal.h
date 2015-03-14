#ifndef JPEG_INTERNAL_H
#define JPEG_INTERNAL_H

#ifndef M_PI
#define M_PI 3.14159265358979323846264338327
#endif

#define JPEG_MAX_QTABLES 4
#define JPEG_MAX_HTABLES 4

#define JPEG_CHUNK_SIDE_LENGTH  8
#define JPEG_CHUNK_NUM_SAMPLES (JPEG_CHUNK_SIDE_LENGTH * JPEG_CHUNK_SIDE_LENGTH)

#include "jpeg.h"
#include "qtable.h"
#include "htable.h"
#include "frame.h"
#include "scan_start.h"

struct jpeg_s {
   unsigned char *data;
   size_t         data_size;

   qtable *qtables[JPEG_MAX_QTABLES];
   size_t  num_qtables;

   frame *frame;

   htable *htables[JPEG_MAX_HTABLES];
   size_t  num_htables;

   scan_start *scan_start;
   int         has_restart_interval;
   size_t      restart_interval;
};

#endif
