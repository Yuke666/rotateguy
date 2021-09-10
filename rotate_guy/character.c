#include "character.h"
#include "level.h"
#include "graphics.h"

#define ROTATION_SPEED 0.01 
#define JUMP_VEL -0.4
#define GRAVITY 0.0009

void Character_Render(Character_t *chr, Level_t *lvl){

	Graphics_RenderSprite(chr->pos.x - (chr->sw/2), chr->pos.y - (chr->sh/2),
		chr->sw, chr->sh, chr->rotation, 0, 0, chr->sx, chr->sy);

	Graphics_RenderRect(chr->pos.x - chr->w/2, chr->pos.y - chr->h/2, chr->w, chr->h, chr->rotation, 0, 0);
}

void Character_Jump(Character_t *chr){
	
	if(chr->remainingJumps == 0)
		return;

	chr->vel.y = JUMP_VEL;
	chr->flags &= ~FLAG_ONGROUND;
	--chr->remainingJumps;
}

void Character_Event(Character_t *chr, Level_t *lvl, u32 event, EventArg_t *args, u8 nArgs){

	if(event == EVENT_ROTATECCW)
		chr->angMomentum = -ROTATION_SPEED;
	else if(event == EVENT_ROTATECW)
		chr->angMomentum = ROTATION_SPEED;
	else if(event == EVENT_STOPROTATING)
		chr->angMomentum = 0;
	else if(event == EVENT_JUMP)
		Character_Jump(chr);
}

void Character_Update(Character_t *chr, Level_t *lvl, float dt){

	if((chr->flags & FLAG_ONGROUND) == 0)
		chr->vel.y += dt * GRAVITY;

	chr->rotation += dt * chr->angMomentum;
	chr->pos = VEC2V(chr->pos, VEC2F(chr->vel, dt, *), +);

	u16 size = MAX((MAX(chr->w, chr->h) >> TILE_SIZE_BITS), 1);

	u16 left = ((u16)chr->pos.x >> TILE_SIZE_BITS);
	u16 top = ((u16)chr->pos.y >> TILE_SIZE_BITS);
	u16 right = left + size;
	u16 bottom = top + size;

	u16 x, y;
	
	for(y = top; y <= bottom; y++){
		for(x = left; x <= right; x++){

			if(lvl->lvl[(y * lvl->lvlWidth) + x] != 0){

				chr->flags |= FLAG_ONGROUND;
				chr->vel.y = 0;
				chr->remainingJumps = 1;
			}
		}
	}
}

void Character_Init(Character_t *chr){
	chr->pos.x = 100;
	chr->pos.y = 200;
	chr->vel.x = 0;
	chr->vel.y = 0;
	chr->w = 16;
	chr->h = 28;
	chr->sw = 20;
	chr->sh = 20;
	chr->sx = 00;
	chr->sy = 00;
	chr->rotation = 0;
	chr->angMomentum = 0;
	chr->flags = FLAG_ONGROUND;
	chr->remainingJumps = 1;
}