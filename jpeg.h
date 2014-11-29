#ifndef JPEG_H
#define JPEG_H

typedef struct jpeg_s jpeg;

jpeg *jpeg_read(const char *filename);
void  jpeg_destroy(jpeg *j);

#endif
