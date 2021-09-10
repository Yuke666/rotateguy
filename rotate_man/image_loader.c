#include <GL/glew.h>
#include <stdio.h>
#include <stdlib.h>
#include <png.h>
#include <string.h>
#include "image_loader.h"
#include "log.h"
#include "memory.h"

static unsigned char *LoadDataIntoStack(char *path, int *w, int *h){

    FILE *fp = fopen(path,"rb");

    if( fp == NULL ){
        LOG(LOG_YELLOW, "Error loading PNG %s: No such file.\n", path);
        return NULL;
    }

    unsigned char header[8];
    fread(header, 1, 8, fp);
    int ispng = !png_sig_cmp(header, 0, 8);

    if(!ispng){
        fclose(fp);
        LOG(LOG_YELLOW, "Not png %s\n", path);
        return NULL;
    }

    png_structp pngPtr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);

    if(!pngPtr) {
        LOG(LOG_YELLOW, "Error loading %s\n",path );
        fclose(fp);
        return NULL;
    }

    png_infop infoPtr = png_create_info_struct(pngPtr);
    if(!infoPtr){
        LOG(LOG_YELLOW, "Error loading %s\n",path );
        fclose(fp);
        png_destroy_read_struct(&pngPtr, &infoPtr, NULL);
        return NULL;
    }

    if(setjmp(png_jmpbuf(pngPtr))){
        LOG(LOG_YELLOW, "Error loading %s\n",path );
        png_destroy_read_struct(&pngPtr, &infoPtr, NULL);
        fclose(fp);
        return NULL;
    }

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

    *w = twidth;
    *h = theight;

    png_read_update_info(pngPtr, infoPtr);

    int rowbytes = png_get_rowbytes(pngPtr, infoPtr);

    png_byte *imageData = (png_byte *)malloc(sizeof(png_byte) * rowbytes * theight);
    if(!imageData){
        png_destroy_read_struct(&pngPtr, &infoPtr, NULL);
        fclose(fp);
        return NULL;
    }

    png_bytep *rowPointers = (png_bytep *) malloc(sizeof(png_bytep) * theight);
    if(!rowPointers){
        png_destroy_read_struct(&pngPtr, &infoPtr, NULL);
        free(imageData);
        fclose(fp);
        return NULL;
    }

    int i;
    for(i = 0; i < (int)theight; ++i)
        rowPointers[theight - 1 - i] = imageData + i * rowbytes;

    png_read_image(pngPtr, rowPointers);
    png_read_end(pngPtr, NULL);

    png_destroy_read_struct(&pngPtr, &infoPtr, NULL);

    free(rowPointers);

    fclose(fp);

    return (unsigned char *)imageData;
}

Image ImageLoader_CreateImage(char *path, int loadMipMaps){

    Image ret;
    memset(&ret, 0, sizeof(Image));

    unsigned char *imageData = LoadDataIntoStack(path, &ret.w, &ret.h);

    if(!imageData)
        return ret;

    int x, y;

    for(y = 0; y < ret.h; y++){
        for(x = 0; x < ret.w; x++){

            printf("0x%.2X, ", imageData[(((y*ret.h) + x) * 4) + 3]);

            if((x+1) % 16 == 0)
                printf("\n");
        }
    }


    return ret;

    glGenTextures(1, &ret.glTexture);
    glBindTexture(GL_TEXTURE_2D, ret.glTexture);

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, ret.w, ret.h, 0, GL_RGBA, GL_UNSIGNED_BYTE, imageData);


    if(loadMipMaps){

        glGenerateMipmap(GL_TEXTURE_2D);
        ImageLoader_SetImageParameters(ret, GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR_MIPMAP_LINEAR, GL_REPEAT, GL_REPEAT);

    } else {

        ImageLoader_SetImageParameters(ret, GL_LINEAR, GL_LINEAR, GL_REPEAT, GL_REPEAT);
    }

    free(imageData);

    glBindTexture(GL_TEXTURE_2D, 0);

    return ret;
}

void ImageLoader_SetImageParameters(Image img, int minFilter, int magFilter, int glTextureWrapS, int glTextureWrapT){
    glBindTexture(GL_TEXTURE_2D, img.glTexture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, minFilter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, magFilter);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,glTextureWrapS);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,glTextureWrapT);
    glBindTexture(GL_TEXTURE_2D, 0);
}

void ImageLoader_DeleteImage(Image *img){
    glDeleteTextures(1, &img->glTexture);
    img->glTexture = 0;
}