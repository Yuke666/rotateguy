#include <stdio.h>
#include <string.h>
#include "game.h"
#include "types.h"
#include "memory.h"
#include "window.h"
#include "math.h"

int main(){

	int err;

	err = Window_Open(GAME_DEFAULT_WIDTH, GAME_DEFAULT_HEIGHT);

	if(err == WIN_ERR)
		return 1;

	err = Memory_Init();

	if(err == MEM_ERR){
		Window_Close();
		return 1;
	}

	Game_Run();

	Window_Close();

	Memory_Close();

	return 0;
}