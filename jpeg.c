#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "bitmap_internal.h"
#include "jpeg.h"
#include "jpeg_internal.h"
#include "jpeg_segment.h"
#include "qtable.h"
#include "htable.h"
#include "frame.h"
#include "scan_start.h"

static unsigned char *read_file(const char *filename
                               ,size_t     *file_size_bytes);

static jpeg_segment *read_next_segment(jpeg   *j
                                      ,size_t *offset);

jpeg *jpeg_read(const char *filename) {
   jpeg *j = malloc(sizeof(jpeg));
   assert(j);
   j->data = (unsigned char *) read_file(filename, &j->data_size);
   j->num_qtables = 0;
   j->num_htables = 0;
   unsigned int i;
   for (i = 0; i < JPEG_MAX_HTABLES; i++) {
      j->htables[i] = NULL;
   }
   j->scan_start = NULL;
   j->frame = NULL;
   j->has_restart_interval = 0;
   j->restart_interval = 0;
   if (!j->data) {
      jpeg_destroy(j);
      return NULL;
   }
   if (  j->data_size < 6
      || j->data[0] != JPEG_MARKER_MAGIC_BYTE
      || j->data[1] != JPEG_HEADER_MAGIC_1
      || j->data[2] != JPEG_MARKER_MAGIC_BYTE
      || j->data[3] != JPEG_HEADER_MAGIC_2) {
      printf("File is not a JPEG file\n");
      jpeg_destroy(j);
      return NULL;
   }
   /* Skip header */
   size_t header_length = read_word(&j->data[4]);
   size_t offset = header_length;
   unsigned char marker = '\0';
   while (offset < j->data_size && marker != JPEG_MARKER_SOS) {
      jpeg_segment *segment = read_next_segment(j, &offset);
      if (segment) {
         switch (segment->marker) {
            case JPEG_MARKER_DQT: {
               int result = qtable_create(segment, j->qtables, &j->num_qtables);
               if (result != 0) {
                  printf("Unable to parse DQT segment\n");
               }
               break;
            }
            case JPEG_MARKER_SOF: {
               if (j->frame != NULL) {
                  printf("Extra SOF segment\n");
               } else {
                  j->frame = frame_create(segment);
                  if (!j->frame) {
                     printf("Unable to parse SOF segment\n");
                  }
               }
               break;
            }
            case JPEG_MARKER_DHT: {
               if (j->num_htables == JPEG_MAX_HTABLES) {
                  printf("Extra htable\n");
               } else {
                  int result = htable_create(segment, j->htables, &j->num_htables);
                  if (result != 0) {
                     printf("Unable to parse DHT segment\n");
                  }
               }
               break;
            }
            case JPEG_MARKER_SOS: {
               if (!j->frame) {
                  printf("Start of scan encountered before start of frame\n");
               } else {
                  size_t segment_start = offset - segment->data_size;
                  j->scan_start = scan_start_create(segment
                                                   ,j->frame
                                                   ,j->data_size - segment_start
                                                   );
                  if (!j->scan_start) {
                     printf("Unable to parse SOS segment\n");
                  }
               }
               break;
            }
            case JPEG_MARKER_DRI: {
               if (segment->data_size != 2) {
                  printf("Invalid DRI segment");
                  assert(0);
               }
               j->has_restart_interval = 1;
               j->restart_interval = read_word(segment->data);
               assert(j->restart_interval > 0);
               break;
            }
            default: {
               /* Ignore unknown segment types as per standard */
               printf("Unknown marker %02x, ignoring\n", segment->marker);
               break;
            }
         }
         marker = segment->marker;
         jpeg_segment_destroy(segment);
      }
   }
   return j;
}

void jpeg_destroy(jpeg *j) {
   if (j) {
      size_t i;
      for (i = 0; i < j->num_qtables; i++) {
         qtable_destroy(j->qtables[i]);
      }
      for (i = 0; i < j->num_htables; i++) {
         htable_destroy(j->htables[i]);
      }
      frame_destroy(j->frame);
      scan_start_destroy(j->scan_start);
      free(j->data);
   }
   free(j);
}

static unsigned char *read_file(const char *filename
                               ,size_t     *out_file_size) {
   FILE          *fp = fopen(filename, "rb");
   unsigned char *data;
   size_t         file_size; 
   if (!fp) {
      perror("Error opening file");
      return NULL;
   }
   fseek(fp, 0L, SEEK_END);
   file_size = ftell(fp);
   fseek(fp, 0L, SEEK_SET);
   data = malloc(file_size);
   assert(data);
   size_t bytes_read = fread(data
                            ,sizeof(unsigned char)
                            ,file_size
                            ,fp);
   if (bytes_read != file_size) {
      printf("Error reading data from file\n");
      return NULL;
   }
   if (out_file_size) {
      *out_file_size = file_size;
   }
   return data;
}

static jpeg_segment *read_next_segment(jpeg   *j
                                      ,size_t *offset) {
   jpeg_segment *segment = NULL;
   unsigned char marker = '\0';
   size_t i;
   assert(j);
   assert(j->data);
   assert(offset);
   i = *offset;
   while (i < j->data_size && segment == NULL) {
      if (j->data[i] == JPEG_MARKER_MAGIC_BYTE && (i + 1) < j->data_size) {
         i++;
         marker = j->data[i];
         i++;
         if ((i + JPEG_SEGMENT_SIZE_LENGTH_BYTES) < j->data_size) {
            size_t segment_size = read_word(j->data + i);
            /* Subtract the number of bytes of the length field from the length
             * of the segment. If length of the segment says it is shorter than its own length field,
             * something weird is going on. */
            if (segment_size >= JPEG_SEGMENT_SIZE_LENGTH_BYTES) {
               segment_size -= JPEG_SEGMENT_SIZE_LENGTH_BYTES;
            } else {
               jpeg_segment_destroy(segment);
               return NULL;
            }
            i += JPEG_SEGMENT_SIZE_LENGTH_BYTES;
            segment = jpeg_segment_create(marker
                                         ,j->data + i
                                         ,segment_size
                                         );
            /* Skip the rest of the segment */
            i += segment->data_size;
         } 
      } else {
         i++;
      }
   }
   *offset = i;
   return segment;
}

