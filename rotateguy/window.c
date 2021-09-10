#include <GL/glew.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>
#include "types.h"
#include "window.h"
#include "log.h"

static SDL_Window *window;
static SDL_GLContext context;

int Window_Open(int w, int h){

    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);

    window = SDL_CreateWindow(
        GAME_TITLE,
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        w,
        h,
        SDL_WINDOW_OPENGL
    );

    context = SDL_GL_CreateContext(window);

    glewExperimental = GL_TRUE;

    if(glewInit() != GLEW_OK) {
        LOG(LOG_RED, "Glew Init Failed");
        return ERROR;
    }
    
    SDL_GL_SetSwapInterval(FPS_CAP);

    return SUCCESS;
}

void Window_Close(){
    SDL_GL_DeleteContext(context);
    SDL_DestroyWindow(window);
    SDL_Quit();
}

void Window_Swap(void){

    SDL_GL_SwapWindow(window);
}