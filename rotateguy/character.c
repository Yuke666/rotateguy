#include <stdlib.h>
#include <stdio.h>
#include "character.h"
#include "game.h"
#include "level.h"
#include "graphics.h"

// character values
#define ROTATION_SPEED 0.01 
#define MAX_SLOPE 0.4
#define STICKY_MAX_ROTATION (PI/4)

// #define SHOOT_TILE_VEL 1.5
// #define SHOOT_TILE_FRICTION 0.005

// #define DEFAULT_MOVESPEED 0.155
// #define DEFAULT_JUMP_VEL -0.8
// #define CHARACTER_MASS 1

#define SHOOT_TILE_VEL 1.5
#define SHOOT_TILE_FRICTION 0.005
#define DEATH_DELAY 500

#define DEFAULT_MOVESPEED 0.5
#define DEFAULT_JUMP_VEL -1.1
#define CHARACTER_MASS 2.3
#define DEATH_SCREEN_SHAKE_SPEED 40
#define DEATH_SCREEN_SHAKE_AMOUNT 15
#define DEATH_ZOOM 0.05

// particles
#define DUST_LIGHTEN 0.9
#define DUST_PARTICLE_EMIT_DIST 1
#define DUST_PARTICLE_LIFETIME 200
#define DUST_PARTICLE_MASS 2
#define DUST_PARTICLE_SIZE 6

#define NUM_SPLAT_PARTICLES 8
#define SPLAT_PARTICLE_LIFETIME 200
#define SPLAT_PARTICLE_MASS 1.5
#define SPLAT_PARTICLE_SIZE 6

#define DEATH_PARTICLE_SIZE 3
#define DEATH_PARTICLE_LIFETIME 500
#define DEATH_PARTICLE_MASS 2

#define PARTICLE_SIZE 4
#define PARTICLE_MASS -0.3
#define RTGUY_VORTEX_FORCE_ANGULAR 0.1
#define RTGUY_VORTEX_FORCE 5
#define RTGUY_VORTEX_REST 25
#define VORTEX_FORCE 0.01
#define PARTICLE_LIFETIME 1000


static void OnCollision(Object_t *obj, Game_t *game, Object_t *obj2, Vec2 resolveVec);
static void OnCollisionStatic(Object_t *obj, Game_t *game, u8 tile, Vec2 resolveVec);
static void Update(Object_t *obj, Game_t *game);
static void Render(Object_t *obj, Game_t *game);
static void Event(Object_t *obj, Game_t *game, u32 event, u64 arg1, u64 arg2);

Object_t *Character_New(Game_t *game, Vec2 pos, u32 tex){

	Object_t *obj = World_NewObject(&game->world);

	Character_t *chr = (Character_t *)obj->data;
	
	chr->sw 				= game->characterTextures[tex].w;
	chr->sh 				= game->characterTextures[tex].h;
	chr->w 					= chr->sw;
	chr->h 					= chr->sh;

	pos.x += chr->w/2;
	pos.y += chr->h/2;

	chr->state 				= CHARACTER_STATE_NONE;
	obj->pos 				= pos;
	chr->respawnPos 		= pos;
	obj->rotation 			= 0;
	chr->angMomentum 		= 0;
	chr->remainingJumps 	= 1;
	chr->texture 			= tex;
	chr->rotatingCCW 		= 0;
	chr->rotatingCW 		= 0;
	chr->lastDustX 			= pos.x;
	chr->onSticky 			= CHARACTER_STICKY_NONE;
	chr->collidingSticky 	= CHARACTER_STICKY_NONE;
	chr->dustTile 			= TILE_NONE;
	chr->vel 				= (Vec2){0,0};
	chr->shootTileVel 		= (Vec2){0,0};
	chr->lastObjCollision 	= NULL;

	chr->vel.x = DEFAULT_MOVESPEED;

	obj->nPoints = 6;
	obj->points[0] = (Vec2){-chr->w/2, -chr->h/3};
	obj->points[1] = (Vec2){-chr->w/4, -chr->h/2};
	obj->points[2] = (Vec2){ chr->w/4, -chr->h/2};
	obj->points[3] = (Vec2){chr->w/2,  -chr->h/3};
	obj->points[4] = (Vec2){chr->w/2,  chr->h/2};
	obj->points[5] = (Vec2){-chr->w/2, chr->h/2};

	obj->OnCollisionStatic = OnCollisionStatic;
	obj->OnCollision = OnCollision;
	obj->Event = Event;

	// World_BVH_Add(&game->world, obj);
	World_QuadtreeUpdate(&game->world, obj);

	return obj;
}

