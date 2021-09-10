#ifndef MENU_DEF
#define MENU_DEF

#include "types.h"
#include "renderer.h"

typedef struct {
	TextRenderer_t 	text;
	u32 			logoImage;
} Menu_t;

typedef struct Game_t Game_t;

void Menu_Start(Game_t *game);

#endif