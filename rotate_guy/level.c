#include "level.h"
#include "graphics.h"

static const u8 levelData[34*17] = {
	9,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,11,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,4,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	1,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,3,
};

static const u32 levelWidth = 34;
static const u32 levelHeight = 17;

void Level_Render(Level_t *lvl){

	Graphics_RenderTileMap(0, 0, lvl->lvl, lvl->lvlWidth, lvl->lvlHeight);

	Graphics_BeginRenderSprites();

	Character_Render(&lvl->player, lvl);
	
	Graphics_EndRenderSprites();
}

void Level_Update(Level_t *lvl, float dt){

	Character_Update(&lvl->player, lvl, dt);
}

void Level_Event(Level_t *lvl, SDL_Event ev){

	if(ev.type == SDL_KEYUP){

		switch(ev.key.keysym.sym){

			case SDLK_w:
				
				Character_Event(&lvl->player, lvl, EVENT_JUMP, NULL, 0);
				break;

			case SDLK_LEFT:

				lvl->rotating[ROTATE_CCW] = 0;
			
				if(lvl->rotating[ROTATE_CW])
					Character_Event(&lvl->player, lvl, EVENT_ROTATECW, NULL, 0);
				else
					Character_Event(&lvl->player, lvl, EVENT_STOPROTATING, NULL, 0);
			
				break;
			
			case SDLK_RIGHT:
	
				lvl->rotating[ROTATE_CW] = 0;
	
				if(lvl->rotating[ROTATE_CCW])
					Character_Event(&lvl->player, lvl, EVENT_ROTATECCW, NULL, 0);
				else
					Character_Event(&lvl->player, lvl, EVENT_STOPROTATING, NULL, 0);
	
				break;
		}

		return;
	}

	if(ev.type == SDL_KEYDOWN){

		switch(ev.key.keysym.sym){

			case SDLK_w:
				Character_Event(&lvl->player, lvl, EVENT_JUMP, NULL, 0);
				break;
			case SDLK_LEFT:
				lvl->rotating[ROTATE_CCW] = 1;
				Character_Event(&lvl->player, lvl, EVENT_ROTATECCW, NULL, 0);
				break;
			case SDLK_RIGHT:
				lvl->rotating[ROTATE_CW] = 1;
				Character_Event(&lvl->player, lvl, EVENT_ROTATECW, NULL, 0);
				break;
		}

		return;
	}
}

void Level_Start(Level_t *lvl){
	lvl->lvl = levelData;
	lvl->lvlWidth = levelWidth;
	lvl->lvlHeight = levelHeight;
	Character_Init(&lvl->player);
}