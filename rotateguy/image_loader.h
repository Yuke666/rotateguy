#ifndef IMAGE_LOADER_DEF
#define IMAGE_LOADER_DEF

#include "types.h"

typedef struct {
	u32 w;
	u32 h;
	float invW;
	float invH;
	u32 texture;
} Texture_t;

u32 ImageLoader_LoadTexture(Texture_t *ret, char *path, u8 loadMipMaps);
u32 ImageLoader_LoadTextureIntoMemory(Texture_t *ret, char *path, u8 loadMipMaps, u8 **pixels, u32 stack);

#endif