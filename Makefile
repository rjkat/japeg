platform=$(shell uname -a)

ifeq ($(platform),GNU/Linux)
	CC=gcc
	CFLAGS=-c -Wall -g --std=c99
	LDFLAGS=
else
	CC=xcrun clang
	CFLAGS=-c -Wall -g
	LDFLAGS=
endif

SOURCES=bitmap.c convert.c dct.c frame.c htable.c htree.c jpeg.c \
        jpeg_segment.c jpeg_stream.c qtable.c scan_start.c
OBJECTS=$(SOURCES:.c=.o)
FRONTEND=japeg_frontend
UNITTEST=test_japeg

all: $(SOURCES) $(FRONTEND) $(UNITTEST)

clean:
	rm *.o japeg_frontend

$(FRONTEND): $(OBJECTS) main.o
	$(CC) $(LDFLAGS) $(OBJECTS) main.o -o $@ -lm

$(UNITTEST): $(OBJECTS) test.o
	$(CC) $(LDFLAGS) $(OBJECTS) test.o -o $@ -lm

.c.o:
	$(CC) $(CFLAGS) $< -o $@
