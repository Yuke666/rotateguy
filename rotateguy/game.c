#include <SDL2/SDL.h>
#include <GL/glew.h>
#include <string.h>
#include "game.h"
#include "graphics.h"
#include "memory.h"
#include "window.h"

#define TILES_TEXTURE_PATH "resources/texture.png"

#ifndef DEFAULT_DISPLAY_FPS
#define DEFAULT_DISPLAY_FPS FALSE
#endif

#define TARGET_DT 16

#define PARTICLE_FRICTION 0.005

static const char *characterTexPaths[NUM_CHR_TEXTURES] = {
    "resources/player.png",
};

static const char *LevelPaths[NUM_LEVELS] = {
    "resources/levels/one.txt",
};

const u8 TileFlags[NUM_TILES] = {
    0,                                                                              // TILE_NONE
    TILE_MASK_16x16_COLLISION,                                                      // TILE_STANDARD
    TILE_MASK_DEADLY,                                                               // TILE_SPIKE_UP
    TILE_MASK_DEADLY,                                                               // TILE_SPIKE_RIGHT
    TILE_MASK_DEADLY,                                                               // TILE_SPIKE_DOWN
    TILE_MASK_DEADLY,                                                               // TILE_SPIKE_LEFT
    // TILE_MASK_16x16_COLLISION | TILE_MASK_BELOW_INVISIBLE | TILE_MASK_NO_COLLISION, // TILE_STICKY
    TILE_MASK_16x16_COLLISION,                                                      // TILE_STICKY
    0,                                                                              // TILE_CHECKPOINT_UP
    0,                                                                              // TILE_CHECKPOINT_DOWN
    TILE_MASK_CREATE_OBJECT | TILE_MASK_PARTICLES,                                  // TILE_JUMP1
    TILE_MASK_CREATE_OBJECT | TILE_MASK_PARTICLES,                                  // TILE_JUMP2
    TILE_MASK_CREATE_OBJECT | TILE_MASK_PARTICLES,                                  // TILE_SHOOT_RIGHT
    TILE_MASK_CREATE_OBJECT | TILE_MASK_PARTICLES,                                  // TILE_SHOOT_DOWN
    TILE_MASK_CREATE_OBJECT | TILE_MASK_PARTICLES,                                  // TILE_SHOOT_UP
    TILE_MASK_CREATE_OBJECT | TILE_MASK_PARTICLES,                                  // TILE_SHOOT_LEFT
};

static const int DefaultKeys[] = {
    SDLK_w,
    SDLK_LEFT,
    SDLK_RIGHT,
    SDLK_r,
    SDLK_q,
};

static void LoadLevel(Game_t *game);

static void Events(Game_t *game){

    SDL_Event ev;
    while(SDL_PollEvent(&ev)){

        if(ev.type == SDL_QUIT)
            game->state = GAMESTATE_QUIT;

        if(ev.type == SDL_KEYDOWN){

            int key = ev.key.keysym.sym;

            if(game->keys[KEY_JUMP] == key)             game->rotateguy->Event(game->rotateguy, game, EV_JUMP, 0, 0);
            else if(game->keys[KEY_ROTATE_CW] == key)   game->rotateguy->Event(game->rotateguy, game, EV_ROTATE_CW, 0, 0);
            else if(game->keys[KEY_ROTATE_CCW] == key)  game->rotateguy->Event(game->rotateguy, game, EV_ROTATE_CCW, 0, 0);
            
            else if(game->keys[KEY_DEBUG_DRAW_QUADTREE] == key){
                
                game->debug.drawQuadtree = !game->debug.drawQuadtree;

            } else if(game->keys[KEY_RELOAD_LVL] == key){
                World_DeleteLevel(&game->world);
                LoadLevel(game);
                game->state = GAMESTATE_QUIT;
                // game->rotateguy->Event(game->rotateguy, game, EV_RESPAWN, 0, 0); 
                // game->nParticles = 0;


            }

        } 

        if(ev.type == SDL_KEYUP){
            
            int key = ev.key.keysym.sym;

            if(game->keys[KEY_ROTATE_CW] == key)        game->rotateguy->Event(game->rotateguy, game, EV_STOP_ROTATE_CW, 0, 0);
            else if(game->keys[KEY_ROTATE_CCW] == key)  game->rotateguy->Event(game->rotateguy, game, EV_STOP_ROTATE_CCW, 0, 0);
        } 
    }
}

