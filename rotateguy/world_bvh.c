#include <GL/glew.h>
#include <string.h>
#include "graphics.h"
#include "memory.h"
#include "game.h"
#include "world.h"
#include "log.h"
#include "object.h"

#define BVH_FAT_MARGIN 2

static void TileObject_Event(Object_t *obj, Game_t *game, u32 event, u64 arg1, u64 arg2){

    if(event == EV_SETACTIVE){

        obj->active = arg1;

    } else if(event == EV_RENDER && obj->active){
    
        Graphics_RenderTile(obj->shape.pos.x + (TILE_SIZE/2), obj->shape.pos.y + (TILE_SIZE/2), TILE_SIZE, TILE_SIZE, 
            obj->tile, game->tilesTexture, 0xFF, 0xFF, 0xFF, 0xFF);
    }
}

static inline Rect2D CombineAABB(Rect2D b1, Rect2D b2){
    
    Rect2D ret = (Rect2D){
        MIN(b1.x, b2.x),
        MIN(b1.y, b2.y),
        MAX(b1.x + b1.w, b2.x + b2.w),
        MAX(b1.y + b1.h, b2.y + b2.h),
    };

    ret.w -= ret.x;
    ret.h -= ret.y;

    return ret;
}

static inline Rect2D MakeFatAABB(Rect2D aabb){
    return (Rect2D){
        aabb.x - BVH_FAT_MARGIN,
        aabb.y - BVH_FAT_MARGIN,
        aabb.w + BVH_FAT_MARGIN*2,
        aabb.h + BVH_FAT_MARGIN*2
    };
}

static inline float SurfaceAreaAABB(Rect2D aabb){
    return aabb.w * aabb.h;
}

static inline u8 BVHNodeIsLeaf(BVH_Node_t *node){
    return (u8)(node->left == NULL);
}

static inline BVH_Node_t *BVHNodeSibling(BVH_Node_t *node){
    return node->parent->left == node ? node->parent->right : node->parent->left;
}

static inline BVH_Node_t **BVHNodePointer(BVH_Node_t *node){
    return node->parent->left == node ? &node->parent->left : &node->parent->right;
}

static inline void BVHNodeUpdateAABB(AABB_Tree_t *tree, BVH_Node_t *node){
    
    if(BVHNodeIsLeaf(node)){

        if(node->data)
            node->aabb = MakeFatAABB(ConvexShapeAABB(&node->data->shape));

    } else {

        node->aabb = CombineAABB(node->left->aabb, node->right->aabb);
    }
}

static BVH_Node_t *CreateBVHNode(AABB_Tree_t *tree){

    BVH_Node_t *node;

    /* create from free list */

    if(!tree->firstFree){
        LOG(LOG_RED, 0, NULL, "ERROR: CreateBVHNode: FREELIST EMPTY\n");
        return NULL;
    }

    node = tree->firstFree;

    /* remove first from free list */
    tree->firstFree = tree->firstFree->next;

    node->left = NULL;
    node->right = NULL;
    node->parent = NULL;
    node->data = NULL;
    node->tile = 0;

    tree->nUsedNodes++;

    return node;
}

static void RemoveBVHNode(AABB_Tree_t *tree, BVH_Node_t *node){
    node->next = tree->firstFree;
    tree->firstFree = node;
    tree->nUsedNodes--;
}

static void AABB_Tree_Sync(AABB_Tree_t *tree, BVH_Node_t *node){

    while(node){
        BVHNodeUpdateAABB(tree, node);
        node = node->parent;
    }
}

static void AABB_Tree_Insert(AABB_Tree_t *tree, BVH_Node_t *node, BVH_Node_t **parentPtr){
    

    BVH_Node_t *parent = *parentPtr;

    if(BVHNodeIsLeaf(parent)){

        /* parent is a leaf, split it. */

        BVH_Node_t *split = CreateBVHNode(tree);
        
        split->parent = parent->parent;

        parent->parent = split;
        node->parent = split;
        
        split->left = node;
        split->right = parent;
        
        *parentPtr = parent = split;

    } else {

        /* parent is a branch, check which node to insert into based on surface area */

        float surfAreaLeftDiff = SurfaceAreaAABB(CombineAABB(parent->left->aabb, node->aabb));
        float surfAreaRightDiff = SurfaceAreaAABB(CombineAABB(parent->right->aabb, node->aabb));

        surfAreaLeftDiff -= SurfaceAreaAABB(parent->left->aabb);
        surfAreaRightDiff -= SurfaceAreaAABB(parent->right->aabb);

        if(surfAreaLeftDiff < surfAreaRightDiff)
            AABB_Tree_Insert(tree, node, &parent->left);
        else
            AABB_Tree_Insert(tree, node, &parent->right);
    
    }

    BVHNodeUpdateAABB(tree, parent);
}

