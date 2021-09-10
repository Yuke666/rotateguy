#ifndef LEVEL_DEF
#define LEVEL_DEF

#include <SDL2/SDL_events.h>
#include "rotateguy.h"
#include "character.h"

enum {
	ROTATE_CCW = 0,
	ROTATE_CW,
};

typedef struct Level_t Level_t;

struct Level_t {

	const u8		*lvl;
	u16				lvlWidth;
	u16				lvlHeight;

	u8 				rotating[2];

	Character_t		player;
};

void Level_Render(Level_t *lvl);
void Level_Update(Level_t *lvl, float dt);
void Level_Event(Level_t *lvl, SDL_Event event);
void Level_Start(Level_t *lvl);

#endif