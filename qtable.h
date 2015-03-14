#ifndef QTABLE_H
#define QTABLE_H

typedef struct qtable_s qtable;
typedef unsigned int qtable_id;

#include "jpeg.h"
#include "jpeg_internal.h"
#include "jpeg_segment.h"

int qtable_create(const jpeg_segment *segment
                 ,qtable *tables[JPEG_MAX_QTABLES]
                 ,size_t *num_qtables);

void qtable_destroy(qtable *table);

unsigned int qtable_get(qtable *table, unsigned int pos);

void qtable_dequantise(qtable *table, int chunk[JPEG_CHUNK_NUM_SAMPLES]);

#endif
