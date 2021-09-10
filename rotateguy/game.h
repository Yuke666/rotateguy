#ifndef GAME_DEF
#define GAME_DEF

#include "types.h"
#include "world.h"
#include "character.h"
#include "image_loader.h"
#include "mod.h"
#include "audio.h"

#define IS_RENDERED_TILE(tile) tile == TILE_NONE || (TileFlags[tile] & TILE_MASK_CREATE_OBJECT)

#define PLAYER_OFFSET_RIGHT (WIDTH/4)
#define PLAYER_START_OFFSET_TOP (HEIGHT/2)

#define GRAVITY 0.0009

#define MAX_PARTICLES (TILE_SIZE*TILE_SIZE*10)

#define MAX_CHR_WIDTH TILE_SIZE*3
#define MAX_CHR_HEIGHT TILE_SIZE*3

enum {
	LEVEL_ONE = 0,
	NUM_LEVELS,
};

enum {
	PARTICLE_FLAG_FOLLOW = 0x01,
	PARTICLE_FLAG_PUSH = 0x02,
	PARTICLE_FLAG_COLLISION = 0x04,
};

enum {
	GAMESTATE_QUIT = 0x00,
	GAMESTATE_RUNNING,
};

enum {
	TILE_MASK_16x16_COLLISION = 0x01, 
	TILE_MASK_INVISIBLE = 0x02 | 0x04,
	TILE_MASK_BELOW_INVISIBLE = 0x02,
	TILE_MASK_ABOVE_INVISIBLE = 0x04,
	TILE_MASK_NO_COLLISION = 0x08,
	TILE_MASK_CREATE_OBJECT = 0x10,
	TILE_MASK_PARTICLES = 0x20,
	TILE_MASK_DEADLY = 0x40,
};

enum {
	TILE_NONE = 0,
	TILE_SQUARE,
	TILE_SPIKE_UP,
	TILE_SPIKE_RIGHT,
	TILE_SPIKE_DOWN,
	TILE_SPIKE_LEFT,
	TILE_STICKY,
	TILE_CHECKPOINT_UP,
	TILE_CHECKPOINT_DOWN,
	TILE_JUMP1,
	TILE_JUMP2,
	TILE_SHOOT_RIGHT,
	TILE_SHOOT_DOWN,
	TILE_SHOOT_UP,
	TILE_SHOOT_LEFT,
	NUM_TILES,
};

enum {
	CHR_TEX_ROTATEGUY = 0,
	NUM_CHR_TEXTURES,
};

enum {
    KEY_JUMP = 0,
    KEY_ROTATE_CCW,
    KEY_ROTATE_CW,
    KEY_RELOAD_LVL,
	KEY_DEBUG_DRAW_QUADTREE,
    NUM_KEY_BINDINGS,
};

typedef struct {
    u8 		r;
    u8 		g;
    u8 		b;
    u8 		a;
    u8 		r2;
    u8 		g2;
    u8 		b2;
    u8 		a2;
    u8     	birthSize;
    u8     	deathSize;
    u8     	tile;
	u8 		flags;
    u32 	lifeTime;
	u32 	emitTime;
	float 	mass;
    Vec2    pos;
    Vec2    vel;
} Particle_t;

typedef struct Game_t Game_t;

typedef struct {
	u8						data[LEVEL_WIDTH * LEVEL_HEIGHT];
	u16						width;
	u16						height;
} Level_t;

typedef struct {
	u8 drawQuadtree;
} GameDebug_t;

struct Game_t {
	u8						state;
	u8						displayFps;
	u8						lvlIndex;
	u16						fps;
	s32						camx;
	s32						camy;
	float					frameTime;
	u32 					deltatime;
	u32 					currTime;
	void 					(*Update)(Game_t *this);
	void 					(*Render)(Game_t *this);
	void 					(*Events)(Game_t *this);
	Texture_t				characterTextures[NUM_CHR_TEXTURES];
	Texture_t				tilesTexture;
	Object_t 				*rotateguy;
	int 					keys[NUM_KEY_BINDINGS];
	AudioThread_t			audioThread;
	MODFile_t				modFile;
	u8						tilePixels[NUM_TILES][4*TILE_SIZE*TILE_SIZE];
	u8						characterPixels[NUM_CHR_TEXTURES][MAX_CHR_WIDTH*MAX_CHR_HEIGHT];
	u32 					nParticles;
	Particle_t 				particles[MAX_PARTICLES];
	Level_t 				lvl;
	World_t 				world;
	GameDebug_t 			debug;
};

void 						Game_Run(void);

extern const u8 			TileFlags[NUM_TILES];

#endif