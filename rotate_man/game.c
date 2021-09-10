#include <SDL2/SDL.h>
#include <string.h>
#include "game.h"
#include "window.h"

// u32 x, u32 y, u8 spacing, u8 size, u8 r, u8 g, u8 b
#define GAME_DISPLAY_FPS_SETTINGS(str) (TextLine_t){6, 6, 0, 4, 0, 0, 255, str}

#ifndef DEFAULT_DISPLAY_FPS
#define DEFAULT_DISPLAY_FPS FALSE
#endif

#define TARGET_DT 33
#define MAX_DT 100

void Game_Run(void){

	Game_t game;
    memset(&game, 0, sizeof(Game_t));

    // init threads

    Audio_Init(&game.audioThread);
        
    game.windowWidth = GAME_DEFAULT_WIDTH; 
    game.windowHeight = GAME_DEFAULT_HEIGHT; 
    game.viewportWidth = VIEWPORT_WIDTH;
    game.viewportHeight = VIEWPORT_HEIGHT;
    
    Renderer_Init(&game.renderer, VIEWPORT_WIDTH, VIEWPORT_HEIGHT);

    /// load song

    if(MODFile_Load(&game.modFile, "resources/7thheaven.mod"))
        Audio_PlayMOD(&game.audioThread, &game.modFile);

    // extern const s8 drum[];
    // Audio_Play(&game.audioThread, drum, 2300, MAX_AUDIO_VOLUME, 100);

    // main loop	

    Editor_Start(&game);

	game.state = GAMESTATE_RUNNING;
    game.displayFps = DEFAULT_DISPLAY_FPS;

    u32 currTime;
	u32 lastTime = SDL_GetTicks();
    u32 frames = 0;
    u32 lastSecond = SDL_GetTicks();

    char fpsBuffer[32];

	while(game.state != GAMESTATE_QUIT){

        game.Events(&game);

        currTime = SDL_GetTicks();

        game.deltatime = currTime - lastTime;

        if(currTime - lastSecond > 1000){
            game.fps = frames;
            game.frameTime = (currTime - lastSecond) / (float)frames;
            lastSecond = currTime;
            frames = 0;
            // Audio_Play(&game.audioThread, drum, 2300, MAX_AUDIO_VOLUME, 100);
        }

        if(game.deltatime == 0)
            continue;

        ++frames;

        if(game.deltatime > MAX_DT)
            game.deltatime = TARGET_DT;



        lastTime = currTime;

        game.Update(&game);
        game.Render(&game);

        if(game.displayFps == TRUE){
            sprintf(fpsBuffer, "FPS: %i | MS: %f", game.fps, game.frameTime);
            TextLine_t line = GAME_DISPLAY_FPS_SETTINGS(fpsBuffer);
            Renderer_RenderString(&game.renderer, &line);
        }

		Window_Swap();
    }

    // exit

    Renderer_Close(&game.renderer);
    Audio_Close(&game.audioThread);
}