static void Kill(Game_t *game, Object_t *obj){

    Character_t *chr = (Character_t *)obj->data;

    Graphics_SetAbberation(1);
	obj->active = 0;
	chr->state = CHARACTER_STATE_DEAD;
	chr->deathTime = game->currTime;

}

static void Respawn(Game_t *game, Object_t *obj){

	Character_t *chr = (Character_t *)obj->data;

    Graphics_SetAbberation(0);

	chr->state 				= CHARACTER_STATE_NONE;
	obj->pos 				= chr->respawnPos;
	obj->rotation 			= 0;
	chr->angMomentum 		= 0;
	chr->remainingJumps 	= 1;
	chr->rotatingCCW 		= 0;
	chr->rotatingCW 		= 0;
	chr->lastDustX 			= obj->pos.x;
	chr->dustTile 			= TILE_NONE;
	chr->vel 				= (Vec2){0,0};
	chr->shootTileVel 		= (Vec2){0,0};
	chr->lastObjCollision 	= NULL;

	chr->vel.x = DEFAULT_MOVESPEED;


	obj->active = 1;
    World_GlobalEvent(game, &game->world, EV_SETACTIVE, TRUE, 0);
}

static void Jump(Character_t *chr){
	
	if(chr->remainingJumps == 0)
		return;

	chr->vel.y = DEFAULT_JUMP_VEL;
	--chr->remainingJumps;
}

static void Render(Object_t *obj, Game_t *game){

	Character_t *chr = (Character_t *)obj->data;

	if(chr->state != CHARACTER_STATE_DEAD){
		Graphics_RenderRotatedSprite(obj->pos.x - chr->w/2, obj->pos.y - chr->h/2,
			chr->w, chr->h, obj->rotation, 0, 0, 0, 0, game->characterTextures[chr->texture]);
	}

}

static void Event(Object_t *obj, Game_t *game, u32 event, u64 arg1, u64 arg2){

	Character_t *chr = (Character_t *)obj->data;

	if(event == EV_JUMP){
		
		Jump(chr);

	} else if(event == EV_ROTATE_CW){
		
		chr->rotatingCW = 1;
		chr->angMomentum = ROTATION_SPEED;

	} else if(event == EV_ROTATE_CCW){
		
		chr->rotatingCCW = 1;
		chr->angMomentum = -ROTATION_SPEED;

	} else if(event == EV_STOP_ROTATE_CCW){
		
		chr->rotatingCCW = 0;
		chr->angMomentum = 0;
		if(chr->rotatingCW) chr->angMomentum = ROTATION_SPEED;

	} else if(event == EV_STOP_ROTATE_CW){
		
		chr->rotatingCW = 0;
		chr->angMomentum = 0;
		if(chr->rotatingCCW) chr->angMomentum = -ROTATION_SPEED;
	
	} else if(event == EV_RENDER){

		Render(obj, game);

	} else if(event == EV_OBJUPDATE){

		Update(obj, game);

	} else if (event == EV_RESPAWN){
		Respawn(game, obj);
	}
}