static void AABB_Tree_AddObject(AABB_Tree_t *tree, Object_t *obj){

    BVH_Node_t *node = CreateBVHNode(tree);

    node->aabb = MakeFatAABB(ConvexShapeAABB(&obj->shape));
    node->data = obj;
    obj->node = node;

    /* insert node or set it to root */

    if(!tree->root)
        tree->root = node;
    else
        AABB_Tree_Insert(tree, node, &tree->root);

}

static void AABB_Tree_AddTileRect(AABB_Tree_t *tree, Rect2D aabb, u8 tile){

    BVH_Node_t *node = CreateBVHNode(tree);

    node->aabb = MakeFatAABB(aabb);
    node->data = NULL;
    node->tile = tile;

    /* insert node or set it to root */

    if(tree->root == NULL)
        tree->root = node;
    else
        AABB_Tree_Insert(tree, node, &tree->root);

}

static void AABB_Tree_Remove(AABB_Tree_t *tree, Object_t *obj){

    BVH_Node_t *node = obj->node;

    obj->node = NULL;
    node->data = NULL;

    /* remove node */

    if(node == tree->root){
    
        /* empty tree */

        RemoveBVHNode(tree, node);
        tree->root = NULL;

    } else {


        /* replace the nodes parent by it's sibling */

        BVH_Node_t *sibling = BVHNodeSibling(node);


        BVH_Node_t *parent = node->parent;
        BVH_Node_t **parentPtr = parent->parent ? BVHNodePointer(parent) : &tree->root;

        
        sibling->parent = parent->parent;

        *parentPtr = sibling;
        
        /* remove the node and it's parent */

        RemoveBVHNode(tree, parent);
        RemoveBVHNode(tree, node);
    
        AABB_Tree_Sync(tree, sibling->parent);
    }
    
}

static void AABB_Tree_UpdateNode(AABB_Tree_t *tree, BVH_Node_t *node){

    if(BVHNodeIsLeaf(node)){

        /* if leaf node and the aabb has moved out of the nodes fat aabb */

        if(node->data && !Math_CheckRect2DInside(ConvexShapeAABB(&node->data->shape), node->aabb)){

            /* replace nodes parent with it's sibling */

            BVH_Node_t *sibling = BVHNodeSibling(node);

            BVH_Node_t *parent = node->parent;
            BVH_Node_t **parentPtr = parent->parent ? BVHNodePointer(parent) : &tree->root;
            
            *parentPtr = sibling;

            sibling->parent = parent->parent;

            /* remove nodes parent */

            RemoveBVHNode(tree, parent);

            AABB_Tree_Sync(tree, sibling->parent);

            /* reinsert node */

            BVHNodeUpdateAABB(tree, node);
            AABB_Tree_Insert(tree, node, &tree->root);
        }

    } else {

        /* not leaf, recurse */

        AABB_Tree_UpdateNode(tree, node->left);     
        AABB_Tree_UpdateNode(tree, node->right);        
    }
}

static void AABB_Tree_Update(AABB_Tree_t *tree){

    /* tree not created. return. */

    if(!tree->root) return;

    /* update if tree only consists of root, otherwise recurse and update needed insertions. */

    if(BVHNodeIsLeaf(tree->root))
        BVHNodeUpdateAABB(tree, tree->root);
    else
        AABB_Tree_UpdateNode(tree, tree->root);

}

static void AABB_Tree_Init(AABB_Tree_t *tree){
    
    /* init free list */

    tree->firstFree = &tree->bvh[0];

    u32 k;
    for(k = 0; k < MAX_BVH_NODES-1; k++){

        tree->bvh[k].next = &tree->bvh[k+1]; 
    }

    tree->bvh[MAX_BVH_NODES-1].next = NULL; 

    /* init root as null */

    tree->root = NULL;

    tree->nUsedNodes = 0;
}


