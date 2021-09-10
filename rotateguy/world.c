#include <GL/glew.h>
#include <string.h>
#include "graphics.h"
#include "memory.h"
#include "game.h"
#include "world.h"
#include "log.h"
#include "object.h"


#define QUADTREE_SIZE MAX(LEVEL_WIDTH << TILE_SIZE_BITS, LEVEL_HEIGHT << TILE_SIZE_BITS)
#define QUADTREE_HEIGHT (LEVEL_HEIGHT << TILE_SIZE_BITS)
#define QUADTREE_WIDTH (LEVEL_WIDTH << TILE_SIZE_BITS)


static void                 TileObject_Event(Object_t *obj, Game_t *game, u32 event, u64 arg1, u64 arg2);
static QuadtreeLink_t*      CreateQuadtreeLink(World_t *world, Object_t *obj, Rect2D rect, u8 tile);
static void                 RemoveQuadtreeLink(World_t *world, QuadtreeLink_t *link);
static void                 QuadtreeRemoveLink(QuadtreeLink_t *link);
static inline void          Ref_QuadtreeLink(QuadtreeLink_t *link);
static void                 Unref_QuadtreeLink(QuadLeaf_t *top, QuadtreeLink_t *link);
static void                 Quadtree_LeafInit(World_t *world, QuadLeaf_t **leafPtr, QuadLeaf_t *parent, u8 index, u8 level);
static void                 Quadtree_Init(World_t *world);
static int                  Quadtree_LeafGetIndex(QuadLeaf_t *q, Rect2D rect);
static void                 Quadtree_LeafInsert(QuadLeaf_t *q, QuadtreeLink_t *link);
static void                 Quadtree_LeafEvent(QuadLeaf_t *q, QuadLeaf_t *top, Game_t *game, Rect2D rect, u32 type, u64 arg1, u64 arg2);
static Shape_t              GetTileRectShape(Rect2D rect, u8 tile);
static void                 Quadtree_LeafFree(QuadLeaf_t *leaf);
static void                 Quadtree_LeafResolveCollisions(QuadLeaf_t *q, QuadLeaf_t *top, Game_t *game, Object_t *obj, Shape_t *shape, Rect2D min);
static void                 Quadtree_LeafGetResolve(QuadLeaf_t *q, Shape_t *shape, Rect2D rect, Vec2 *resolve);


static void TileObject_Event(Object_t *obj, Game_t *game, u32 event, u64 arg1, u64 arg2){

    if(event == EV_SETACTIVE){

        obj->active = arg1;

    } else if(event == EV_RENDER && obj->active){

        Graphics_RenderTile(obj->shape.pos.x + (TILE_SIZE/2), obj->shape.pos.y + (TILE_SIZE/2), TILE_SIZE, TILE_SIZE, 
            obj->tile, game->tilesTexture, 0xFF, 0xFF, 0xFF, 0xFF);
    }
}

static QuadtreeLink_t *CreateQuadtreeLink(World_t *world, Object_t *obj, Rect2D rect, u8 tile){

    if(!world->firstFreeQuadtreeLink){
        LOG(LOG_RED, "OUT OF FREE QUADTREE LINKS");
        return NULL;
    }

    QuadtreeLink_t *link = world->firstFreeQuadtreeLink;
    
    world->firstFreeQuadtreeLink = world->firstFreeQuadtreeLink->next;

    link->obj = obj;
    link->tile = tile;
    link->rect = rect;
    link->update = 0;
    link->ref = 0;
    link->next = NULL;
    link->prev = NULL;
    link->leaf = NULL;

    return link;
}

static void RemoveQuadtreeLink(World_t *world, QuadtreeLink_t *link){

    link->next = world->firstFreeQuadtreeLink;

    world->firstFreeQuadtreeLink = link;    
}

static void QuadtreeRemoveLink(QuadtreeLink_t *link){
    if(link->next){ link->next->prev = link->prev; }
    if(link->prev){ link->prev->next = link->next; }
    if(link->leaf && link->leaf->list == link){ link->leaf->list = link->next; }
    link->leaf = NULL;
}