static void OnCollision(Object_t *obj, Game_t *game, Object_t *obj2, Vec2 resolveVec){

    Character_t *chr = (Character_t *)obj->data;

    if(chr->lastObjCollision == obj2){
    	return;
    }

   	if(obj2->tile == TILE_JUMP1){
   		chr->remainingJumps++;
   	} else if(obj2->tile == TILE_JUMP2){
   		chr->remainingJumps+=2;
    } else if(obj2->tile == TILE_SHOOT_LEFT){
		chr->shootTileVel.x = -SHOOT_TILE_VEL;
		chr->shootTileVel.y = 0;
		chr->vel.y = 0;
    } else if(obj2->tile == TILE_SHOOT_RIGHT){
		chr->shootTileVel.x = SHOOT_TILE_VEL;
		chr->shootTileVel.y = 0;
		chr->vel.y = 0;
    } else if(obj2->tile == TILE_SHOOT_UP){
		chr->shootTileVel.y = -SHOOT_TILE_VEL;
		chr->shootTileVel.x = 0;
		chr->vel.y = 0;
    } else if(obj2->tile == TILE_SHOOT_DOWN){
		chr->shootTileVel.y = SHOOT_TILE_VEL;
		chr->shootTileVel.x = 0;
		chr->vel.y = 0;
    }

	if((TileFlags[obj2->tile] & TILE_MASK_PARTICLES)){

		Vec2 center = obj2->pos;
		center.x += TILE_SIZE/2;
		center.y += TILE_SIZE/2;

		Particle_t *p;

	    int x, y;
	    for(x = 0; x < TILE_SIZE; x++){
		    for(y = 0; y < TILE_SIZE; y++){

		    	u8 index = (((TILE_SIZE-y-1) * TILE_SIZE) + x);

		        if(game->tilePixels[obj2->tile][(index*4)+3] == 0) continue;
	        
		    	p = &game->particles[game->nParticles++];

		        p->r = p->r2 = game->tilePixels[obj2->tile][(index*4)];
		        p->g = p->g2 = game->tilePixels[obj2->tile][(index*4)+1];
		        p->b = p->b2 = game->tilePixels[obj2->tile][(index*4)+2];
		        p->a = p->a2 = 0xFF;

				p->flags = 0;

		        p->lifeTime = PARTICLE_LIFETIME;
		        p->emitTime = game->currTime;

		        p->birthSize = PARTICLE_SIZE;
		        p->deathSize = 0;

		        p->mass = PARTICLE_MASS;

		        p->pos.x = (center.x - ((x - (TILE_SIZE/2)) * p->birthSize));
		        // p->pos.y = (center.y + (y-((TILE_SIZE/2)))*p->birthSize);
		        p->pos.y = (center.y - ((y - (TILE_SIZE/2)) * p->birthSize));

				p->flags = PARTICLE_FLAG_PUSH;

		        // p->vel = (Vec2){0,0};
		        p->vel = chr->vel;

		        // p->vel = Math_Vec2AddVec2(p->vel, Math_Vec2MultFloat(Math_Vec2Normalize(Math_Vec2SubVec2(p->pos, center)), 0.4));
		    }
	    }

	    chr->lastObjCollision = obj2;
	    obj2->Event(obj2, game, EV_SETACTIVE, 0, 0);
	    // World_RemoveObject(game, &game->world, obj2->handle);
   	}


}

static void CharacterParticlesSplat(Object_t *obj, Game_t *game, u8 splatTile, Vec2 resolveVec){

    Character_t *chr = (Character_t *)obj->data;

	Vec2 center = obj->pos;

	Particle_t *p;

    resolveVec = Math_Vec2Normalize(resolveVec);

	float sintheta = sinf(obj->rotation);
	float costheta = cosf(obj->rotation);

    resolveVec.x = -resolveVec.x;
    resolveVec.y = -resolveVec.y;

    Vec2 pos;

    int skip = 2;

    int x, y;
    for(x = 0; x < chr->sw; x+=skip){
	    for(y = 0; y < chr->sh; y+=skip){

	    	u32 index = ((y * chr->sw) + x);

	        if(game->characterPixels[chr->texture][index] == 0) continue;
        
	    	p = &game->particles[game->nParticles++];

	        p->r = p->r2 = 0xFF;
	        p->g = p->g2 = 0x00;
	        p->b = p->b2 = 0x00;
	        p->a = p->a2 = 0xFF;

			p->flags = PARTICLE_FLAG_COLLISION;

	        p->lifeTime = DEATH_PARTICLE_LIFETIME;
	        p->emitTime = game->currTime;

	        p->birthSize = DEATH_PARTICLE_SIZE;
	        p->deathSize = DEATH_PARTICLE_SIZE;

	        p->mass = DEATH_PARTICLE_MASS;

	        pos.x = (x - (chr->sw/2.0f));
	        pos.y = (y - (chr->sh/2.0f));

	        p->pos.x = center.x + ( (pos.x * costheta) - (sintheta * pos.y) );
	        p->pos.y = center.y + ( (pos.x * sintheta) + (costheta * pos.y) );

	        // p->vel = chr->vel;
	        p->vel = Math_Vec2AddVec2(chr->vel,
	        	Math_Vec2MultFloat(Math_Vec2Normalize(Math_Vec2SubVec2(p->pos, center)), Math_Vec2Magnitude(chr->vel)));

	        p->vel = Math_Vec2MultFloat(p->vel, 0.5);

	        // printf("%f %f\n", resolveVec.x, resolveVec.y);

	        // if(Math_Vec2Dot(resolveVec, Math_Vec2Normalize(p->vel)) < 0){
		       //  p->vel = Math_Vec2Reflect(resolveVec, p->vel);
	        // }
	    }
	}

}

