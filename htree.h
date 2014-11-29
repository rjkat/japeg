#ifndef HTREE_H
#define HTREE_H
#include <stdint.h>
#include <stdlib.h>
#include "jpeg_stream.h"

typedef struct htree_s htree;

/* num_codes[i] contains how many codes there are with length i */
/* codes[i] is the sequence of codes of length i */
htree         *htree_create
                         (size_t          max_string_bits
                         ,size_t          code_bits
                         ,const size_t   *num_codes
                         ,uint8_t       **codes);

void           htree_destroy(htree *tree);

/* Returns 0 if the lookup succeeded, 1 otherwise */
int            htree_get(const htree   *tree
                        ,jpeg_stream   *stream
                        ,uint8_t       *code
                        );

#endif
