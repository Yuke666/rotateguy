#ifndef OBJECT_DEF
#define OBJECT_DEF

#include "types.h"
#include "math.h"

#define MAX_OBJECT_DATA_SIZE 1024

typedef struct Game_t Game_t;
typedef struct Object_t Object_t;
// typedef struct BVH_Node_t BVH_Node_t;
typedef struct QuadtreeLink_t QuadtreeLink_t;

enum {
	EV_NONE = 0,
	EV_RENDER,
	EV_OBJUPDATE,
	EV_LOCALUPDATE,
	EV_GLOBALUPDATE,
	EV_OBJDESTROY,
	EV_DESTROY,
	EV_SETOWNER,
	EV_SETPOS,
	EV_SETVEL,
	EV_SETROTATION,
	EV_JUMP,
	EV_ROTATE_CW,
	EV_ROTATE_CCW,
	EV_STOP_ROTATE_CW,
	EV_STOP_ROTATE_CCW,
	EV_SETACTIVE,
	EV_RESPAWN,
	EV_COLLISION,
	NUM_EVENTS,
};

struct Object_t {

	u8 								active;
	u8 								tile;

	u32 							handle;

	union {
		Shape_t 					shape;
		struct {
			SHAPE_STRUCTURE
		};
	};

	void (*OnCollision)				(Object_t *this, Game_t *game, Object_t *obj, Vec2 resolve);
	void (*OnCollisionStatic)		(Object_t *this, Game_t *game, u8 tile, Vec2 resolve);
	void (*Event)					(Object_t *this, Game_t *game, u32 type, u64 arg1, u64 arg2);

	// BVH_Node_t 						*node;
	QuadtreeLink_t					*quadtreeLink;

	u8 								data[MAX_OBJECT_DATA_SIZE];
};

#endif