static void ParticlesSplat(Object_t *obj, Game_t *game, u8 splatTile, Vec2 resolveVec){

    Character_t *chr = (Character_t *)obj->data;

	u32 index;

    if(resolveVec.y < 0){
	    index = (((TILE_SIZE-1)*TILE_SIZE) + (TILE_SIZE/2)) * 4;
	} else {
	    index = ((1*TILE_SIZE) + (TILE_SIZE/2)) * 4;
	}

	u8 r = game->tilePixels[splatTile][index+0];
	u8 g = game->tilePixels[splatTile][index+1];
	u8 b = game->tilePixels[splatTile][index+2];

    resolveVec.y = -resolveVec.y;
    resolveVec.x = -resolveVec.x;

	resolveVec = Math_Vec2Normalize(resolveVec);

	float step = chr->w/(float)(NUM_SPLAT_PARTICLES-1);

	Vec2 center = (Vec2){obj->pos.x, obj->pos.y};

	u32 k;
	for(k = 0; k < NUM_SPLAT_PARTICLES; k++){
		
		Particle_t *p = &game->particles[game->nParticles++];

		p->r = p->r2 = r;
		p->g = p->g2 = g;
		p->b = p->b2 = b;
        p->a = p->a2 = 0xFF;

	    p->lifeTime = SPLAT_PARTICLE_LIFETIME;
	    p->emitTime = game->currTime;

	    p->flags = 0;
	    
	    p->mass = SPLAT_PARTICLE_MASS;

	    p->birthSize = SPLAT_PARTICLE_SIZE;
        p->deathSize = 0;

	    p->pos.x = center.x + (k * step) - (chr->w/2);
	    p->pos.y = center.y + (((chr->h/2.0f)) * resolveVec.y);

	    p->vel = Math_Vec2MultFloat(Math_Vec2Reflect(resolveVec,
	    	Math_Vec2Normalize(Math_Vec2SubVec2(p->pos,center))), fabs(chr->lastVel.y) / p->mass);

	    p->vel.x += chr->lastVel.x;

	    if(resolveVec.y < 0)
		    p->pos.y += chr->h - p->birthSize;
		else
		    p->pos.y -= chr->h;
	}
}