static void Render(Game_t *game){
    (void)game;

    Graphics_RenderString("level 1", 50, 50, 1, 1, 1);
    Graphics_RenderString("the awakening ", 50, 68, 1, 1, 1);

    Graphics_SetCameraPos((Vec2){game->camx, game->camy});

    Rect2D renderRect = (Rect2D){game->camx, game->camy, WIDTH, HEIGHT};

    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE);
    // glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    u32 k;
    for(k = 0; k < game->nParticles; k++){
        
        Particle_t *p = &game->particles[k];

        float lifePercentage = ((game->currTime - p->emitTime) / (float)p->lifeTime);

        float size = (p->birthSize * (1 - lifePercentage)) + (p->deathSize * lifePercentage);
        
        u8 r = (p->r * (1 - lifePercentage)) + (p->r2 * lifePercentage);
        u8 g = (p->g * (1 - lifePercentage)) + (p->g2 * lifePercentage);
        u8 b = (p->b * (1 - lifePercentage)) + (p->b2 * lifePercentage);
        u8 a = (p->a * (1 - lifePercentage)) + (p->a2 * lifePercentage);

        if(p->tile != TILE_NONE){

            Graphics_RenderTile(p->pos.x, p->pos.y, size, size, 
                p->tile, game->tilesTexture, r, g, b, a);

        } else {

            Graphics_RenderRect(p->pos.x - size/2, p->pos.y - size/2, size, size, r, g, b, a);
        }
    }

    glDisable(GL_BLEND);


    // Graphics_RenderTileMap(game->camx, game->camy, game->lvl.data, game->lvl.width, game->lvl.height, game->tilesTexture);
    World_DrawLevel(&game->world);

    if(game->debug.drawQuadtree)
        World_DrawQuadtree(&game->world, renderRect);


    World_LocalEvent(game, &game->world, renderRect, EV_RENDER, 0, 0);


    Graphics_Render();
}

static void UpdateParticles(Game_t *game){

    Shape_t shape;
    shape.rotation = 0;
    shape.nPoints = 4;

    u32 k;
    for(k = 0; k < game->nParticles; k++){

        Particle_t *p = &game->particles[k];

        if(p->lifeTime > 0 && game->currTime >= p->emitTime + p->lifeTime){
            game->nParticles--;
            *p = game->particles[game->nParticles]; 
            --k;
            continue;
        }

        p->vel.y += game->deltatime * GRAVITY * p->mass;
        p->pos.x += p->vel.x * (float)game->deltatime;
        p->pos.y += p->vel.y * (float)game->deltatime;
    
        if((p->flags & PARTICLE_FLAG_COLLISION)){

            shape.pos = (Vec2){p->pos.x,p->pos.y};

            shape.points[0] = (Vec2){-p->birthSize/2, -p->birthSize/2};
            shape.points[1] = (Vec2){+p->birthSize/2, -p->birthSize/2};
            shape.points[2] = (Vec2){+p->birthSize/2, +p->birthSize/2};
            shape.points[3] = (Vec2){-p->birthSize/2, +p->birthSize/2};

            Vec2 resolve = (Vec2){0,0};

            World_GetShapeCollisionResolve(&game->world, &shape, &resolve);
            float mag = Math_Vec2Magnitude(resolve);

            if(mag){

                p->pos.x -= resolve.x;
                p->pos.y -= resolve.y;

                resolve = Math_Vec2DivideFloat(resolve, mag);

                SWAP(resolve.x, resolve.y, float);

                resolve.y = -resolve.y;

                p->vel = Math_Vec2Reflect(p->vel, resolve);
            }
        }

        p->vel.x -= p->vel.x * PARTICLE_FRICTION * (float)game->deltatime;
        p->vel.y -= p->vel.y * PARTICLE_FRICTION * (float)game->deltatime;
    }


}

static void Update(Game_t *game){

    UpdateParticles(game);

    Rect2D updateRect = (Rect2D){game->camx, game->camy, WIDTH, HEIGHT};
    World_LocalEvent(game, &game->world, updateRect, EV_LOCALUPDATE, 0, 0);

    game->rotateguy->Event(game->rotateguy, game, EV_OBJUPDATE, 0, 0);
}

static void LoadTilesTexture(Game_t *game){

    u8 *tilesTexturePixels;
    ImageLoader_LoadTextureIntoMemory(&game->tilesTexture, TILES_TEXTURE_PATH, 0, &tilesTexturePixels, STACK_TOP);
    

    int k;
    for(k = 1; k < NUM_TILES; k++){
        
        u32 tx = ((k-1) * TILE_SIZE) % (game->tilesTexture.w);
        u32 ty = (((k-1) * TILE_SIZE) / game->tilesTexture.w)*TILE_SIZE;

        int x, y;
        for(x = 0; x < TILE_SIZE; x++){
            for(y = 0; y < TILE_SIZE; y++){

                int intoIndex = ((y*TILE_SIZE)+x) * 4;
                int from = (((y+ty)*game->tilesTexture.w)+x+tx) * 4;

                game->tilePixels[k][intoIndex] = tilesTexturePixels[from]; 
                game->tilePixels[k][intoIndex+1] = tilesTexturePixels[from+1]; 
                game->tilePixels[k][intoIndex+2] = tilesTexturePixels[from+2]; 
                game->tilePixels[k][intoIndex+3] = tilesTexturePixels[from+3]; 
            }
        }
    }

    Memory_Pop(1, STACK_TOP);
}

