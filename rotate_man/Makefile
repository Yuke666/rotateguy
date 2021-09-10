CC=gcc

LFLAGS = 
LIBS = -lm
LIBS += $(shell pkg-config --static --libs sdl2)
LIBS += $(shell pkg-config --static --libs glew)

CFLAGS  = -Wall -Wextra

SOURCES=main.c log.c window.c game.c memory.c math.c audio.c font.c renderer.c \
		 mod.c level.c levels.c editor.c menu.c drum.c graphics.c

cDEFINES = -D MEMORY_DEBUG

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