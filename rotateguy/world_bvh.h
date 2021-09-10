#ifndef WORLD_DEF
#define WORLD_DEF

#include "types.h"
#include "object.h"
#include "image_loader.h"

#define MAX_OBJECTS 		1024
#define MAX_BVH_NODES       1024

struct BVH_Node_t {

    BVH_Node_t              *left;
    BVH_Node_t              *right;

    Object_t                *data;
    u8						tile;

    union {
        BVH_Node_t          *parent;
        BVH_Node_t          *next;
    };

    Rect2D                  aabb;

};

typedef struct {
    BVH_Node_t              bvh[MAX_BVH_NODES];
    BVH_Node_t              *firstFree;
    BVH_Node_t              *root;
    u32                     nUsedNodes;
} AABB_Tree_t;

typedef struct {
	u32 					objFreeList[MAX_OBJECTS];
	u32 					objFreeListIndex;
	Object_t 				objects[MAX_OBJECTS];
	u32 					vao;
	u32 					posVbo;
	u32 					uvVbo;
	u32 					ebo;
	u32 					nElements;
	Texture_t 				tilesTexture;
	AABB_Tree_t 			aabbTree;
} World_t;

Object_t 	*World_NewObject(World_t *world);
void 		World_RemoveObject(Game_t *game, World_t *world, u32 handle);
void 		World_Init(World_t *world);
void 		World_DeleteLevel(World_t *world);
void 		World_DrawLevel(World_t *world);
void 		World_LoadLevel(World_t *world, u8 *data, u32 width, u32 height, Texture_t texture);
void 		World_ResolveCollisions(Game_t *game, World_t *world, Object_t *obj);
void 		World_GlobalEvent(Game_t *game, World_t *world, u32 type, u64 arg1, u64 arg2);
void 		World_LocalEvent(Game_t *game, World_t *world, Rect2D minRect, u32 type, u64 arg1, u64 arg2);
void 		World_BVH_Add(World_t *world, Object_t *obj);
void 		World_BVH_Remove(World_t *world, Object_t *obj);
void 		World_BVH_Update(World_t *world, Object_t *obj);

#endif