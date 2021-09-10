#include "level.h"
#include <stdio.h>

u8 Level_GridAt(Level_t *lvl, u32 x, u32 y){

	y >>= TILE_SIZE_BITS;
	x >>= TILE_SIZE_BITS;

	// return (lvl->grid.data[(y << lvl->grid.widthShift) + (x >> 1)] >> ((~x & 0x01)<<1)) & 0x0F;

	return (lvl->grid.data[y][x >> 1] >> ((~x & 0x01)<<2)) & 0x0F;
}

void Level_SetGridAt(Level_t *lvl, u32 x, u32 y, u8 set){

	y >>= TILE_SIZE_BITS;
	x >>= TILE_SIZE_BITS;

	u8 *curr = &lvl->grid.data[y][x >> 1];


	if(x & 0x01){
		*curr &= ~0x0F;
		*curr |= set & 0x0F;
		return;
	}

	*curr &= ~0xF0;
	*curr |= set << 4;
}
