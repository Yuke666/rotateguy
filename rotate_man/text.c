#include <GL/glew.h>
#include "text.h"
#include "images.h"
#include "shader_files.h"
#include "window.h"
#include "shaders.h"
#include "math.h"
#include "utils.h"

#define TAB_SPACING 4
#define MAX_DRAW_CHARACTERS 512
#define SS_CHAR_SIZE 0.0625f

static const Vertex22 squareData[6] = {
    {{-0.5f, -0.5f}, { 0, 1 }},
    {{-0.5f,  0.5f}, { 0, 0 }},
    {{ 0.5f,  0.5f}, { 1, 0 }},
    {{ 0.5f,  0.5f}, { 1, 0 }},
    {{ 0.5f, -0.5f}, { 1, 1 }},
    {{-0.5f, -0.5f}, { 0, 1 }},
};

void Text_Close(void){
    glDeleteTextures(1, &fontTexture.texture);
    glDeleteBuffers(1, &font->posVbo);
    glDeleteBuffers(1, &font->uvVbo);
    glDeleteVertexArrays(1, &font->vao);
}

void Text_Init(void){

    glGenVertexArrays(1, &font->vao);
    glBindVertexArray(font->vao);

    glGenBuffers(1, &font->uvVbo);
    glBindBuffer(GL_ARRAY_BUFFER, font->uvVbo);

    glEnableVertexAttribArray(UV_LOC);
    glVertexAttribPointer(UV_LOC, 2, GL_FLOAT, GL_FALSE, 0, 0);

    glBufferData(GL_ARRAY_BUFFER, sizeof(Vec2) * MAX_DRAW_CHARACTERS * 6, NULL, GL_STATIC_DRAW);

    glGenBuffers(1, &font->posVbo);
    glBindBuffer(GL_ARRAY_BUFFER, font->posVbo);
    
    glEnableVertexAttribArray(POS_LOC);
    glVertexAttribPointer(POS_LOC, 2, GL_FLOAT, GL_FALSE, 0, 0);
    
    glBufferData(GL_ARRAY_BUFFER, sizeof(Vec2) * MAX_DRAW_CHARACTERS * 6, NULL, GL_STATIC_DRAW);

    glBindVertexArray(0);
}

static u32 ValidateXY(u32 *x, u32 *y, u32 fontSize, u32 startX, u32 vSpacing, u32 maxWidth){
    
    // bottom width is the smaller one.
    if(*x+fontSize >= maxWidth - fontSize){

        *y += (fontSize + vSpacing);
        *x = startX;

        // top and bottom height are the same.
        if(*y+fontSize >= TOP_HEIGHT)
            return 0;
    }

    return 1;
}

void Text_Draw(u32 x, u32 y, u32 hSpacing, u32 vSpacing, u32 maxWidth, char *text){

    u32 fontSize = fontTexture.width / 16;

    u32 startX = x;

    u32 num = 0;

    glBindVertexArray(font->vao);
    
    u32 p;

    while((p = text++)){

        if(p == '\n'){

            y += (fontSize + vSpacing);
            
            x = startX;

            // top and bottom height are the same.
            if(y+fontSize >= TOP_HEIGHT)
                break;

            continue;
        
        } else if(p == '\t' ){
            
            x += (fontSize + hSpacing) * TAB_SPACING;
            
            if(ValidateXY(&x, &y, fontSize, startX, vSpacing, maxWidth) == 0) break;

            continue;

        } else if(p < 32){

            break;
        
        } else if(p == 32){

            x += (fontSize + hSpacing);

            if(ValidateXY(&x, &y, fontSize, startX, vSpacing, maxWidth) == 0) break;

            continue;
        }

        p -= 32;

        float tX = (p % 16) / 16.0f;
        float tY = (1 - SS_CHAR_SIZE) - (floorf(p / 16.0f) / 16.0f);

        u32 k;
        for(k = 0; k < 6; k++){

            Vec2 pos, uv;
            
            uv.x = (squareData[k].coord.x * SS_CHAR_SIZE) + tX;
            uv.y = (squareData[k].coord.y * SS_CHAR_SIZE) + tY;
            pos.x = (squareData[k].pos.x * fontSize) + x;
            pos.y = (squareData[k].pos.y * fontSize) + y;

            glBindBuffer(GL_ARRAY_BUFFER, font->posVbo);
            glBufferSubData(GL_ARRAY_BUFFER, sizeof(Vec2) * ((6 * num) + k), sizeof(Vec2), &pos.x);
            glBindBuffer(GL_ARRAY_BUFFER, font->uvVbo);
            glBufferSubData(GL_ARRAY_BUFFER, sizeof(Vec2) * ((6 * num) + k), sizeof(Vec2), &uv.x);
        }

        x += (fontSize + hSpacing);

        ValidateXY(&x, &y, fontSize, startX, vSpacing, maxWidth);

        ++num;

        if(num >= MAX_DRAW_CHARACTERS)
            break;

    }

    Shader_2d shader = Shader_GetShader_2d();
    glUseProgram(shader.program);

    glBindTexture(GL_TEXTURE_2D, fontTexture.texture);

    glDrawArrays(GL_TRIANGLES, 0, 6 * num);
}