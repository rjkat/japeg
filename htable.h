#ifndef HTABLE_H
#define HTABLE_H
#include <stdint.h>

typedef struct htable_s htable;
typedef uint8_t htable_id;

#include "frame.h"
#include "jpeg_segment.h"
#include "jpeg_internal.h"
#include "scan_start.h"
#include "jpeg_stream.h"
#define HTABLE_OK            0
#define HTABLE_END_OF_BLOCK  1
#define HTABLE_ERR_DECODE    2

typedef enum {
   HTABLE_TYPE_DC = 0,
   HTABLE_TYPE_AC = 1
} htable_type;


int     htable_create(const jpeg_segment *segment
                     ,htable *tables[JPEG_MAX_HTABLES]
                     ,size_t *num_htables);

int     htable_decode(jpeg_stream  *stream
                     ,htable       *table
                     ,int16_t      *result
                     ,size_t       *num_previous_zeros
                     );

htable *htable_get_table(htable *const htables[JPEG_MAX_HTABLES]
                        ,htable_type type
                        ,htable_id   id);

void    htable_destroy(htable *table);

#endif
