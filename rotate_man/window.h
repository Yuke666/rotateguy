#ifndef WINDOW_DEF
#define WINDOW_DEF

#include "types.h"

#ifndef GAME_TITLE
#define GAME_TITLE "LSD"
#endif

#define GAME_DEFAULT_WIDTH 	800
#define GAME_DEFAULT_HEIGHT 600
#define VIEWPORT_WIDTH 		320
#define VIEWPORT_HEIGHT 	240
#define WIN_ERR 			0
#define WIN_OK 				1


int Window_Open		(u32 w, u32 h);
void Window_Swap	(void);
void Window_Close	(void);

#endif