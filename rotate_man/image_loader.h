#ifndef IMAGE_LOADER_DEF
#define IMAGE_LOADER_DEF

#include "math.h"

typedef struct {
	int w;
	int h;
	unsigned int glTexture;
} Image;


Image ImageLoader_CreateImage(char *path, int mipmaps);
void ImageLoader_SetImageParameters(Image img, int minFilter, int magFilter, int glTextureWrapS, int glTextureWrapT);
void ImageLoader_DeleteImage(Image *img);

#endif