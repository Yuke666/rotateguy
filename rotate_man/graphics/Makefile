CC=gcc

LIBS = -lm
LIBS += $(shell pkg-config --static --libs libpng)

CFLAGS  = -Wall -Wextra

SOURCES=main.c

all: main.c
	$(CC) $(SOURCES) $(CFLAGS) $(LIBS) -o graphics-converter