static void LoadCharacterTextures(Game_t *game){

    int k;

    for(k = 0; k < NUM_CHR_TEXTURES; k++){

        Texture_t *tex = &game->characterTextures[k];

        u8 *pixels;
        ImageLoader_LoadTextureIntoMemory(tex, (char *)characterTexPaths[k], 0, &pixels, STACK_TOP);

        u32 x, y;
        for(x = 0; x < tex->w; x++){
            for(y = 0; y < tex->h; y++){

                int index = ((y*tex->w)+x);

                game->characterPixels[k][index] = pixels[(index*4)+3]; 
            }
        }

        Memory_Pop(1, STACK_TOP);
    }
}

static void LoadLevel(Game_t *game){

    FILE *fp = fopen(LevelPaths[game->lvlIndex], "rb");

    game->lvl.width = 0;
    game->lvl.height = 0;

    u8 *data = NULL;

    char key[32];
    char *keyc = key;
    int num = 0;
    
    while(!feof(fp)){

        int c = fgetc(fp);

        if(c >= 'a'&& c <= 'z') *(keyc++) = c;

        if(c == '=') {

            *(keyc++) = 0;

            if(strcmp(key, "width") == 0)       data = (u8 *)&game->lvl.width;
            else if(strcmp(key, "height") == 0) data = (u8 *)&game->lvl.height;
            else if(strcmp(key, "data") == 0)   data = game->lvl.data;

            keyc = key;

            do { c = fgetc(fp); } while(!feof(fp) && strchr("\n\r", c));

            if(data == NULL){
                do { c = fgetc(fp); } while(!feof(fp) && strchr("\n\r", c)  == 0);
                continue;
            }

            num = 0;

            do {
                
                if(c >= '0' && c <= '9'){
                    num = (num * 10) + (c - '0');
                    c = fgetc(fp);
                } else if(c == ','){
                    *data = (u8)num;
                    data++;
                    num = 0;
                    do { c = fgetc(fp); } while(!feof(fp) && strchr("\n\r", c));
                } else {
                    if(num > 0xFF) *((u16 *)data) = (u16)num;
                    else *((u8 *)data) = (u8)num;
                    break;
                }

            } while(!feof(fp));

            data = NULL;
        }

        if(c == '['){ while(!feof(fp) && c != ']') { c = fgetc(fp); } fgetc(fp); }
    }

    fclose(fp);

    World_LoadLevel(&game->world, game->lvl.data, game->lvl.width, game->lvl.height, game->tilesTexture);
}

void Game_Run(void){

	Game_t *game = Memory_Alloc(0, sizeof(Game_t));

    memset(game, 0, sizeof(Game_t));

    Audio_Init(&game->audioThread);

    // if(MODFile_Load(&game->modFile, "resources/osborne1.mod"))
    //     Audio_PlayMOD(&game->audioThread, &game->modFile);

    memcpy(game->keys, DefaultKeys, sizeof(game->keys));

    LoadCharacterTextures(game);

    LoadTilesTexture(game);

    game->lvlIndex = 0;
    LoadLevel(game);

    glClearColor(0,0,0,1);

    game->rotateguy = Character_New(game, (Vec2){100, PLAYER_START_OFFSET_TOP}, CHR_TEX_ROTATEGUY);

    game->Update = Update;
    game->Events = Events;
    game->Render = Render;

	game->state = GAMESTATE_RUNNING;
    game->displayFps = DEFAULT_DISPLAY_FPS;

	u32 lastTime = SDL_GetTicks();
    u32 frames = 0;
    u32 lastSecond = SDL_GetTicks();

    // char fpsBuffer[32];

	while(game->state != GAMESTATE_QUIT){

        game->Events(game);

        game->currTime = SDL_GetTicks();

        game->deltatime = game->currTime - lastTime;

        if(game->currTime - lastSecond > 1000){
            game->fps = frames;
            game->frameTime = (game->currTime - lastSecond) / (float)frames;
            lastSecond = game->currTime;
            frames = 0;

            printf("fps: %i | ms: %f\n", game->fps, game->frameTime);
        }

#if (FPS_CAP == 0)
        if(game->deltatime == 0)
            continue;
#endif

        ++frames;

        if(game->deltatime > TARGET_DT)
            game->deltatime = TARGET_DT;


        lastTime = game->currTime;

        game->Update(game);
        game->Render(game);

        // if(game->displayFps == TRUE){
            // sprintf(fpsBuffer, "FPS: %i | MS: %f", game->fps, game->frameTime);
            // ...
        // }

		Window_Swap();
    }


    Audio_Close(&game->audioThread);

    World_DeleteLevel(&game->world);

    int k;
    for(k = 0; k < NUM_CHR_TEXTURES; k++)
        glDeleteTextures(1, &game->characterTextures[k].texture);

    glDeleteTextures(1, &game->tilesTexture.texture);
    
    /* pop game structure */
    
    Memory_Pop(0,1);

}