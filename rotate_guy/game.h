#ifndef GAME_DEF
#define GAME_DEF

#include "rotateguy.h"
#include "level.h"

enum {
	GAMESTATE_QUIT = 0x00,
	GAMESTATE_RUNNING,
};

typedef struct Game_t Game_t;

struct Game_t {
	u8						state;
	u8						displayFps;
	u16						fps;
	float					frameTime;
	s32 					deltatime;
	void 					(*Update)(Game_t *this);
	void 					(*Render)(Game_t *this);
	void 					(*Events)(Game_t *this);
	union {
		Level_t 			lvl;
	};
};

void Game_Run(void);

#endif