static inline void Ref_QuadtreeLink(QuadtreeLink_t *link){
    ++link->ref;
}

static void Unref_QuadtreeLink(QuadLeaf_t *top, QuadtreeLink_t *link){

    if(--link->ref == 0){

        QuadtreeRemoveLink(link);

        if(link->update) {
            Quadtree_LeafInsert(top, link);
            link->update = 0;
        }
    }
}

static void Quadtree_LeafInit(World_t *world, QuadLeaf_t **leafPtr,
        QuadLeaf_t *parent, u8 index, u8 level){

    if(world->onLeaf >= NUM_TOTAL_QUADS){
        LOG(LOG_RED, "ERROR: Quadtree_LeafInit: OUT OF USABLE QUADS (MAX IS %i)", NUM_TOTAL_QUADS);
        return;
    }

    *leafPtr = &world->quadtreeLeafs[world->onLeaf++];

    QuadLeaf_t *leaf = *leafPtr;

    leaf->list = NULL;

    leaf->size = QUADTREE_SIZE;
    leaf->xpos = 0;
    leaf->ypos = 0;

    if(parent != NULL){
        leaf->size = parent->size/2;
        leaf->xpos = parent->xpos + ((index % 2) * leaf->size);
        leaf->ypos = parent->ypos + (round((index % 4) / 2) * leaf->size);
    }

    if(level <= QUADTREE_LEVELS && leaf->xpos < QUADTREE_WIDTH && leaf->ypos < QUADTREE_HEIGHT){

        u32 k;
        for(k = 0; k < 4; k++)
            Quadtree_LeafInit(world, &leaf->children[k], leaf, k, level+1);
    }

}

static void Quadtree_Init(World_t *world){

    world->onLeaf = 0;
    Quadtree_LeafInit(world, &world->quadtree, NULL, 0, 0);


    u32 k;
    for(k = 0; k < NUM_QUADTREE_LINKS-1; k++)
        world->quadtreeLinks[k].next = &world->quadtreeLinks[k+1];
    
    world->quadtreeLinks[k].next = NULL;

    world->firstFreeQuadtreeLink = &world->quadtreeLinks[0];
}


static int Quadtree_LeafGetIndex(QuadLeaf_t *q, Rect2D rect){

    u32 ySplit = q->ypos + (q->size/2);
    u32 xSplit = q->xpos + (q->size/2);

    if(rect.x + rect.w < xSplit){
        if(rect.y + rect.h < ySplit) return 0;
        if(rect.y > ySplit) return 2;
    }

    if(rect.x > xSplit){
        if(rect.y + rect.h < ySplit) return 1;
        if(rect.y > ySplit) return 3;
    }

    return -1;
}

static void Quadtree_LeafInsert(QuadLeaf_t *q, QuadtreeLink_t *link){
    
    
    if(q->children[0]){

        int index = Quadtree_LeafGetIndex(q, link->rect);

        if(index != -1){
            Quadtree_LeafInsert(q->children[index], link);
            return;
        }
    }
    
    Ref_QuadtreeLink(link);

    link->next = q->list;

    if(q->list) q->list->prev = link;

    q->list = link;

    link->leaf = q;
}

static void Quadtree_LeafEvent(QuadLeaf_t *q, QuadLeaf_t *top, Game_t *game, 
    Rect2D rect, u32 type, u64 arg1, u64 arg2){

    Rect2D qrect = (Rect2D){q->xpos, q->ypos, q->size, q->size};

    if(!Math_CheckCollisionRect2D(rect, qrect))
        return;


    QuadtreeLink_t *currLink = q->list;
    QuadtreeLink_t *next;

    while(currLink){
        
        next = currLink->next;

        Ref_QuadtreeLink(currLink);
        
        if(currLink->obj && currLink->obj->Event)
            currLink->obj->Event(currLink->obj, game, type, arg1, arg2);

        Unref_QuadtreeLink(top, currLink);
        
        currLink = next;
    }

    if(q->children[0]){

        u32 k;
        for(k = 0; k < 4; k++){
            Quadtree_LeafEvent(q->children[k], top, game, rect, type, arg1, arg2);
        }
    }
}

