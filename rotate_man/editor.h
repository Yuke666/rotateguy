#ifndef EDITOR_DEF
#define EDITOR_DEF

#include "level.h"

typedef struct {
	Level_t 	lvl;
	u32 		mousex;
	u32 		mousey;
} Editor_t;

typedef struct Game_t Game_t;

void Editor_Start(Game_t *game);

#endif