#ifndef CHARACTER_DEF
#define CHARACTER_DEF

#include "rotateguy.h"
#include "math.h"

enum {
	FLAG_ONGROUND = 0x1,
};

typedef struct {

	u8			w;
	u8			h;

	u8 			sx;
	u8 			sy;
	u8 			sw;
	u8 			sh;

	u8			flags;
	u8			remainingJumps;

	float 		rotation;
	float 		angMomentum;

	Vec2		pos;
	Vec2		acc;
	Vec2		vel;



} Character_t;

typedef struct Level_t Level_t;

void Character_Event(Character_t *chr, Level_t *lvl, u32 event, EventArg_t *args, u8 nArgs);
void Character_Render(Character_t *chr, Level_t *lvl);
void Character_Update(Character_t *chr, Level_t *lvl, float dt);
void Character_Init(Character_t *chr);

#endif