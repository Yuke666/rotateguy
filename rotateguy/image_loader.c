#include "image_loader.h"
#include "log.h"
#include "memory.h"
#include <GL/glew.h>
#include <png.h>

u32 ImageLoader_LoadTextureIntoMemory(Texture_t *ret, char *path, u8 loadMipMaps, u8 **pixels, u32 stack){

    FILE *fp = fopen(path,"rb");

    if( fp == NULL ){
        LOG(LOG_YELLOW, "Error loading PNG %s: No such file.", path);
        return 0;
    }

    unsigned char header[8];
    fread(header, 1, 8, fp);
    int ispng = !png_sig_cmp(header, 0, 8);

    if(!ispng){
        fclose(fp);
        LOG(LOG_YELLOW, "Not png %s", path);
        return 0;
    }

    png_structp pngPtr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if(!pngPtr) {
        LOG(LOG_YELLOW, "Error loading %s",path );
        fclose(fp);
        return 0;
    }

    png_infop infoPtr = png_create_info_struct(pngPtr);
    if(!infoPtr){
        LOG(LOG_YELLOW, "Error loading %s",path );
        fclose(fp);
        png_destroy_read_struct(&pngPtr, &infoPtr, NULL);
        return 0;
    }

    if(setjmp(png_jmpbuf(pngPtr))){
        LOG(LOG_YELLOW, "Error loading %s",path );
        png_destroy_read_struct(&pngPtr, &infoPtr, NULL);
        fclose(fp);
        return 0;
    }

    glGenTextures(1, &ret->texture);

    png_set_sig_bytes(pngPtr, 8);
    png_init_io(pngPtr, fp);
    png_read_info(pngPtr, infoPtr);

    int bit_depth, color_type;
    png_uint_32 twidth, theight;

    png_get_IHDR(pngPtr, infoPtr, &twidth, &theight, &bit_depth, &color_type, NULL, NULL, NULL);

    if(color_type == PNG_COLOR_TYPE_GRAY || color_type == PNG_COLOR_TYPE_GRAY_ALPHA)
        png_set_gray_to_rgb(pngPtr);

    if(color_type == PNG_COLOR_TYPE_PALETTE)
        png_set_palette_to_rgb(pngPtr);

    png_get_IHDR(pngPtr, infoPtr, &twidth, &theight, &bit_depth, &color_type, NULL, NULL, NULL);

    if(png_get_valid(pngPtr, infoPtr, PNG_INFO_tRNS))
        png_set_tRNS_to_alpha(pngPtr);

    if(bit_depth < 8)
        png_set_packing(pngPtr);

    if(color_type == PNG_COLOR_TYPE_RGB || color_type == PNG_COLOR_TYPE_GRAY)
        png_set_add_alpha(pngPtr, 255, PNG_FILLER_AFTER);

    png_get_IHDR(pngPtr, infoPtr, &twidth, &theight, &bit_depth, &color_type, NULL, NULL, NULL);

    ret->w = twidth;
    ret->h = theight;

    png_read_update_info(pngPtr, infoPtr);

    int rowbytes = png_get_rowbytes(pngPtr, infoPtr);

    *pixels = (u8 *)Memory_Alloc(stack, sizeof(u8) * rowbytes * ret->h);
    if(!*pixels){
        png_destroy_read_struct(&pngPtr, &infoPtr, NULL);
        fclose(fp);
        return 0;
    }

    png_bytep *rowPointers = (png_bytep *)Memory_Alloc(stack, sizeof(png_bytep) * ret->h);
    if(!rowPointers){
        png_destroy_read_struct(&pngPtr, &infoPtr, NULL);
        Memory_Pop(stack, 1);
        fclose(fp);
        return 0;
    }

    u32 i;
    for(i = 0; i < ret->h; ++i)
        rowPointers[i] = *pixels + i * rowbytes;

    png_read_image(pngPtr, rowPointers);
    png_read_end(pngPtr, NULL);

    png_destroy_read_struct(&pngPtr, &infoPtr, NULL);

    Memory_Pop(stack, 1);
    fclose(fp);

    glBindTexture(GL_TEXTURE_2D, ret->texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, ret->w, ret->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, *pixels);

    if(loadMipMaps){
        glGenerateMipmap(GL_TEXTURE_2D);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST_MIPMAP_NEAREST);
    } else {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    }

    glBindTexture(GL_TEXTURE_2D, 0);

    ret->invW = 1.0f / (float)ret->w;
    ret->invH = 1.0f / (float)ret->h;

    return 1;
}


u32 ImageLoader_LoadTexture(Texture_t *ret, char *path, u8 loadMipMaps){

    u8 *pixels;
    u32 retval = ImageLoader_LoadTextureIntoMemory(ret, path, loadMipMaps, &pixels, STACK_TOP);

    if(retval){
        Memory_Pop(STACK_TOP, 1);
    }

    return retval;
}