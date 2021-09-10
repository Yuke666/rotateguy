#include <stdio.h>
#include <string.h>
#include "memory.h"
#include "graphics.h"
#include "window.h"
#include "game.h"
#include "math.h"

int main(){

	int err;

	err = Window_Open(GAME_DEFAULT_WIDTH, GAME_DEFAULT_HEIGHT);

	if(err == ERROR)
		return 1;

	err = Memory_Init();

	if(err == ERROR){
		Window_Close();
		return 1;
	}

    Graphics_Init();

	Game_Run();

    Graphics_Close();

	Window_Close();

	Memory_Close();

	return 0;
}