static Shape_t GetTileRectShape(Rect2D rect, u8 tile){

    Shape_t shape;
    shape.rotation = 0;
    shape.pos = (Vec2){rect.x,rect.y};
    shape.nPoints = 0;


    if(tile != TILE_NONE){

        if(tile == TILE_SPIKE_UP){
            
            shape.nPoints = 3;

            shape.points[0] = (Vec2){0, 0+rect.h};
            shape.points[1] = (Vec2){(rect.w/2), 0};
            shape.points[2] = (Vec2){rect.w, 0+rect.h};

        } else if(tile == TILE_SPIKE_RIGHT){
            
            shape.nPoints = 3;

            shape.points[0] = (Vec2){0, 0};
            shape.points[1] = (Vec2){+rect.w, 0 + (rect.h/2)};
            shape.points[2] = (Vec2){0, 0+rect.h};

        } else if(tile == TILE_SPIKE_LEFT){
            
            shape.nPoints = 3;

            shape.points[0] = (Vec2){+rect.w, 0};
            shape.points[1] = (Vec2){0, 0 + (rect.h/2)};
            shape.points[2] = (Vec2){+rect.w, 0+rect.h};

        } else if(tile == TILE_SPIKE_DOWN){

            shape.nPoints = 3;

            shape.points[0] = (Vec2){0, 0};
            shape.points[1] = (Vec2){+(rect.w/2), 0+rect.h};
            shape.points[2] = (Vec2){+rect.w, 0};

        } else {
            
            shape.nPoints = 4;

            shape.points[0] = (Vec2){0, 0};
            shape.points[1] = (Vec2){+rect.w, 0};
            shape.points[2] = (Vec2){+rect.w, 0+rect.h};
            shape.points[3] = (Vec2){0, 0+rect.h};
        }
    }

    return shape;
}

static void Quadtree_LeafResolveCollisions(QuadLeaf_t *q, QuadLeaf_t *top, Game_t *game,
    Object_t *obj, Shape_t *shape, Rect2D rect){


    Rect2D qrect = (Rect2D){q->xpos, q->ypos, q->size, q->size};

    if(!Math_CheckCollisionRect2D(rect, qrect))
        return;


    Shape_t linkShape;

    QuadtreeLink_t *currLink = q->list;
    QuadtreeLink_t *next;


    while(currLink){

            
        if(!Math_CheckCollisionRect2D(currLink->rect, rect)){
            currLink = currLink->next;
            continue;
        }

        next = currLink->next;

        Ref_QuadtreeLink(currLink);

        Vec2 resolveVec;
        
        if(currLink->obj){

            if(currLink->obj != obj && currLink->obj->active){

                if(SATShapeShape(shape, &currLink->obj->shape, &resolveVec)){

                    if(obj->OnCollision)
                        obj->OnCollision(obj, game, currLink->obj, resolveVec);
                }
            }

        } else {

            linkShape = GetTileRectShape(currLink->rect, currLink->tile);

            if(SATShapeShape(shape, &linkShape, &resolveVec)){

                // if((TileFlags[currLink->tile] & TILE_MASK_NO_COLLISION))
                //     resolveVec = (Vec2){0,0};

                if(obj->OnCollisionStatic)
                    obj->OnCollisionStatic(obj, game, currLink->tile, resolveVec);

            }
        }

        Unref_QuadtreeLink(top, currLink);
        
        currLink = next;
    }

    if(q->children[0]){

        u32 k;
        for(k = 0; k < 4; k++){
            Quadtree_LeafResolveCollisions(q->children[k], top, game, obj, shape, rect);
        }
    }
}

