#include <SDL2/SDL.h>
#include <GL/glew.h>
#include "menu.h"
#include "window.h"
#include "math.h"
#include "game.h"

static const TextLine_t mainMenu[] = {
	{10, 10, 4, 16, 255, 0, 255, "hello world"},
	{10, 40, 0, 8, 255, 0, 255, "hello world"},
};

static void Close(Game_t *game){

	Renderer_DestroyTextVao(&game->menu.text.vao, &game->menu.text.vbo);
	Renderer_DeleteImage(&game->menu.logoImage);
}

static void MilkDropEffect(Game_t *game){

    float currTime = SDL_GetTicks();

    glUseProgram(game->renderer.shaders[SHADER_MILKDROP].program);
    glUniform1f(game->renderer.shaders[SHADER_MILKDROP].milkdrop.time, currTime);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	Renderer_Milkdrop(&game->renderer);
    glDisable(GL_BLEND);

    glUseProgram(game->renderer.shaders[SHADER_TEXTURELESS_2D].program);


    glUniform3f(game->renderer.shaders[SHADER_TEXTURELESS_2D].color, 
        (1 + (0.5*sin(currTime * 0.01 + 100))),
        (1 + (0.5*sin(currTime * 0.01 + 20))), 
        (1 + (0.5*sin(30 + currTime * 0.01)))
    );

    // Shaders_DrawStrokedCircle((Vec3){cBuffer.width/2,cBuffer.height/2,0}, cBuffer.wiegh, 4, 0, 10);

	Vec2 lines[AUDIO_MAX_SAMPLES];

	SDL_LockAudio();
	u32 k;
	for(k = 0; k < game->audioThread.nSamples; k++){
		lines[k].y = (game->renderer.height/2) + (game->audioThread.samples[k]/255.0f*game->renderer.height/2);
		lines[k].x = ((float)k/(float)game->audioThread.nSamples) * (float)game->renderer.width;
	}
	SDL_UnlockAudio();

    glLineWidth(2);

	Renderer_DrawLines(&game->renderer, lines, game->audioThread.nSamples);



    glUniform3f(game->renderer.shaders[SHADER_TEXTURELESS_2D].color, 
        (1 + (0.5*sin(30 + currTime * 0.01))),
        (1 + (0.5*sin(currTime * 0.01 + 100))),
        (1 + (0.5*sin(currTime * 0.01 + 20))) 
    );

    float y;

    for(k = 0; k < 4; k++){

        y = (game->audioThread.modPlayer.channels[k].period/2) + 10;
        Renderer_DrawStrokedCircle(&game->renderer, (game->renderer.width/5)*(k+1), y, 25, k+3, y / 10.0f, 10);
    }

    // Shaders_DrawCircle((Vec3){cBuffer.width/2 - 30,cBuffer.height/2 - 30,0}, 60, 3, 0);

    // Shaders_SetUniformColor((Vec4){1,1,1,1});

    // Shaders_DrawStrokedCircle((Vec3){0,0,0}, 0.8, 12, rotation, 0.1);
    // Shaders_SetUniformColor((Vec4){1,1,1,1});
    // Shaders_DrawStrokedCircle((Vec3){0,0,0}, 0.6, 6, rotation, 0.1);

    // Shaders_DrawStrokedCircle((Vec3){0,0,0}, 1, 4, PI/4, 0.2);
}

static void DrawMilkDrop(Game_t *game){

	glViewport(0, 0, game->renderer.milkdrop.buffers[1].width, game->renderer.milkdrop.buffers[1].height);

    // if((SDL_GetTicks() - game->lastMilkdropDraw) > MILKDROP_DELAY){

        Renderer_BindColorBuffer(GL_FRAMEBUFFER,game->renderer.milkdrop.buffers[0]);
        glClear(GL_COLOR_BUFFER_BIT);

        Renderer_BindColorBuffer(GL_READ_FRAMEBUFFER,game->renderer.milkdrop.buffers[1]);
        Renderer_BindColorBuffer(GL_DRAW_FRAMEBUFFER,game->renderer.milkdrop.buffers[0]);

        glBlitFramebuffer(0, 0, game->renderer.milkdrop.buffers[0].width, 
            game->renderer.milkdrop.buffers[0].height, 0, 0, game->renderer.milkdrop.buffers[0].width,
            game->renderer.milkdrop.buffers[0].height, GL_COLOR_BUFFER_BIT, GL_NEAREST);

        // game->lastMilkdropDraw = SDL_GetTicks();
    // }

	float currTime = SDL_GetTicks();

    glClearColor(((0.5*sin(currTime * 0.01 + 100))), ((0.5*sin(currTime * 0.01 + 20))), 
        ((0.5*sin(30 + currTime * 0.01))),1);

    // glClearColor(0,0,0,1);

    Renderer_BindColorBuffer(GL_FRAMEBUFFER, game->renderer.milkdrop.buffers[1]);
    glClear(GL_COLOR_BUFFER_BIT);

    MilkDropEffect(game);
}

static void Render(Game_t *game){

    (void)game;
    glClearColor(0,0,0,0);
    glClear(GL_COLOR_BUFFER_BIT);

    // milkdrop
    DrawMilkDrop(game);
    glClearColor(0,0,0,0);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glClear(GL_COLOR_BUFFER_BIT);

	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
	Renderer_BindColorBuffer(GL_READ_FRAMEBUFFER, game->renderer.milkdrop.buffers[1]);

	glViewport(0, 0, GAME_DEFAULT_WIDTH, GAME_DEFAULT_HEIGHT);

	glBlitFramebuffer(0,0,game->renderer.milkdrop.buffers[1].width, game->renderer.milkdrop.buffers[1].height,
		0,0, GAME_DEFAULT_WIDTH, GAME_DEFAULT_HEIGHT, GL_COLOR_BUFFER_BIT, GL_NEAREST);


	// remaining

	// Renderer_RenderTextVao(&game->renderer, game->menu.text.vao, game->menu.text.vbo, game->menu.text.num);

	// Renderer_RenderTexturedRect(&game->renderer, VIEWPORT_WIDTH/2 - 64, VIEWPORT_HEIGHT/2 - 64, 128, 128, 0, 0, 1, 1, game->menu.logoImage);
}

static void Events(Game_t *game){
    
    SDL_Event ev;
    
    while(SDL_PollEvent(&ev)){

        if(ev.type == SDL_QUIT || (ev.type == SDL_KEYDOWN && (ev.key.keysym.sym == SDLK_ESCAPE || ev.key.keysym.sym == SDLK_q)))
            game->state = GAMESTATE_QUIT;
    }
}

static void Update(Game_t *game){

    (void)game;
}

void Menu_Start(Game_t *game){

	game->Render = Render;
	game->Events = Events;
	game->Update = Update;

	Renderer_CreateTextVao(&game->menu.text.vao, &game->menu.text.vbo);

	game->menu.text.num = 0;

	u32 k;
	for(k = 0; k < sizeof(mainMenu)/sizeof(TextLine_t); k++)
		Renderer_CalcTextDataLen(&mainMenu[k], &game->menu.text.num);

	Renderer_SetTextVaoBufferSize(game->menu.text.vao, game->menu.text.vbo, game->menu.text.num);

	game->menu.text.num = 0;

	for(k = 0; k < sizeof(mainMenu)/sizeof(TextLine_t); k++)
		Renderer_InsertTextData(game->menu.text.vao, game->menu.text.vbo, (TextLine_t *)&mainMenu[k], &game->menu.text.num);

	// init images

	extern const u16 menu_logo[];
	Renderer_CreateImage(&game->menu.logoImage, menu_logo, 64, 64);
}
