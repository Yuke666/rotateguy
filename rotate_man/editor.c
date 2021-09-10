#include <SDL2/SDL.h>
#include <GL/glew.h>
#include "editor.h"
#include "game.h"

static void Close(Game_t *game){
	(void)game;
}

static void Render(Game_t *game){

    (void)game;
    glClearColor(1,1,1,1);
    glClear(GL_COLOR_BUFFER_BIT);

    // Renderer_Render(game->level);
}

static void Events(Game_t *game){

    Editor_t *editor = &game->editor;    

    SDL_Event ev;
    
    while(SDL_PollEvent(&ev)){

        if(ev.type == SDL_QUIT || (ev.type == SDL_KEYDOWN && (ev.key.keysym.sym == SDLK_ESCAPE || ev.key.keysym.sym == SDLK_q))){
            Close(game);
            game->state = GAMESTATE_QUIT;
        }

        if(ev.type == SDL_MOUSEMOTION){

            u32 x = (ev.motion.x/(float)game->windowWidth) * game->viewportWidth;
            u32 y = (ev.motion.y/(float)game->windowHeight) * game->viewportHeight;


            
        }
    }
}

static void Update(Game_t *game){

    (void)game;
}

void Editor_Start(Game_t *game){

	game->Render = Render;
	game->Events = Events;
	game->Update = Update;

    Editor_t *editor = &game->editor;    

    Level_SetGridAt(&editor->lvl, 0, 0, 1);
}
