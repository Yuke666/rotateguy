#ifndef CHARACTER_DEF
#define CHARACTER_DEF

#include "types.h"
#include "image_loader.h"
#include "math.h"

#define MAX_CHARACTER_POINTS 		8

enum {
	CHARACTER_STICKY_NONE = 0,
	CHARACTER_STICKY_COLLISION,
	CHARACTER_STICKY_STUCK,
};
enum {
	CHARACTER_STATE_NONE = 0,
	CHARACTER_STATE_DEAD,
};

typedef struct Object_t Object_t;

typedef struct {

	u8				w;
	u8				h;

	u8 				sw;
	u8 				sh;

	u8 				state;
	u8 				onSticky;
	u8				collidingSticky;
	u8				remainingJumps;

	u8 				rotatingCCW;
	u8 				rotatingCW;

	u8 				dustTile;

	u32 			deathTime;

	float 			lastDustX;
	float 			angMomentum;

	Vec2 			dustResolve;
	Vec2 			lastVel;

	Vec2 			respawnPos;

	Object_t 		*lastObjCollision;

	Vec2			vel;
	Vec2			shootTileVel;

	u32 			texture;

} Character_t;

typedef struct Game_t Game_t;

Object_t *Character_New(Game_t *game, Vec2 pos, u32 tex);

#endif