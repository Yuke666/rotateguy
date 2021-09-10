#include <stdio.h>
#include <stdlib.h>
#include <png.h>
#include <string.h>

static unsigned char *LoadDataIntoStack(char *path, int *w, int *h){

    FILE *fp = fopen(path,"rb");

    if( fp == NULL ){
        printf("Error loading PNG %s: No such file.\n", path);
        return NULL;
    }

    unsigned char header[8];
    fread(header, 1, 8, fp);
    int ispng = !png_sig_cmp(header, 0, 8);

    if(!ispng){
        fclose(fp);
        printf("Not png %s\n", path);
        return NULL;
    }

    png_structp pngPtr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);

    if(!pngPtr) {
        printf("Error loading %s\n",path );
        fclose(fp);
        return NULL;
    }

    png_infop infoPtr = png_create_info_struct(pngPtr);
    if(!infoPtr){
        printf("Error loading %s\n",path );
        fclose(fp);
        png_destroy_read_struct(&pngPtr, &infoPtr, NULL);
        return NULL;
    }

    if(setjmp(png_jmpbuf(pngPtr))){
        printf("Error loading %s\n",path );
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
        rowPointers[i] = imageData + i * rowbytes;

    png_read_image(pngPtr, rowPointers);
    png_read_end(pngPtr, NULL);

    png_destroy_read_struct(&pngPtr, &infoPtr, NULL);

    free(rowPointers);

    fclose(fp);

    return (unsigned char *)imageData;
}

int main(int argc, char **argv){

    if(argc < 2){
        printf("Usage: %s <input.png> <output.img>\n", argv[0]);
        return 1;
    }

    int w, h;

    unsigned char *imageData = LoadDataIntoStack(argv[1], &w, &h);

    if(!imageData)
        return 1;

    FILE *fp = fopen(argv[2], "wb");

    int x, y;

    for(y = 0; y < h; y++){
        for(x = 0; x < w; x++){

            unsigned short highcolor = (imageData[(((y*w) + x) * 4)] >> 3) << 1;
            highcolor |= (imageData[(((y*w) + x) * 4) + 1] >> 3) << 6;

            highcolor |= (imageData[(((y*w) + x) * 4) + 2] >> 3) << 11;
            highcolor |= (imageData[(((y*w) + x) * 4) + 3] & 0X01);

            fwrite(&highcolor, sizeof(unsigned short), 1, fp);
        }
    }

    free(imageData);
    
    fclose(fp);

    return 0;
}