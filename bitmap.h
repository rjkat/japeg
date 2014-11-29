#ifndef BITMAP_H
#define BITMAP_H

typedef struct bitmap_s bitmap;

int  bitmap_write(bitmap *b, const char *filename);

void bitmap_destroy(bitmap *b);

#endif