static void ParticlesDust(Object_t *obj, Game_t *game){

    Character_t *chr = (Character_t *)obj->data;

    if(obj->pos.x - chr->lastDustX < DUST_PARTICLE_EMIT_DIST || chr->dustTile == TILE_NONE){
    	return;
    }

	float up = Math_Vec2Normalize(chr->dustResolve).y;

    u32 index = 0;

    if(up < 0){
	    index = (((TILE_SIZE-1)*TILE_SIZE) + (TILE_SIZE/2)) * 4;
	} else {
	    index = ((1*TILE_SIZE) + (TILE_SIZE/2)) * 4;
	}


    chr->lastVel.y = -chr->lastVel.y;
    chr->lastVel.x = -chr->lastVel.x;

	Vec2 center = (Vec2){obj->pos.x, obj->pos.y};

	Particle_t *p = &game->particles[game->nParticles++];

	u8 r = game->tilePixels[chr->dustTile][index+0];
	u8 g = game->tilePixels[chr->dustTile][index+1];
	u8 b = game->tilePixels[chr->dustTile][index+2];

	float luma = r * 0.2126 + g * 0.7152 + b * 0.0722;

	p->r = p->r2 = MIN((DUST_LIGHTEN * luma + r), 0xFF);
	p->g = p->g2 = MIN((DUST_LIGHTEN * luma + g), 0xFF);
	p->b = p->b2 = MIN((DUST_LIGHTEN * luma + b), 0xFF);
	p->a = p->a2 = 0xFF;


	chr->lastDustX = obj->pos.x;
	chr->dustTile = TILE_NONE;

    p->lifeTime = DUST_PARTICLE_LIFETIME;
    p->emitTime = game->currTime;

    p->flags = 0;
    
    p->mass = DUST_PARTICLE_MASS * up;

    p->birthSize = DUST_PARTICLE_SIZE;
    p->deathSize = 0;

    p->pos.x = center.x - (chr->w/2);

	p->vel = (Vec2){-chr->vel.x, -chr->vel.x * up}; 

    if(up > 0)
	    p->pos.y = center.y + (chr->h/2) - p->birthSize;
	else
	    p->pos.y = center.y - (chr->h/2);
}

static void OnCollisionStatic(Object_t *obj, Game_t *game, u8 tile, Vec2 resolveVec){
    
    Character_t *chr = (Character_t *)obj->data;
    obj->pos.y -= resolveVec.y;

    if(resolveVec.x && fabs(Math_Vec2Dot(resolveVec, (Vec2){0,1})) < MAX_SLOPE){
	    obj->pos.x -= resolveVec.x;
    	Kill(game, obj);
		CharacterParticlesSplat(obj, game, tile, resolveVec);
    	return;
    }

    if((TileFlags[tile] & TILE_MASK_DEADLY)){
	    obj->pos.x -= resolveVec.x;
    	Kill(game, obj);
		CharacterParticlesSplat(obj, game, tile, resolveVec);
    	return;
    }


    // if((TileFlags[tile] & TILE_MASK_16x16_COLLISION) && resolveVec.y != 0){
    // 	chr->onSticky = CHARACTER_STICKY_COLLISION;
    // }

    if(tile == TILE_STICKY){

    	chr->collidingSticky = CHARACTER_STICKY_COLLISION;

	    if(resolveVec.y < 0){
			chr->onSticky = CHARACTER_STICKY_STUCK;
			chr->dustTile = TILE_STICKY;
			chr->dustResolve = resolveVec;

			if(chr->lastVel.y != 0){
				ParticlesSplat(obj, game, TILE_STICKY, resolveVec);
			}
	    }
    }


	// if(chr->onSticky != CHARACTER_STICKY_NONE && chr->collidingSticky != CHARACTER_STICKY_NONE){

	//     if(resolveVec.y < 0){
	// 		chr->onSticky = CHARACTER_STICKY_STUCK;
	// 		chr->dustTile = TILE_STICKY;
	// 		chr->dustResolve = resolveVec;

	// 		if(chr->lastVel.y != 0){
	// 			ParticlesSplat(obj, game, TILE_STICKY, resolveVec);
	// 		}
	//     }
    // }

    if(resolveVec.y > 0){

		chr->remainingJumps = 1;
    }

    if(resolveVec.y != 0){
    	
    	if(chr->dustTile == TILE_NONE){
			chr->dustTile = tile;
			chr->dustResolve = resolveVec;
    	}
		
		chr->vel.y = 0;
    }
}

