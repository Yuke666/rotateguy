#include <GL/glew.h>
#include <string.h>
#include "graphics.h"
#include "memory.h"
#include "game.h"
#include "world.h"
#include "log.h"
#include "object.h"

#define QUADTREE_MAX_OBJECTS 8
#define QUADTREE_MEM_STACK 0
#define QUADTREE_SIZE MAX(LEVEL_WIDTH << TILE_SIZE_BITS, LEVEL_HEIGHT << TILE_SIZE_BITS)

struct QuadtreeLeaf_t {

    u8                      level;
    u8                      index;
    u8                      nCollisions;
    s32                     xpos;
    s32                     ypos;
    u32                     size;

    struct {
        Rect2D              rect;
        ConvexShape_t       shape;
        u8                  tile;
        Object_t            *object;
    } collisions[QUADTREE_MAX_OBJECTS];
        
    QuadtreeLeaf_t          *children[4];
};

static void Quadtree_LeafInit(QuadtreeLeaf_t *leaf, QuadtreeLeaf_t *parent, u8 index){

    leaf->nCollisions = 0;
    leaf->level = 0;
    leaf->index = index;

    leaf->size = QUADTREE_SIZE;
    leaf->xpos = 0;
    leaf->ypos = 0;

    if(parent != NULL){
        leaf->size = parent->size/2;
        leaf->level = parent->level+1;
        leaf->xpos = parent->xpos + ((index % 2) * leaf->size);
        leaf->ypos = parent->ypos + (round((index % 4) / 2) * leaf->size);
    }

}

static void Quadtree_LeafSplit(QuadtreeLeaf_t *leaf){

    u32 k;
    for(k = 0; k < 4; k++){
        
        leaf->children[k] = (QuadtreeLeaf_t *)Memory_Alloc(QUADTREE_MEM_STACK, sizeof(QuadtreeLeaf_t));
        
        Quadtree_LeafInit(leaf->children[k], leaf, k);
    }
}