static void Quadtree_LeafGetResolve(QuadLeaf_t *q, Shape_t *shape, Rect2D rect, Vec2 *resolve){


    Rect2D qrect = (Rect2D){q->xpos, q->ypos, q->size, q->size};

    if(!Math_CheckCollisionRect2D(rect, qrect))
        return;


    Shape_t linkShape;

    QuadtreeLink_t *currLink = q->list;

    while(currLink){

        if(!Math_CheckCollisionRect2D(currLink->rect, rect)){
            currLink = currLink->next;
            continue;
        }

        Vec2 resolveVec;
        
        if(currLink->obj){

            if(currLink->obj->active){

                if(SATShapeShape(shape, &currLink->obj->shape, &resolveVec)){

                    // if(Math_Vec2Magnitude(resolveVec) > Math_Vec2Magnitude(*resolve))
                        *resolve = resolveVec;
                    return;
                }
            }

        } else {

            linkShape = GetTileRectShape(currLink->rect, currLink->tile);

            if(SATShapeShape(shape, &linkShape, &resolveVec)){

                // if(Math_Vec2Magnitude(resolveVec) > Math_Vec2Magnitude(*resolve))
                    *resolve = resolveVec;
                return;
            }
        }
        
        currLink = currLink->next;
    }

    if(q->children[0]){

        u32 k;
        for(k = 0; k < 4; k++){
            Quadtree_LeafGetResolve(q->children[k], shape, rect, resolve);
        }
    }
}

static void Quadtree_LeafFree(QuadLeaf_t *leaf){

    leaf->list = NULL;

    if(!leaf->children[0]) return;

    u32 k;
    for(k = 0; k < 4; k++){
        Quadtree_LeafFree(leaf->children[k]);
    }
}

void World_GlobalEvent(Game_t *game, World_t *world, u32 type, u64 arg1, u64 arg2){
    
    Object_t *obj;

    u32 k;
    for(k = MAX_OBJECTS-1; k > world->freeListIndex; k--){
        obj = &world->objects[world->freeList[k]];
        obj->Event(obj, game, type, arg1, arg2);
    }
}

void World_QuadtreeRemove(Object_t *obj){

    obj->quadtreeLink->update = 0;

    Unref_QuadtreeLink(NULL, obj->quadtreeLink);
}

void World_QuadtreeUpdate(World_t *world, Object_t *obj){

// #ifdef DEBUG
//     if(obj->quadtreeLink->ref > 1){
//         LOG(LOG_YELLOW, "ATTEMPTED TO UPDATE OBJECT IN QUADTREE WHILE REFERENCE OF LINK > 1");
//     }
// #endif

    obj->quadtreeLink->rect = ShapeAABB(&obj->shape);
    obj->quadtreeLink->tile = obj->tile;


    if(obj->quadtreeLink->ref == 0){

        Quadtree_LeafInsert(world->quadtree, obj->quadtreeLink);

    } else {

        obj->quadtreeLink->update = 1;
        
        Unref_QuadtreeLink(world->quadtree, obj->quadtreeLink);
    }

}

void World_LocalEvent(Game_t *game, World_t *world, Rect2D rect, u32 type, u64 arg1, u64 arg2){

    Quadtree_LeafEvent(world->quadtree, world->quadtree, game, rect, type, arg1, arg2);
}

void World_ResolveCollisions(Game_t *game, World_t *world, Object_t *obj){

    Rect2D rect = ShapeAABB(&obj->shape);

    Quadtree_LeafResolveCollisions(world->quadtree, world->quadtree, game, obj, &obj->shape, rect);
}

void World_GetShapeCollisionResolve(World_t *world, Shape_t *shape, Vec2 *resolve){

    *resolve = (Vec2){0,0};
    
    Rect2D rect = ShapeAABB(shape);

    Quadtree_LeafGetResolve(world->quadtree, shape, rect, resolve);
}

