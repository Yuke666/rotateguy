#include <SDL2/SDL.h>
#include <string.h>
#include "game.h"
#include "graphics.h"
#include "window.h"

#ifndef DEFAULT_DISPLAY_FPS
#define DEFAULT_DISPLAY_FPS FALSE
#endif

#define TARGET_DT 33
#define MAX_DT 100

static void Events(Game_t *game){

    SDL_Event ev;
    while(SDL_PollEvent(&ev)){

        if(ev.type == SDL_QUIT)
            game->state = GAMESTATE_QUIT;

        Level_Event(&game->lvl, ev);
    }
}

static void Render(Game_t *game){
    (void)game;

    Level_Render(&game->lvl);
    Graphics_Render();
}

static void Update(Game_t *game){
    
    (void)game;
    Level_Update(&game->lvl, game->deltatime);
}

void Game_Run(void){

	Game_t game;
    memset(&game, 0, sizeof(Game_t));

    game.Update = Update;
    game.Events = Events;
    game.Render = Render;

	game.state = GAMESTATE_RUNNING;
    game.displayFps = DEFAULT_DISPLAY_FPS;

    u32 currTime;
	u32 lastTime = SDL_GetTicks();
    u32 frames = 0;
    u32 lastSecond = SDL_GetTicks();

    Level_Start(&game.lvl);

    // char fpsBuffer[32];

	while(game.state != GAMESTATE_QUIT){

        game.Events(&game);

        currTime = SDL_GetTicks();

        game.deltatime = currTime - lastTime;

        if(currTime - lastSecond > 1000){
            game.fps = frames;
            game.frameTime = (currTime - lastSecond) / (float)frames;
            lastSecond = currTime;
            frames = 0;

            printf("fps: %i | ms: %f\n", game.fps, game.frameTime);
        }

#ifdef UNCAPPED
        if(game.deltatime == 0)
            continue;
#endif

        ++frames;

        if(game.deltatime > MAX_DT)
            game.deltatime = TARGET_DT;


        lastTime = currTime;

        game.Update(&game);
        game.Render(&game);

        // if(game.displayFps == TRUE){
            // sprintf(fpsBuffer, "FPS: %i | MS: %f", game.fps, game.frameTime);
            // ...
        // }

		Window_Swap();
    }
}