static int Quadtree_LeafGetIndex(QuadtreeLeaf_t *q, Rect2D rect){

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

static void TileObject_Event(Object_t *obj, Game_t *game, u32 event, u64 arg1, u64 arg2){

    if(event == EV_SETACTIVE){

        obj->active = arg1;

    } else if(event == EV_RENDER && obj->active){

        Graphics_RenderTile(obj->shape.pos.x + (TILE_SIZE/2), obj->shape.pos.y + (TILE_SIZE/2), TILE_SIZE, TILE_SIZE, 
            obj->tile, game->tilesTexture, 0xFF, 0xFF, 0xFF, 0xFF);
    }
}

static void Quadtree_LeafInsert(QuadtreeLeaf_t *q, Object_t *obj, Rect2D rect, u8 tile){


    if(q->children[0]){

        int index = Quadtree_LeafGetIndex(q, rect);

        if(index != -1){
            Quadtree_LeafInsert(q->children[index], obj, rect, tile);
            return;
        }
    }

    if(q->nCollisions >= QUADTREE_MAX_OBJECTS){

        Quadtree_LeafSplit(q);

        int index = Quadtree_LeafGetIndex(q, rect);

        if(index != -1){
            Quadtree_LeafInsert(q->children[index], obj, rect, tile);
            return;
        }
    }

    printf("%i\n",q->nCollisions );

    ConvexShape_t *shape = obj ? &obj->shape : &q->collisions[q->nCollisions].shape;

    if(tile != TILE_NONE){

        shape->rotation = 0;
        shape->pos = (Vec2){rect.x,rect.y};
        shape->nPoints = 0;

        if(tile == TILE_SPIKE_UP){
            
            shape->nPoints = 3;

            shape->points[0] = (Vec2){0, 0+rect.h};
            shape->points[1] = (Vec2){(rect.w/2), 0};
            shape->points[2] = (Vec2){rect.w, 0+rect.h};

        } else if(tile == TILE_SPIKE_RIGHT){
            
            shape->nPoints = 3;

            shape->points[0] = (Vec2){0, 0};
            shape->points[1] = (Vec2){+rect.w, 0 + (rect.h/2)};
            shape->points[2] = (Vec2){0, 0+rect.h};

        } else if(tile == TILE_SPIKE_LEFT){
            
            shape->nPoints = 3;

            shape->points[0] = (Vec2){+rect.w, 0};
            shape->points[1] = (Vec2){0, 0 + (rect.h/2)};
            shape->points[2] = (Vec2){+rect.w, 0+rect.h};

        } else if(tile == TILE_SPIKE_DOWN){

            shape->nPoints = 3;

            shape->points[0] = (Vec2){0, 0};
            shape->points[1] = (Vec2){+(rect.w/2), 0+rect.h};
            shape->points[2] = (Vec2){+rect.w, 0};

        } else {
            
            shape->nPoints = 4;

            shape->points[0] = (Vec2){0, 0};
            shape->points[1] = (Vec2){+rect.w, 0};
            shape->points[2] = (Vec2){+rect.w, 0+rect.h};
            shape->points[3] = (Vec2){0, 0+rect.h};
        }

        if(obj){
            obj->Event = TileObject_Event;
        }
    }

    if(obj){
        // obj->inQuad = q;
        // obj->tile = tile;
        // q->collisions[q->nCollisions].object = obj;
        // q->collisions[q->nCollisions].rect = rect;
    } else {
        // q->collisions[q->nCollisions].object = NULL;
        // q->collisions[q->nCollisions].rect = rect;
        // q->collisions[q->nCollisions].tile = tile;
    }

    ++q->nCollisions;
}

static void Quadtree_LeafEvent(QuadtreeLeaf_t *q, Game_t *game, 
    Rect2D minRect, u32 type, u64 arg1, u64 arg2){

    Rect2D rect = (Rect2D){q->xpos, q->ypos, q->size, q->size};

    if(!Math_CheckCollisionRect2D(rect, minRect))
        return;

    u32 k;
    for(k = 0; k < q->nCollisions; k++){
        if(q->collisions[k].object)
            q->collisions[k].object->Event(q->collisions[k].object, game, type, arg1, arg2);
    }

    if(q->children[0]){

        for(k = 0; k < 4; k++){
            Quadtree_LeafEvent(q->children[k], game, minRect, type, arg1, arg2);
        }
    }
}

static void Quadtree_LeafResolveCollisions(QuadtreeLeaf_t *q, Game_t *game, Object_t *obj, ConvexShape_t *shape, Rect2D minRect){

    Rect2D rect = (Rect2D){q->xpos, q->ypos, q->size, q->size};

    if(!Math_CheckCollisionRect2D(rect, minRect))
        return;

    u32 k;
    for(k = 0; k < q->nCollisions; k++){

        if(!Math_CheckCollisionRect2D(q->collisions[k].rect, minRect)){
            continue;
        }

        Vec2 resolveVec;
        
        if(q->collisions[k].object && q->collisions[k].object != obj && q->collisions[k].object->active){

            if(SATShapeShape(shape, &q->collisions[k].object->shape, &resolveVec)){

                if(obj->OnCollision)
                    obj->OnCollision(obj, game, q->collisions[k].object, resolveVec);
            }

        } else {

            if(SATShapeShape(shape, &q->collisions[k].shape, &resolveVec)){

                if((TileFlags[q->collisions[k].tile] & TILE_MASK_NO_COLLISION))
                    resolveVec = (Vec2){0,0};

                if(obj->OnCollisionStatic)
                    obj->OnCollisionStatic(obj, game, q->collisions[k].tile, resolveVec);

            }
        }
    }

    if(q->children[0]){

        for(k = 0; k < 4; k++){
            Quadtree_LeafResolveCollisions(q->children[k], game, obj, shape, minRect);
        }
    }
}

static void Quadtree_LeafFree(QuadtreeLeaf_t *leaf){

    if(!leaf->children[0]) return;

    u32 k;
    for(k = 0; k < 4; k++){
        Quadtree_LeafFree(leaf->children[k]);
    }

    Memory_Pop(QUADTREE_MEM_STACK, 4);
}

void World_GlobalEvent(Game_t *game, World_t *world, u32 type, u64 arg1, u64 arg2){
    
    Object_t *obj;

    u32 k;
    for(k = MAX_OBJECTS-1; k > world->freeListIndex; k--){
        obj = &world->objects[world->freeList[k]];
        obj->Event(obj, game, type, arg1, arg2);
    }
}

void World_QuadtreeRemove(World_t *world, Object_t *obj){

    if(obj->inQuad){

        u32 k;
        for(k = 0; k < obj->inQuad->nCollisions; k++){
            if(obj->inQuad->collisions[k].object == obj){
                obj->inQuad->collisions[k] = obj->inQuad->collisions[obj->inQuad->nCollisions-1];
                --obj->inQuad->nCollisions;
                break;
            }
        }
    
        obj->inQuad = NULL;
    }
}

void World_QuadtreeUpdate(World_t *world, Object_t *obj){
    
    World_QuadtreeRemove(world, obj);

    Rect2D minRect = ConvexShapeAABB(&obj->shape);

    Quadtree_LeafInsert(world->quadtree, obj, minRect, 0);
}

void World_LocalEvent(Game_t *game, World_t *world, Rect2D minRect, u32 type, u64 arg1, u64 arg2){

    Quadtree_LeafEvent(world->quadtree, game, minRect, type, arg1, arg2);
}

void World_ResolveCollisions(Game_t *game, World_t *world, Object_t *obj){

    Rect2D minRect = ConvexShapeAABB(&obj->shape);

    Quadtree_LeafResolveCollisions(world->quadtree, game, obj, &obj->shape, minRect);
}

void World_Init(World_t *world){

    world->freeListIndex = MAX_OBJECTS-1;

    u32 k;
    for(k = 0; k < MAX_OBJECTS; k++)
        world->freeList[k] = k;

}

Object_t *World_NewObject(World_t *world){

    if(world->freeListIndex == 0)
        return NULL;

    u32 handle = world->freeList[world->freeListIndex];

    --world->freeListIndex;

    Object_t *ret = &world->objects[handle];

    ret->handle = handle;
    ret->active = 1;

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

    World_QuadtreeRemove(world, &world->objects[handle]);

    World_GlobalEvent(game, world, EV_DESTROY, handle, 0);

    ++world->freeListIndex;

    world->freeList[world->freeListIndex] = handle;
}

void World_LoadLevel(World_t *world, u8 *data, u32 width, u32 height, Texture_t texture){

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

    world->tilesTexture = texture;

    u32 num = 0;

    u32 x, y;
    for(y = 0; y < height; y++){

        for(x = 0; x < width; x++){

            int index = (y * width) + x;

            if(data[index] == 0) continue;

            ++num;
        }
    }

    glBindVertexArray(world->vao);

    glBindBuffer(GL_ARRAY_BUFFER,world->uvVbo);
    glBufferData(GL_ARRAY_BUFFER,num*4*sizeof(Vec2), NULL, GL_STATIC_DRAW);

    glBindBuffer(GL_ARRAY_BUFFER,world->posVbo);
    glBufferData(GL_ARRAY_BUFFER, num*4*sizeof(Vec2), NULL, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, world->ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, num*6*sizeof(int), NULL, GL_STATIC_DRAW);

    u32 nTilesTextureX = texture.w / TILE_SIZE;

    world->quadtree = (QuadtreeLeaf_t *)Memory_Alloc(QUADTREE_MEM_STACK, sizeof(QuadtreeLeaf_t));
    Quadtree_LeafInit(world->quadtree, NULL, 0);

    float invTexWidth = 1.0f / (texture.w / TILE_SIZE);
    float invTexHeight = 1.0f / (texture.h / TILE_SIZE);

    Vec2 coords[4];
    Vec2 verts[4];
    int elements[] = {0, 1, 2, 2, 3, 0};

    u32 currLevelSize = 0;

    for(y = 0; y < height; y++){

        for(x = 0; x < width; x++){

            int index = (y * width) + x;

            if(data[index] == 0 || (TileFlags[data[index]] & TILE_MASK_CREATE_OBJECT))
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
            glBufferSubData(GL_ARRAY_BUFFER, currLevelSize*4*sizeof(Vec2), 4*sizeof(Vec2), coords);
 
            glBindBuffer(GL_ARRAY_BUFFER,world->posVbo);
            glBufferSubData(GL_ARRAY_BUFFER, currLevelSize*4*sizeof(Vec2), 4*sizeof(Vec2), verts);

            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, world->ebo);
            glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, currLevelSize*6*sizeof(int), 6*sizeof(int), elements);

            u32 k;
            for(k = 0; k < 6; k++)
                elements[k] += 4;

            ++currLevelSize;
        }
    }

    world->nElements = currLevelSize * 6;

    glBindVertexArray(0);

    u8 *closed = (u8 *)Memory_Alloc(1, width * height);
    memset(closed, 0, width * height);

    for(y = 0; y < height; y++){

        for(x = 0; x < width; x++){

            u32 index = (y * width) + x;

            if(data[index] == 0 || !(TileFlags[data[index]] & TILE_MASK_INVISIBLE) || closed[index])
                continue;

            int block = data[index];

            int rectHeight = 0;
            int rectWidth = 0;
            
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

                int tile = block;

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

            Rect2D bbRect = (Rect2D){0,0,0,0};

            bbRect.x = x * TILE_SIZE;
            bbRect.y = y * TILE_SIZE;

            if((TileFlags[data[index]] & TILE_MASK_ABOVE_INVISIBLE)){
                bbRect.y -= TILE_SIZE;                
            } else if((TileFlags[data[index]] & TILE_MASK_BELOW_INVISIBLE)){
                bbRect.y += TILE_SIZE;                
            }

            bbRect.w = TILE_SIZE * rectWidth;
            bbRect.h = TILE_SIZE * rectHeight;

            Quadtree_LeafInsert(world->quadtree, NULL, bbRect, block);
        }
    }

    memset(closed, 0, width * height);

    for(y = 0; y < height; y++){

        for(x = 0; x < width; x++){

            u32 index = (y * width) + x;

            if(data[index] == 0 || closed[index]) continue;

            Rect2D bbRect = (Rect2D){0,0,0,0};

            if(!(TileFlags[data[index]] & TILE_MASK_16x16_COLLISION)){

                bbRect = (Rect2D){x * TILE_SIZE, y * TILE_SIZE, TILE_SIZE, TILE_SIZE};

            } else {

                int block = data[index];

                int rectHeight = 0;
                int rectWidth = 0;
                
                u32 k, j;
                for(k = y; k < height; k++){

                    u32 index = (k * width) + x;
                
                    if(closed[index]) break;

                    if((TileFlags[data[index]] & TILE_MASK_16x16_COLLISION))
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

                        if(!(TileFlags[tile] & TILE_MASK_16x16_COLLISION))
                            break;
                    }

                    if((TileFlags[tile] & TILE_MASK_16x16_COLLISION))
                        ++rectWidth;
                    else
                        break;
                }


                for(k = y; k < y + rectHeight; k++)
                    for(j = x; j < x + rectWidth; j++)
                        closed[(k * width) + j] = 1;

                bbRect.x = x * TILE_SIZE;
                bbRect.y = y * TILE_SIZE;

                bbRect.w = TILE_SIZE * rectWidth;
                bbRect.h = TILE_SIZE * rectHeight;

            }

            if((TileFlags[data[index]] & TILE_MASK_CREATE_OBJECT)){

                Quadtree_LeafInsert(world->quadtree, World_NewObject(world), bbRect, data[index]);
    
            } else {

                Quadtree_LeafInsert(world->quadtree, NULL, bbRect, data[index]);
            }

        }
    }

    Memory_Pop(1,1);

}

void World_DeleteLevel(World_t *world){
    glDeleteBuffers(1, &world->posVbo);
    glDeleteBuffers(1, &world->ebo);
    glDeleteBuffers(1, &world->uvVbo);
    glDeleteVertexArrays(1, &world->vao);

    Quadtree_LeafFree(world->quadtree);
    Memory_Pop(QUADTREE_MEM_STACK, 1);
}

void World_DrawLevel(World_t *world){

    Graphics_UseShader(TEXTURED_SHADER);
    glBindVertexArray(world->vao);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, world->tilesTexture.texture);

    glDrawElements(GL_TRIANGLES, world->nElements, GL_UNSIGNED_INT, 0);
}