CC=gcc

LFLAGS = 
LIBS = -lm
LIBS += $(shell pkg-config --static --libs sdl2)
LIBS += $(shell pkg-config --static --libs glew)
LIBS += $(shell pkg-config --static --libs libpng)

CFLAGS  = -Wall -Wextra

SOURCES=main.c window.c log.c memory.c image_loader.c graphics.c game.c \
		math.c character.c world.c mod.c audio.c

OUTPUT = output

OBJECTS=$(SOURCES:%.c=$(OUTPUT)/%.o)
EXECUTABLE=main

all: $(OUTPUT) $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS)
	$(CC) $(OBJECTS) $(LFLAGS) $(LIBS) -o $@

$(OUTPUT):
	@[ -d $(OUTPUT) ] || mkdir -p $(OUTPUT)


$(OUTPUT)/%.o: %.c
	$(CC) -c $(CFLAGS) $(DEFINES) $< -o $@

clean:
	rm -rf $(OUTPUT)