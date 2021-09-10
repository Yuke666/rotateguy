#ifndef LEVEL_DEF
#define LEVEL_DEF

#include "types.h"

#define TILE_SIZE_BITS 4
#define TILE_SIZE 16
#define MAX_LEVEL_HEIGHT 32
#define MAX_LEVEL_WIDTH 1024

typedef struct {
	u8 	data[MAX_LEVEL_HEIGHT][MAX_LEVEL_WIDTH];
} LevelGrid_t;

typedef struct {
	LevelGrid_t grid;
} Level_t;

void Level_SetGridAt(Level_t *lvl, u32 x, u32 y, u8 set);
u8 Level_GridAt(Level_t *lvl, u32 x, u32 y);

#endif