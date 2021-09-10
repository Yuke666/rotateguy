#ifndef WINDOW_DEF
#define WINDOW_DEF

#include "rotateguy.h"

#ifndef GAME_TITLE
#define GAME_TITLE "ROTATEGUY"
#endif

#define GAME_DEFAULT_WIDTH 	(WIDTH)
#define GAME_DEFAULT_HEIGHT (HEIGHT)
#define WIN_ERR 			0
#define WIN_OK 				1

int 	Window_Open	 (u32 w, u32 h);
void 	Window_Swap	 (void);
void 	Window_Close (void);

#endif