Object_t *World_NewObject(World_t *world){

    if(world->freeListIndex == 0)
        return NULL;

    u32 handle = world->freeList[world->freeListIndex];

    --world->freeListIndex;

    Object_t *ret = &world->objects[handle];

    ret->handle = handle;
    ret->active = 1;

    ret->quadtreeLink = CreateQuadtreeLink(world, ret, (Rect2D){0,0,0,0}, TILE_NONE);

    return ret;
}

void World_RemoveObject(Game_t *game, World_t *world, u32 handle){

    if(world->freeListIndex == MAX_OBJECTS-1){
        LOG(LOG_YELLOW, "ATTEMPTED TO REMOVE OBJECT WHEN NO USED OBJECTS");
    }
    
    if(handle > MAX_OBJECTS){
        LOG(LOG_YELLOW, "ATTEMPTED TO REMOVE HANDLE OVER MAX_OBJECTS");
    }

    if(world->objects[handle].active == 0){
        LOG(LOG_YELLOW, "ATTEMPTED TO REMOVE OBJECT NON ACTIVE OBJECT");
    }

    world->objects[handle].Event(&world->objects[handle], game, EV_OBJDESTROY, 0, 0);

    World_QuadtreeRemove(&world->objects[handle]);

    RemoveQuadtreeLink(world, world->objects[handle].quadtreeLink);

    World_GlobalEvent(game, world, EV_DESTROY, handle, 0);

    ++world->freeListIndex;

    world->freeList[world->freeListIndex] = handle;
}