static ConvexShape_t GetTileRectShape(Rect2D rect, u8 tile){

    ConvexShape_t shape;
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

static void AABB_Tree_CheckNodeCollision(BVH_Node_t *node, Game_t *game, Object_t *obj){

    if(Math_CheckCollisionRect2D(node->aabb, ConvexShapeAABB(&obj->shape))){

        if(BVHNodeIsLeaf(node)){
        
            if(node->data != obj){

                Vec2 resolveVec;

                if(node->data){

                    if(SATShapeShape(&obj->shape, &node->data->shape, &resolveVec)){
                        if(obj->OnCollision){

                            obj->OnCollision(obj, game, node->data, resolveVec);
                        }
                    }

                } else {

                    ConvexShape_t shape = GetTileRectShape(node->aabb, node->tile);

                    if(SATShapeShape(&obj->shape, &shape, &resolveVec)){
                        if(obj->OnCollisionStatic != NULL){

                            obj->OnCollisionStatic(obj, game, node->tile, resolveVec);
                        }
                    }

                }
            }

        } else {
    
            AABB_Tree_CheckNodeCollision(node->left, game, obj);         
            AABB_Tree_CheckNodeCollision(node->right, game, obj);            
        }
    }
}

static void AABB_Tree_NodeEvent(BVH_Node_t *node, Game_t *game, Rect2D aabb, u32 type, u64 arg1, u64 arg2){

    if(Math_CheckCollisionRect2D(node->aabb, aabb)){

        if(BVHNodeIsLeaf(node)){
    
            if(node->data && node->data->Event){

                node->data->Event(node->data, game, type, arg1, arg2);
            }

        } else {
    
            AABB_Tree_NodeEvent(node->left, game, aabb, type, arg1, arg2);         
            AABB_Tree_NodeEvent(node->right, game, aabb, type, arg1, arg2);            
        }
    }
}

static void AABB_Tree_CheckCollision(AABB_Tree_t *tree, Game_t *game, Object_t *obj){

    AABB_Tree_CheckNodeCollision(tree->root, game, obj);
}

void World_BVH_Add(World_t *world, Object_t *obj){
    AABB_Tree_AddObject(&world->aabbTree, obj); 
}

void World_BVH_Remove(World_t *world, Object_t *obj){
    AABB_Tree_Remove(&world->aabbTree, obj); 
}

void World_BVH_Update(World_t *world, Object_t *obj){
    AABB_Tree_UpdateNode(&world->aabbTree, obj->node); 
}

void World_GlobalEvent(Game_t *game, World_t *world, u32 type, u64 arg1, u64 arg2){
    
    Object_t *obj;

    u32 k;
    for(k = MAX_OBJECTS-1; k > world->objFreeListIndex; k--){
        obj = &world->objects[world->objFreeList[k]];
        obj->Event(obj, game, type, arg1, arg2);
    }
}

void World_LocalEvent(Game_t *game, World_t *world, Rect2D minRect, u32 type, u64 arg1, u64 arg2){
    AABB_Tree_NodeEvent(world->aabbTree.root, game, minRect, type, arg1, arg2);
}

void World_ResolveCollisions(Game_t *game, World_t *world, Object_t *obj){

    AABB_Tree_CheckCollision(&world->aabbTree, game, obj);
}

void World_Init(World_t *world){

    world->objFreeListIndex = MAX_OBJECTS-1;

    u32 k;
    for(k = 0; k < MAX_OBJECTS; k++)
        world->objFreeList[k] = k;

}

Object_t *World_NewObject(World_t *world){

    if(world->objFreeListIndex == 0)
        return NULL;

    u32 handle = world->objFreeList[world->objFreeListIndex];

    --world->objFreeListIndex;

    Object_t *ret = &world->objects[handle];

    ret->handle = handle;
    ret->active = 1;

    return ret;
}

void World_RemoveObject(Game_t *game, World_t *world, u32 handle){

    if(world->objFreeListIndex == MAX_OBJECTS-1){
        LOG(LOG_YELLOW, "ATTEMPTED TO REMOVE OBJECT WHEN NO USED OBJECTS");
    }
    
    if(handle > MAX_OBJECTS){
        LOG(LOG_YELLOW, "ATTEMPTED TO REMOVE HANDLE OVER MAX_OBJECTS");
    }

    if(world->objects[handle].active == 0){
        LOG(LOG_YELLOW, "ATTEMPTED TO REMOVE OBJECT NON ACTIVE OBJECT");
    }

    world->objects[handle].Event(&world->objects[handle], game, EV_OBJDESTROY, 0, 0);

    AABB_Tree_Remove(&world->aabbTree, &world->objects[handle]);

    World_GlobalEvent(game, world, EV_DESTROY, handle, 0);

    ++world->objFreeListIndex;

    world->objFreeList[world->objFreeListIndex] = handle;
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

    AABB_Tree_Init(&world->aabbTree);

    u8 *closed = (u8 *)Memory_Alloc(1, width * height);
    // memset(closed, 0, width * height);

    // for(y = 0; y < height; y++){

    //     for(x = 0; x < width; x++){

    //         u32 index = (y * width) + x;

    //         if(data[index] == 0 || !(TileFlags[data[index]] & TILE_MASK_INVISIBLE) || closed[index])
    //             continue;

    //         int block = data[index];

    //         int rectHeight = 0;
    //         int rectWidth = 0;
            
    //         u32 k, j;
    //         for(k = y; k < height; k++){

    //             u32 index = (k * width) + x;
            
    //             if(closed[index]) break;

    //             if(data[index] == block)
    //                 ++rectHeight;
    //             else
    //                 break;
    //         }

    //         for(k = x; k < width; k++){

    //             int tile = block;

    //             for(j = y; j < y + rectHeight; j++){

    //                 u32 index = (j * width) + k;
                
    //                 if(closed[index]){
    //                     tile = TILE_NONE;
    //                     break;
    //                 }

    //                 tile = data[index];

    //                 if(tile == block)
    //                     break;
    //             }

    //             if(tile == block)
    //                 ++rectWidth;
    //             else
    //                 break;
    //         }

    //         for(k = y; k < y + rectHeight; k++)
    //             for(j = x; j < x + rectWidth; j++)
    //                 closed[(k * width) + j] = 1;

    //         Rect2D bbRect = (Rect2D){0,0,0,0};

    //         bbRect.x = x * TILE_SIZE;
    //         bbRect.y = y * TILE_SIZE;

    //         if((TileFlags[data[index]] & TILE_MASK_ABOVE_INVISIBLE)){
    //             bbRect.y -= TILE_SIZE;                
    //         } else if((TileFlags[data[index]] & TILE_MASK_BELOW_INVISIBLE)){
    //             bbRect.y += TILE_SIZE;                
    //         }

    //         bbRect.w = TILE_SIZE * rectWidth;
    //         bbRect.h = TILE_SIZE * rectHeight;

    //         AABB_Tree_AddTileRect(&world->aabbTree, bbRect, block);
    //     }
    // }

    memset(closed, 0, width * height);

    for(y = 0; y < height; y++){

        for(x = 0; x < width; x++){

            u32 index = (y * width) + x;

            if(data[index] == 0 || closed[index]) continue;

            Rect2D bbRect = (Rect2D){0,0,0,0};

            if(!(TileFlags[data[index]] & TILE_MASK_16x16_COLLISION)){

                bbRect = (Rect2D){x * TILE_SIZE, y * TILE_SIZE, TILE_SIZE, TILE_SIZE};

            } else {

                u8 block = data[index];

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

                    u32 tile = block;

                    for(j = y; j < y + rectHeight; j++){

                        u32 index = (j * width) + k;
                    
                        if(closed[index]){
                            tile = TILE_NONE;
                            break;
                        }

                        tile = data[index];

                        if(data[index] != block)
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

                bbRect.x = x * TILE_SIZE;
                bbRect.y = y * TILE_SIZE;

                bbRect.w = TILE_SIZE * rectWidth;
                bbRect.h = TILE_SIZE * rectHeight;

            }

            if((TileFlags[data[index]] & TILE_MASK_CREATE_OBJECT)){

                Object_t *obj = World_NewObject(world);
                obj->Event = TileObject_Event;
                obj->shape = GetTileRectShape(bbRect, data[index]);
                obj->tile = data[index];

                AABB_Tree_AddObject(&world->aabbTree, obj);
    
            } else {

                AABB_Tree_AddTileRect(&world->aabbTree, bbRect, data[index]);
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
}


static void AABB_Tree_Draw(BVH_Node_t *node, u32 level){

    if(node == NULL)
        return;


    if(BVHNodeIsLeaf(node)){

        glDisable(GL_BLEND);
        Graphics_RenderRectLines(node->aabb.x, node->aabb.y, node->aabb.w, node->aabb.h, 0, 0, 0, 255);
        Graphics_RenderRect(node->aabb.x, node->aabb.y, node->aabb.w, node->aabb.h, 0, 20*level, 0, 255);
        glEnable(GL_BLEND);

    } else {

        Graphics_RenderRect(node->aabb.x, node->aabb.y, node->aabb.w, node->aabb.h, 255, 0, 0, 5);
    }

    AABB_Tree_Draw(node->left, level+1);
    AABB_Tree_Draw(node->right, level+1);
}

void World_DrawLevel(World_t *world){

    glLineWidth(2);
    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE);
    AABB_Tree_Draw(world->aabbTree.root, 0);
    glDisable(GL_BLEND);


    Graphics_UseShader(TEXTURED_SHADER);
    glBindVertexArray(world->vao);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, world->tilesTexture.texture);

    glDrawElements(GL_TRIANGLES, world->nElements, GL_UNSIGNED_INT, 0);

}