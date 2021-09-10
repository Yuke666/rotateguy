#ifndef WORLD_DEF
#define WORLD_DEF

#include "types.h"
#include "object.h"
#include "image_loader.h"

#define MAX_OBJECTS 		512
#define MAX_STATIC_OBJECTS  4096
#define NUM_QUADTREE_LINKS (MAX_OBJECTS + MAX_STATIC_OBJECTS)
#define QUADTREE_LEVELS     6
#define NUM_TOTAL_QUADS     1024

typedef struct QuadtreeLink_t QuadtreeLink_t;
typedef struct QuadLeaf_t QuadLeaf_t;

struct QuadtreeLink_t {
    u8                      tile;
    u8                      update;
    u8                      ref;
    Rect2D                  rect;
    Object_t                *obj;
    QuadLeaf_t              *leaf;
    QuadtreeLink_t          *next;
    QuadtreeLink_t          *prev;
};

struct QuadLeaf_t {
    s32                     xpos;
    s32                     ypos;
    u32                     size;
    QuadtreeLink_t          *list;
    QuadLeaf_t              *children[4];
};

typedef struct {

	/* objects */
    u32 					freeList[MAX_OBJECTS];
	u32 					freeListIndex;
	Object_t 				objects[MAX_OBJECTS];
	
    /* quadtree links */
    QuadtreeLink_t          quadtreeLinks[NUM_QUADTREE_LINKS];
    QuadtreeLink_t          *firstFreeQuadtreeLink;

    /* renderering data */
    u32 					vao;
	u32 					posVbo;
	u32 					uvVbo;
	u32 					ebo;
	u32 					nElements;
	Texture_t 				tilesTexture;

    /* quadtree */
    QuadLeaf_t              quadtreeLeafs[NUM_TOTAL_QUADS];
    u32                     onLeaf;
    QuadLeaf_t              *quadtree;

} World_t;

Object_t 	*World_NewObject(World_t *world);
void 		World_RemoveObject(Game_t *game, World_t *world, u32 handle);
void 		World_DeleteLevel(World_t *world);
void 		World_DrawLevel(World_t *world);
void        World_DrawQuadtree(World_t *world, Rect2D renderRect);
void 		World_LoadLevel(World_t *world, u8 *data, u32 width, u32 height, Texture_t texture);
void 		World_ResolveCollisions(Game_t *game, World_t *world, Object_t *obj);
void 		World_GlobalEvent(Game_t *game, World_t *world, u32 type, u64 arg1, u64 arg2);
void 		World_LocalEvent(Game_t *game, World_t *world, Rect2D minRect, u32 type, u64 arg1, u64 arg2);
void        World_QuadtreeRemove(Object_t *obj);
void        World_QuadtreeUpdate(World_t *world, Object_t *obj);
void        World_GetShapeCollisionResolve(World_t *world, Shape_t *shape, Vec2 *resolve);

#endif