void World_LoadLevel(World_t *world, u8 *data, u32 width, u32 height, Texture_t texture){

    /* reset free list */

    world->freeListIndex = MAX_OBJECTS-1;

    u32 k;
    for(k = 0; k < MAX_OBJECTS; k++)
        world->freeList[k] = k;


    /* count number of rendered tiles */

    u32 nRenderedTiles = 0;

    u32 x, y;
    for(y = 0; y < height; y++){

        for(x = 0; x < width; x++){

            int index = (y * width) + x;

            if(IS_RENDERED_TILE(data[index]))
                continue;

            ++nRenderedTiles;
        }
    }


    /* setup vao for rendering */

    glGenVertexArrays(1, &world->vao);
    glBindVertexArray(world->vao);

    glGenBuffers(1, &world->posVbo);
    glGenBuffers(1, &world->uvVbo);

    glEnableVertexAttribArray(POS_LOC);
    glEnableVertexAttribArray(UV_LOC);

    glBindBuffer(GL_ARRAY_BUFFER,world->uvVbo);
    glVertexAttribPointer(UV_LOC, 2, GL_FLOAT, GL_FALSE, 0, 0);

    glBindBuffer(GL_ARRAY_BUFFER,world->posVbo);
    glVertexAttribPointer(POS_LOC, 2, GL_FLOAT, GL_FALSE, 0, 0);

    glGenBuffers(1, &world->ebo);

    glBindVertexArray(world->vao);

    glBindBuffer(GL_ARRAY_BUFFER,world->uvVbo);
    glBufferData(GL_ARRAY_BUFFER,nRenderedTiles*4*sizeof(Vec2), NULL, GL_STATIC_DRAW);

    glBindBuffer(GL_ARRAY_BUFFER,world->posVbo);
    glBufferData(GL_ARRAY_BUFFER, nRenderedTiles*4*sizeof(Vec2), NULL, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, world->ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, nRenderedTiles*6*sizeof(int), NULL, GL_STATIC_DRAW);

    /* 6 elements per tile */
    world->nElements = nRenderedTiles * 6;

    /* fill the buffers */

    u32 nTilesTextureX = texture.w / TILE_SIZE;

    /* 0.0-1.0 offset each tile gives in texture */

    float invTexWidth = 1.0f / (texture.w / TILE_SIZE);
    float invTexHeight = 1.0f / (texture.h / TILE_SIZE);

    /* temp data */
    Vec2 coords[4], verts[4];
    int elements[] = {0, 1, 2, 2, 3, 0};

    u32 tileOffset = 0;

    for(y = 0; y < height; y++){

        for(x = 0; x < width; x++){

            int index = (y * width) + x;

            if(IS_RENDERED_TILE(data[index]))
                continue;

            u32 tile = data[index]-1;

            float texX = tile % nTilesTextureX;
            float texY = tile / nTilesTextureX;

            verts[0] = (Vec2){x * TILE_SIZE,        y * TILE_SIZE};
            verts[1] = (Vec2){verts[0].x+TILE_SIZE, verts[0].y};
            verts[2] = (Vec2){verts[1].x,           verts[0].y+TILE_SIZE};
            verts[3] = (Vec2){verts[0].x,           verts[2].y};

            coords[0] = (Vec2){texX * invTexWidth, texY * invTexHeight};
            coords[1] = (Vec2){(texX+1) * invTexWidth, coords[0].y};
            coords[2] = (Vec2){coords[1].x, (texY+1) * invTexHeight};
            coords[3] = (Vec2){coords[0].x, coords[2].y};

            glBindBuffer(GL_ARRAY_BUFFER,world->uvVbo);
            glBufferSubData(GL_ARRAY_BUFFER, tileOffset*4*sizeof(Vec2), 4*sizeof(Vec2), coords);
 
            glBindBuffer(GL_ARRAY_BUFFER,world->posVbo);
            glBufferSubData(GL_ARRAY_BUFFER, tileOffset*4*sizeof(Vec2), 4*sizeof(Vec2), verts);

            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, world->ebo);
            glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, tileOffset*6*sizeof(int), 6*sizeof(int), elements);

            u32 k;
            for(k = 0; k < 6; k++)
                elements[k] += 4;

            ++tileOffset;
        }
    }

    glBindVertexArray(0);

    /* set the sprite sheet */

    world->tilesTexture = texture;

    /* initialize the quadtree */

    Quadtree_Init(world);

    /* build the static collisions */

    u8 *closed = (u8 *)Memory_Alloc(1, width * height);
    memset(closed, 0, width * height);

    /* start with the invisible collisions */
    
    for(y = 0; y < height; y++){

        for(x = 0; x < width; x++){

            u32 index = (y * width) + x;

            if(data[index] == 0 || !(TileFlags[data[index]] & TILE_MASK_INVISIBLE) || closed[index])
                continue;

            u32 block = data[index];

            u32 rectHeight = 0;
            u32 rectWidth = 0;
            
            u32 k, j;
            for(k = y; k < height; k++){

                u32 index = (k * width) + x;
            
                if(closed[index]) break;

                if(data[index] == block)
                    ++rectHeight;
                else
                    break;
            }

            for(k = x; k < width; k++){

                u32 tile = block;

                for(j = y; j < y + rectHeight; j++){

                    u32 index = (j * width) + k;
                
                    if(closed[index]){
                        tile = TILE_NONE;
                        break;
                    }

                    tile = data[index];

                    if(tile == block)
                        break;
                }

                if(tile == block)
                    ++rectWidth;
                else
                    break;
            }

            for(k = y; k < y + rectHeight; k++)
                for(j = x; j < x + rectWidth; j++)
                    closed[(k * width) + j] = 1;

            Rect2D aabb = (Rect2D){0,0,0,0};

            aabb.x = x * TILE_SIZE;
            aabb.y = y * TILE_SIZE;

            if((TileFlags[data[index]] & TILE_MASK_ABOVE_INVISIBLE)){
                aabb.y -= TILE_SIZE;                
            } else if((TileFlags[data[index]] & TILE_MASK_BELOW_INVISIBLE)){
                aabb.y += TILE_SIZE;                
            }

            aabb.w = TILE_SIZE * rectWidth;
            aabb.h = TILE_SIZE * rectHeight;

            QuadtreeLink_t *link = CreateQuadtreeLink(world, NULL, aabb, block);

            Quadtree_LeafInsert(world->quadtree, link);
        }
    }

    /* reset closed list and build the normal collisions */

    memset(closed, 0, width * height);

    for(y = 0; y < height; y++){

        for(x = 0; x < width; x++){

            u32 index = (y * width) + x;

            if(data[index] == 0 || closed[index]) continue;

            Rect2D aabb = (Rect2D){0,0,0,0};

            if(!(TileFlags[data[index]] & TILE_MASK_16x16_COLLISION)){

                aabb = (Rect2D){x * TILE_SIZE, y * TILE_SIZE, TILE_SIZE, TILE_SIZE};

            } else {

                u32 block = data[index];

                u32 rectHeight = 0;
                u32 rectWidth = 0;
                
                u32 k, j;
                for(k = y; k < height; k++){

                    u32 index = (k * width) + x;
                
                    if(closed[index]) break;

                    // if((TileFlags[data[index]] & TILE_MASK_16x16_COLLISION))
                    if(data[index] == block)
                        ++rectHeight;
                    else
                        break;
                }

                for(k = x; k < width; k++){

                    u32 tile = block;

                    for(j = y; j < y + rectHeight; j++){

                        u32 index = (j * width) + k;
                    
                        if(closed[index]){
                            tile = TILE_NONE;
                            break;
                        }

                        tile = data[index];

                        // if(!(TileFlags[tile] & TILE_MASK_16x16_COLLISION))
                        if(tile != block)
                            break;
                    }

                    // if((TileFlags[tile] & TILE_MASK_16x16_COLLISION))
                    if(tile == block)
                        ++rectWidth;
                    else
                        break;
                }


                for(k = y; k < y + rectHeight; k++)
                    for(j = x; j < x + rectWidth; j++)
                        closed[(k * width) + j] = 1;

                aabb.x = x * TILE_SIZE;
                aabb.y = y * TILE_SIZE;

                aabb.w = TILE_SIZE * rectWidth;
                aabb.h = TILE_SIZE * rectHeight;

            }

            if((TileFlags[data[index]] & TILE_MASK_CREATE_OBJECT)){
                
                Object_t *obj = World_NewObject(world);

                obj->tile = data[index];
                obj->shape = GetTileRectShape(aabb, obj->tile);
                obj->Event = TileObject_Event;

                World_QuadtreeUpdate(world, obj);
    
            } else {
    

                QuadtreeLink_t *link = CreateQuadtreeLink(world, NULL, aabb, data[index]);

                Quadtree_LeafInsert(world->quadtree, link);
            }

        }
    }


    /* pop the closed list */
    
    Memory_Pop(1,1);

}

