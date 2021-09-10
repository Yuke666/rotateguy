#ifndef GAME_DEF
#define GAME_DEF

#include "renderer.h"
#include "types.h"
#include "level.h"
#include "audio.h"
#include "editor.h"
#include "menu.h"

enum {
	GAMESTATE_QUIT = 0x00,
	GAMESTATE_RUNNING,
	GAMESTATE_MENU,
	GAMESTATE_EDITOR,
	GAMESTATE_PLAYING,
};

typedef struct Game_t Game_t;

struct Game_t {
	u8						state;
	u8						displayFps;
	u16						viewportWidth;
	u16						viewportHeight;
	u16						windowWidth;
	u16						windowHeight;
	u16						fps;
	u32 					lastMilkdropDraw;
	float					frameTime;
	s32 					deltatime;
	void 					(*Update)(Game_t *this);
	void 					(*Render)(Game_t *this);
	void 					(*Events)(Game_t *this);
	AudioThread_t			audioThread;
	Renderer_t			 	renderer;
	MODFile_t				modFile;
	union {
		Editor_t 				editor;
		Menu_t					menu;
	};
};

void Game_Run(void);

#endif