static void UpdateDead(Object_t *obj, Game_t *game){

    Character_t *chr = (Character_t *)obj->data;
		
	float percentage = ((game->currTime - chr->deathTime) / (float)DEATH_DELAY);

	obj->pos.x += chr->vel.x * game->deltatime * pow((1-percentage), 3);

	if(obj == game->rotateguy){
	
	    game->camx = obj->pos.x - PLAYER_OFFSET_RIGHT;
	    game->camy = 0;

		int shake = ((game->currTime - chr->deathTime)/DEATH_SCREEN_SHAKE_SPEED) % 4;

		game->camx += ((shake % 2)*2-1)*(DEATH_SCREEN_SHAKE_AMOUNT*MAX(0.2, pow((1-percentage), 2)));
		game->camy += ((shake / 2)*2-1)*(DEATH_SCREEN_SHAKE_AMOUNT*MAX(0.2, pow((1-percentage), 2)));

		Graphics_SetCameraZoom((Vec2){1+DEATH_ZOOM, 1+DEATH_ZOOM});
	}

	if(game->currTime - chr->deathTime > DEATH_DELAY){
		Respawn(game, obj);
		Graphics_SetCameraZoom((Vec2){1, 1});
		return;
	}

}

static void UpdateAlive(Object_t *obj, Game_t *game){

    Character_t *chr = (Character_t *)obj->data;

	if(chr->onSticky != CHARACTER_STICKY_STUCK){
		chr->vel.y += game->deltatime * GRAVITY * CHARACTER_MASS;
	} else {
		chr->vel.y -= game->deltatime * GRAVITY * CHARACTER_MASS;
	}

	chr->shootTileVel = Math_Vec2SubVec2(chr->shootTileVel,
		Math_Vec2MultFloat(chr->shootTileVel, game->deltatime * SHOOT_TILE_FRICTION)); 

	// if(chr->shootTileVel.x) chr->vel.y = 0;

	chr->collidingSticky = CHARACTER_STICKY_NONE;

	obj->rotation += game->deltatime * chr->angMomentum;

	while(obj->rotation < 0) obj->rotation += PI*2;
	while(obj->rotation > PI*2) obj->rotation -= PI*2;

	obj->pos = Math_Vec2AddVec2(obj->pos, Math_Vec2MultFloat(chr->vel, game->deltatime));
	obj->pos = Math_Vec2AddVec2(obj->pos, Math_Vec2MultFloat(chr->shootTileVel, game->deltatime));

	// ParticlesDust(obj, game);

	World_ResolveCollisions(game, &game->world, obj);

    chr->lastVel = chr->vel;

	if(chr->onSticky == CHARACTER_STICKY_STUCK){

		if(chr->collidingSticky == CHARACTER_STICKY_NONE ||
			fabs(obj->rotation - PI) > STICKY_MAX_ROTATION){
    		chr->onSticky = CHARACTER_STICKY_NONE;
    	}
	}

	// World_BVH_Update(&game->world, obj);
	World_QuadtreeUpdate(&game->world, obj);

	u32 k;
	for(k = 0; k < game->nParticles; k++){
    	Particle_t *p = &game->particles[k];

    	if(!(p->flags & PARTICLE_FLAG_PUSH)){
    		continue;
    	}

        Vec2 vec = Math_Vec2SubVec2(p->pos, obj->pos);

        float mag = Math_Vec2Magnitude(vec);

        if(mag != 0){

	        Vec2 force = (Vec2){0,0};

	        vec = Math_Vec2Normalize(vec);

	        force = Math_Vec2AddVec2(force, Math_Vec2MultFloat((Vec2){-vec.y, vec.x}, RTGUY_VORTEX_FORCE_ANGULAR / mag)); 

	        force = Math_Vec2AddVec2(force, Math_Vec2MultFloat(vec, VORTEX_FORCE / mag)); 
	    	p->vel = Math_Vec2AddVec2(p->vel, Math_Vec2MultFloat(force, game->deltatime));
        }
	}

	if(obj == game->rotateguy){
	    game->camx = obj->pos.x - PLAYER_OFFSET_RIGHT;
	    game->camy = 0;
	}
}

static void Update(Object_t *obj, Game_t *game){

    Character_t *chr = (Character_t *)obj->data;

    if(chr->state == CHARACTER_STATE_DEAD){

    	UpdateDead(obj, game);

    } else {

    	UpdateAlive(obj, game);
    }


}