void World_DeleteLevel(World_t *world){
    glDeleteBuffers(1, &world->posVbo);
    glDeleteBuffers(1, &world->ebo);
    glDeleteBuffers(1, &world->uvVbo);
    glDeleteVertexArrays(1, &world->vao);

    Quadtree_LeafFree(world->quadtree);
    // Memory_Pop(QUADTREE_MEM_STACK, 1);
}

static void DrawQuad(QuadLeaf_t *leaf, Rect2D sRect, u32 level, u32 index){


    Rect2D qrect = (Rect2D){leaf->xpos, leaf->ypos, leaf->size, leaf->size};

    if(!Math_CheckCollisionRect2D(sRect, qrect))
        return;


    glLineWidth((QUADTREE_LEVELS - (level-1))*2);

    Graphics_RenderRectLines(leaf->xpos,leaf->ypos,leaf->size,leaf->size, 0xFF,0,0,0xFF);

    if(!leaf->children[0]) return;

    u32 k;
    for(k = 0; k < 4; k++){
        DrawQuad(leaf->children[k], sRect, level+1, k+1);
    }
}

void World_DrawQuadtree(World_t *world, Rect2D renderRect){

    DrawQuad(world->quadtree, renderRect, 0, 0);
}

void World_DrawLevel(World_t *world){

    Graphics_UseShader(TEXTURED_SHADER);
    glBindVertexArray(world->vao);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, world->tilesTexture.texture);

    glDrawElements(GL_TRIANGLES, world->nElements, GL_UNSIGNED_INT, 0);
}