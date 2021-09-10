#include <GL/glew.h>
#include <stdio.h>
#include "graphics.h"
#include "image_loader.h"
#include "math.h"
#include "window.h"
#include "memory.h"
#include "log.h"

#define FONT_PATH "resources/font.png"

#define STRDEF(x) STR(x)
#define STR(x) #x

#define CLEAR_COLOR 0.02,0.02,0.02,0.02

#define POS_ATTRIB "pos"
#define UV_ATTRIB "uv"

#define FONT_SIZE_BITS 4
#define FONT_SIZE_MASK ((1 << FONT_SIZE_BITS)-1)

#define TILES_SIZE 128
#define TILES_SIZE_BITS 3
#define TILES_SIZE_MASK 7

#define MAX_TEXT_CHARS 128
#define RENDER_VRAM_SIZE ((TILES_X * TILES_Y)*6)

static const u8 	RectTriangleVerts[] = {0,0,1,0,1,1,1,1,0,1,0,0};

// shaders
static Shader_t     shaders[NUM_SHADERS];
// textured/texturelesss vao/vbos
static u32          vao_g;
static u32          posVbo_g;
static u32          uvVbo_g;
// quad vao/vbo
static u32          quadVao_g;
static u32          quadVbo_g;
// framebuffer
static u32      	fb_g;
static u32      	fbTexture_g;
// textures
static Texture_t    font_g;

static void Compile(Shader_t *shader, const char *vSource, const char *fSource){

    shader->program = glCreateProgram();
    shader->fShader = glCreateShader(GL_FRAGMENT_SHADER);
    shader->vShader = glCreateShader(GL_VERTEX_SHADER);
    
    GLint status = GL_TRUE;

    char buffer[512];

    glShaderSource(shader->fShader, 1, (const GLchar **)&fSource, NULL);
    glCompileShader(shader->fShader);
    glGetShaderiv(shader->fShader, GL_COMPILE_STATUS, &status);

    if(status != GL_TRUE){
        glGetShaderInfoLog(shader->fShader, 512, NULL, buffer);
        LOG(LOG_RED, "FSHADER: %s", buffer);
        return;
    }

    glAttachShader(shader->program, shader->fShader);

    glShaderSource(shader->vShader, 1, (const GLchar **)&vSource, NULL);
    glCompileShader(shader->vShader);
    glGetShaderiv(shader->vShader, GL_COMPILE_STATUS, &status);

    if(status != GL_TRUE){
        glGetShaderInfoLog(shader->vShader, 512, NULL, buffer);
        LOG(LOG_RED, "VSHADER: %s", buffer);
        return;
    }

    glBindAttribLocation(shader->program, POS_LOC, POS_ATTRIB);
    glBindAttribLocation(shader->program, UV_LOC, UV_ATTRIB);

    glAttachShader(shader->program, shader->vShader);
    glLinkProgram(shader->program);

    glUseProgram(shader->program);

    shader->uniColorLoc = glGetUniformLocation(shader->program, "uniformColor");
    shader->camPosLoc = glGetUniformLocation(shader->program, "camPos");
    shader->zoomLoc = glGetUniformLocation(shader->program, "zoom");
    shader->abberationLoc = glGetUniformLocation(shader->program, "abberation");
}

static const char *VS_Source = "#version 120\n"
STR(
attribute vec2 pos;
attribute vec2 uv;
varying vec2 TexCoord;
)

STR(uniform float invWidth = 2.0f/) STRDEF(WIDTH) STR(;)
STR(uniform float InvHeight = 2.0f/) STRDEF(HEIGHT) STR(;)

STR(

uniform vec2 camPos = vec2(0,0);
uniform vec2 zoom = vec2(1,1);

void main(){

    TexCoord = uv;
    gl_Position = vec4((vec2(((pos.x - camPos.x) * invWidth), 2-((pos.y - camPos.y) * InvHeight)) - 1) * zoom, -1, 1);
}
);

static const char *FS_Source = "#version 120\n"
STR(
varying vec2 TexCoord;
uniform sampler2D tex;
uniform vec4 uniformColor = vec4(1,1,1,1);

void main(){

    vec4 color = texture2D(tex, TexCoord);
    if(color.a < 0.5) discard;

    gl_FragColor = color * vec4(uniformColor.rgb*uniformColor.a, 1);
}
);

static const char *VS_Quad_Source = "#version 120\n"
STR(
attribute vec2 pos;
varying vec2 TexCoord;
)

STR(

void main(){
    TexCoord = pos;
    gl_Position = vec4((pos * 2) - 1, -1, 1);
}
);

static const char *FS_Quad_Source = "#version 120\n"
STR(
varying vec2 TexCoord;
uniform sampler2D tex;
uniform int abberation = 0;

void main(){

    vec3 color = texture2D(tex, TexCoord).rgb;

    if(abberation){
        color.r += texture2D(tex, vec2(TexCoord.x+0.015, TexCoord.y)).r;
        color.g += texture2D(tex, vec2(TexCoord.x-0.015, TexCoord.y)).g;
    }

    gl_FragColor = vec4(color, 1);
}
);

static const char *VS_Textureless_Source = "#version 120\n"
STR(
attribute vec2 pos;
)

STR(uniform float invWidth = 2.0f/) STRDEF(WIDTH) STR(;)
STR(uniform float InvHeight = 2.0f/) STRDEF(HEIGHT) STR(;)

STR(

uniform vec2 camPos = vec2(0,0);
uniform vec2 zoom = vec2(1,1);

void main(){

    gl_Position = vec4((vec2(((pos.x - camPos.x) * invWidth), 2-((pos.y - camPos.y) * InvHeight)) - 1) * zoom, -1, 1);
}
);

static const char *FS_Textureless_Source = "#version 120\n"
STR(

uniform vec4 uniformColor = vec4(1,1,1,1);

void main(){

    gl_FragColor = vec4(uniformColor.rgb*uniformColor.a, 1);
}
);


static void CreateFrameBuffer(void){

    glGenFramebuffers(1,&fb_g);
    glBindFramebuffer(GL_FRAMEBUFFER, fb_g);

    glGenTextures(1, &fbTexture_g);
    glBindTexture(GL_TEXTURE_2D, fbTexture_g);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, WIDTH, HEIGHT, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, fbTexture_g, 0);

    glDrawBuffer(GL_COLOR_ATTACHMENT0);

    if(glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE){
        glDeleteFramebuffers(1, &fb_g);
        glDeleteTextures(1, &fbTexture_g);
        LOG(LOG_RED, "Error creating framebuffer.");
    }

    glViewport(0, 0, WIDTH, HEIGHT);

    glClearColor(CLEAR_COLOR);
    glClear(GL_COLOR_BUFFER_BIT);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Graphics_Init(void){

    glEnable(GL_CULL_FACE);
    glCullFace(GL_FRONT);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_POLYGON_SMOOTH);

    // textures

    ImageLoader_LoadTexture(&font_g, FONT_PATH, 1);

    // compile shaders

    Compile(&shaders[TEXTURED_SHADER], VS_Source, FS_Source);
    Compile(&shaders[QUAD_SHADER], VS_Quad_Source, FS_Quad_Source);
    Compile(&shaders[TEXTURELESS_SHADER], VS_Textureless_Source, FS_Textureless_Source);

    // quad vao

    glGenVertexArrays(1, &quadVao_g);
    glBindVertexArray(quadVao_g);

    glGenBuffers(1, &quadVbo_g);
    glBindBuffer(GL_ARRAY_BUFFER, quadVbo_g);
    glBufferData(GL_ARRAY_BUFFER, sizeof(RectTriangleVerts), RectTriangleVerts, GL_STATIC_DRAW);
    glEnableVertexAttribArray(POS_LOC);
    glVertexAttribPointer(POS_LOC, 2, GL_UNSIGNED_BYTE, GL_FALSE, 0, 0);

    // textured/textureless vao/vbos

    glGenVertexArrays(1, &vao_g);
    glBindVertexArray(vao_g);

    glGenBuffers(1, &posVbo_g);
    glBindBuffer(GL_ARRAY_BUFFER, posVbo_g);
    glBufferData(GL_ARRAY_BUFFER, sizeof(u16)*2*RENDER_VRAM_SIZE, NULL, GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(POS_LOC);
    glVertexAttribPointer(POS_LOC, 2, GL_SHORT, GL_FALSE, 0, 0);

    glGenBuffers(1, &uvVbo_g);
    glBindBuffer(GL_ARRAY_BUFFER, uvVbo_g);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float)*2*RENDER_VRAM_SIZE, NULL, GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(UV_LOC);
    glVertexAttribPointer(UV_LOC, 2, GL_FLOAT, GL_FALSE, 0, 0);

    // init framebuffer
    
    CreateFrameBuffer();

    glBindFramebuffer(GL_FRAMEBUFFER, fb_g);
}

static void DeleteShader(Shader_t *shader){
    glDeleteProgram(shader->program);
    glDeleteShader(shader->fShader);
    glDeleteShader(shader->vShader);
}

void Graphics_Close(void){

    glDeleteFramebuffers(1, &fb_g);
    glDeleteTextures(1, &fbTexture_g);
    
    DeleteShader(&shaders[TEXTURED_SHADER]);
    DeleteShader(&shaders[TEXTURELESS_SHADER]);
    DeleteShader(&shaders[QUAD_SHADER]);

    glDeleteVertexArrays(1, &quadVao_g);
    glDeleteBuffers(1, &quadVbo_g);

    glDeleteVertexArrays(1, &vao_g);
    glDeleteBuffers(1, &uvVbo_g);
    glDeleteBuffers(1, &posVbo_g);

    glDeleteTextures(1, &font_g.texture);
}

void Graphics_Resize(int w, int h){
    glBindTexture(GL_TEXTURE_2D, fbTexture_g);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
}

void Graphics_Render(void){

    // render

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    glBindVertexArray(quadVao_g);

    glViewport(0, 0, GAME_DEFAULT_WIDTH, GAME_DEFAULT_HEIGHT);

    glUseProgram(shaders[QUAD_SHADER].program);

    glCullFace(GL_BACK);

    glBindTexture(GL_TEXTURE_2D, fbTexture_g);
    glDrawArrays(GL_TRIANGLES, 0, 6);

    // end

    glBindFramebuffer(GL_FRAMEBUFFER, fb_g);
    glViewport(0, 0, WIDTH, HEIGHT);

    glClearColor(CLEAR_COLOR);
    glClear(GL_COLOR_BUFFER_BIT);
    
    glCullFace(GL_FRONT);

}

void Graphics_RenderString(char *str, u32 xpos, u32 ypos, u8 r, u8 g, u8 b){

	glUseProgram(shaders[TEXTURED_SHADER].program);
	glUniform4f(shaders[TEXTURED_SHADER].uniColorLoc, r / 255.0f, g  / 255.0f, b  / 255.0f, 1); 


    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, font_g.texture);

	glBindVertexArray(vao_g);

	u32 offset = 0;

    u16 pos[2];
    float uv[2];

    u32 k;

    char p;
    while((p = *str++) && offset < MAX_TEXT_CHARS*6){

        p -= ' ';

	    for(k = 0; k < 12; k+=2){

	    	pos[0] = ((s16)RectTriangleVerts[k] << 3) + xpos;
	    	pos[1] = ((s16)RectTriangleVerts[k+1] << 3) + ypos;
	    	uv[0] = (float)((RectTriangleVerts[k] + (p & FONT_SIZE_MASK)) << 3) * font_g.invW;
	    	uv[1] = (float)((RectTriangleVerts[k+1] + (p >> FONT_SIZE_BITS)) << 3) * font_g.invH;
        
            glBindBuffer(GL_ARRAY_BUFFER, posVbo_g);
            glBufferSubData(GL_ARRAY_BUFFER, offset*sizeof(pos), sizeof(pos), pos);
            glBindBuffer(GL_ARRAY_BUFFER, uvVbo_g);
            glBufferSubData(GL_ARRAY_BUFFER, offset*sizeof(uv), sizeof(uv), uv);
	    	++offset;
	    }
	    
	    xpos += 9;
    }

    glDisable(GL_CULL_FACE);

    glUniform4f(shaders[TEXTURED_SHADER].uniColorLoc, 1, 1, 1, 1); 

    glDrawArrays(GL_TRIANGLES, 0, offset);
}

void Graphics_RenderTileMap(u32 sx, u32 sy, const u8 *map, u16 mapW, u16 mapH, Texture_t tex){

    glBindVertexArray(vao_g);

	u8 cx = sx & TILE_SIZE_MASK;
	u8 cy = sy & TILE_SIZE_MASK;

	sx >>= TILE_SIZE_BITS;
	sy >>= TILE_SIZE_BITS;

	// start offset -1 tile
	// TILES_X and TILES_Y are +2
	if(sy > 0){
		sy -= 1;
		cy += TILE_SIZE;
	}

	if(sx > 0){
		sx -= 1;
		cx += TILE_SIZE;
	}

    sx %= mapW;
    sy %= mapH;

	u8 tile;

	u32 offset = 0;

	u32 x, y, k;

    u16 pos[2];
    float uv[2];

	for(y = 0; y < MIN(TILES_Y, mapH-sy); y++){
		
		for(x = 0; x < MIN(TILES_X, mapW-sx); x++){

			tile = map[((y+sy)*mapW) + x + sx];

			if(!tile) continue;

			--tile;

		    for(k = 0; k < 12; k+=2){

		    	pos[0] = (((s16)RectTriangleVerts[k]+x) << TILE_SIZE_BITS) - cx;
		    	pos[1] = (((s16)RectTriangleVerts[k+1]+y) << TILE_SIZE_BITS) - cy;
		    	uv[0] = (float)((RectTriangleVerts[k] + (tile & TILES_SIZE_MASK)) << TILE_SIZE_BITS) * tex.invW;
		    	uv[1] = (float)((RectTriangleVerts[k+1] + (tile >> TILES_SIZE_BITS)) << TILE_SIZE_BITS) * tex.invH;

                glBindBuffer(GL_ARRAY_BUFFER, posVbo_g);
                glBufferSubData(GL_ARRAY_BUFFER, offset*sizeof(pos), sizeof(pos), pos);
                glBindBuffer(GL_ARRAY_BUFFER, uvVbo_g);
                glBufferSubData(GL_ARRAY_BUFFER, offset*sizeof(uv), sizeof(uv), uv);
                ++offset;
		    }
		}
	}

	glUseProgram(shaders[TEXTURED_SHADER].program);
	glUniform4f(shaders[TEXTURED_SHADER].uniColorLoc, 1, 1, 1, 1); 

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, tex.texture);

    glDrawArrays(GL_TRIANGLES, 0, offset);
}

void Graphics_RenderSprite(float x, float y, u16 w, u16 h, u16 tx, u16 ty, u16 tw, u16 th, Texture_t tex){

    glBindVertexArray(vao_g);

    u32 offset = 0;
    u16 pos[2];

    float uv[2];

    u32 k;
    for(k = 0; k < 12; k+=2){

        pos[0] = x + ((s16)RectTriangleVerts[k] * w);
        pos[1] = y + ((s16)RectTriangleVerts[k+1] * h);

        uv[0] = ((float)(RectTriangleVerts[k] * tw) + tx) * tex.invW;
        uv[1] = ((float)(RectTriangleVerts[k+1] * th) + ty) * tex.invH;

        glBindBuffer(GL_ARRAY_BUFFER, posVbo_g);
        glBufferSubData(GL_ARRAY_BUFFER, offset*sizeof(pos), sizeof(pos), pos);
        glBindBuffer(GL_ARRAY_BUFFER, uvVbo_g);
        glBufferSubData(GL_ARRAY_BUFFER, offset*sizeof(uv), sizeof(uv), uv);
    
        ++offset;
    }


    glUseProgram(shaders[TEXTURED_SHADER].program);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, tex.texture);

    glBindVertexArray(vao_g);
    glBindBuffer(GL_ARRAY_BUFFER, posVbo_g);

    glDrawArrays(GL_TRIANGLES, 0, 6);
}

void Graphics_RenderTile(float x, float y, u16 w, u16 h, u8 tile, Texture_t tex, u8 r, u8 g, u8 b, u8 a){

    glUseProgram(shaders[TEXTURED_SHADER].program);
    glUniform4f(shaders[TEXTURED_SHADER].uniColorLoc, r / 255.0f, g / 255.0f, b / 255.0f, a / 255.0f); 

    u32 tx = ((tile-1) * TILE_SIZE) % (tex.w);
    u32 ty = (((tile-1) * TILE_SIZE) / tex.w)*TILE_SIZE;

    Graphics_RenderSprite(x - (w/2), y - (h/2), w, h, 
        tx, ty, TILE_SIZE, TILE_SIZE, tex);

    glUniform4f(shaders[TEXTURED_SHADER].uniColorLoc, 1, 1, 1, 1); 
}


void Graphics_RenderRotatedSprite(float x, float y, u16 w, u16 h,
    float rotation, s16 ox, s16 oy, u16 tx, u16 ty, Texture_t tex){

    glBindVertexArray(vao_g);

    float sintheta = sinf(rotation);
    float costheta = cosf(rotation);

    ox += w/2;
    oy += h/2;

    s16 relx, rely;

    u32 offset = 0;
    u16 pos[2];

    float uv[2];

    u32 k;
    for(k = 0; k < 12; k+=2){

    	relx = ((s16)RectTriangleVerts[k] * w) - ox;
    	rely = ((s16)RectTriangleVerts[k+1] * h) - oy;

        pos[0] = x + ox + ( (relx * costheta) - (sintheta * rely) );
        pos[1] = y + oy + ( (relx * sintheta) + (costheta * rely) );
        // pos[0] = x + ( (relx * costheta) - (sintheta * rely) );
        // pos[1] = y + ( (relx * sintheta) + (costheta * rely) );

    	uv[0] = ((float)(RectTriangleVerts[k] * w) + tx) * tex.invW;
    	uv[1] = ((float)(RectTriangleVerts[k+1] * h) + ty) * tex.invH;

        glBindBuffer(GL_ARRAY_BUFFER, posVbo_g);
        glBufferSubData(GL_ARRAY_BUFFER, offset*sizeof(pos), sizeof(pos), pos);
        glBindBuffer(GL_ARRAY_BUFFER, uvVbo_g);
        glBufferSubData(GL_ARRAY_BUFFER, offset*sizeof(uv), sizeof(uv), uv);
    
        ++offset;
    }


    glUseProgram(shaders[TEXTURED_SHADER].program);
    glUniform4f(shaders[TEXTURED_SHADER].uniColorLoc, 1, 1, 1, 1); 

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, tex.texture);

    glBindVertexArray(vao_g);
    glBindBuffer(GL_ARRAY_BUFFER, posVbo_g);

    glDrawArrays(GL_TRIANGLES, 0, 6);
}

void Graphics_RenderRotatedRect(float x, float y, u16 w, u16 h, float rotation, s16 ox, s16 oy){

    glBindVertexArray(vao_g);

    float sintheta = sinf(rotation);
    float costheta = cosf(rotation);

    ox += w/2;
    oy += h/2;

    s16 relx, rely;

    u32 offset = 0;
    u16 pos[2];

    u32 k;
    for(k = 0; k < 12; k+=2){

        relx = ((s16)RectTriangleVerts[k] * w) - ox;
        rely = ((s16)RectTriangleVerts[k+1] * h) - oy;

        pos[0] = x + ox + ( (relx * costheta) - (sintheta * rely) );
        pos[1] = y + oy + ( (relx * sintheta) + (costheta * rely) );
        // pos[0] = x + ( (relx * costheta) - (sintheta * rely) );
        // pos[1] = y + ( (relx * sintheta) + (costheta * rely) );

        glBindBuffer(GL_ARRAY_BUFFER, posVbo_g);
        glBufferSubData(GL_ARRAY_BUFFER, offset*sizeof(pos), sizeof(pos), pos);
    
        ++offset;
    }


    glUseProgram(shaders[TEXTURELESS_SHADER].program);
    glUniform4f(shaders[TEXTURELESS_SHADER].uniColorLoc, 1, 1, 1, 1); 

    // glActiveTexture(GL_TEXTURE0);
    // glBindTexture(GL_TEXTURE_2D, sprites_g);

    glBindVertexArray(vao_g);
    glBindBuffer(GL_ARRAY_BUFFER, posVbo_g);

    glDisableVertexAttribArray(UV_LOC);

    glDrawArrays(GL_TRIANGLES, 0, 6);

    glEnableVertexAttribArray(UV_LOC);
}

void Graphics_RenderRect(float x, float y, u16 w, u16 h, u8 r, u8 g, u8 b, u8 a){

    glBindVertexArray(vao_g);

    u32 offset = 0;
    u16 pos[2];

    u32 k;
    for(k = 0; k < 12; k+=2){

        pos[0] = x + ((s16)RectTriangleVerts[k] * w);
        pos[1] = y + ((s16)RectTriangleVerts[k+1] * h);

        glBindBuffer(GL_ARRAY_BUFFER, posVbo_g);
        glBufferSubData(GL_ARRAY_BUFFER, offset*sizeof(pos), sizeof(pos), pos);
    
        ++offset;
    }


    glUseProgram(shaders[TEXTURELESS_SHADER].program);
    glUniform4f(shaders[TEXTURELESS_SHADER].uniColorLoc, r / 255.0f, g / 255.0f, b / 255.0f, a / 255.0f); 

    glBindVertexArray(vao_g);
    glBindBuffer(GL_ARRAY_BUFFER, posVbo_g);

    glDisableVertexAttribArray(UV_LOC);

    glDrawArrays(GL_TRIANGLES, 0, 6);

    glEnableVertexAttribArray(UV_LOC);

    glUniform4f(shaders[TEXTURELESS_SHADER].uniColorLoc, 1, 1, 1, 1); 
}

void Graphics_RenderRectLines(float x, float y, u16 w, u16 h, u8 r, u8 g, u8 b, u8 a){

    glBindVertexArray(vao_g);

    u16 buffer[2*4];

    buffer[0] = x;
    buffer[1] = y;
    buffer[2] = x + w;
    buffer[3] = y;
    buffer[4] = x + w;
    buffer[5] = y + h;
    buffer[6] = x;
    buffer[7] = y + h;

    glBindBuffer(GL_ARRAY_BUFFER, posVbo_g);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(buffer), buffer);


    glUseProgram(shaders[TEXTURELESS_SHADER].program);
    glUniform4f(shaders[TEXTURELESS_SHADER].uniColorLoc, r / 255.0f, g / 255.0f, b / 255.0f, a / 255.0f); 

    glBindVertexArray(vao_g);
    glBindBuffer(GL_ARRAY_BUFFER, posVbo_g);

    glDisableVertexAttribArray(UV_LOC);

    glDrawArrays(GL_LINE_STRIP, 0, 4);

    glEnableVertexAttribArray(UV_LOC);

    glUniform4f(shaders[TEXTURELESS_SHADER].uniColorLoc, 1, 1, 1, 1); 
}

void Graphics_UseShader(int shader){
    glUseProgram(shaders[shader].program);
}


void Graphics_SetCameraPos(Vec2 camPos){
    glUseProgram(shaders[TEXTURELESS_SHADER].program);
    glUniform2f(shaders[TEXTURELESS_SHADER].camPosLoc, camPos.x, camPos.y); 
    glUseProgram(shaders[TEXTURED_SHADER].program);
    glUniform2f(shaders[TEXTURED_SHADER].camPosLoc, camPos.x, camPos.y); 
}

void Graphics_SetCameraZoom(Vec2 zoom){
    glUseProgram(shaders[TEXTURELESS_SHADER].program);
    glUniform2f(shaders[TEXTURELESS_SHADER].zoomLoc, zoom.x, zoom.y); 
    glUseProgram(shaders[TEXTURED_SHADER].program);
    glUniform2f(shaders[TEXTURED_SHADER].zoomLoc, zoom.x, zoom.y); 
}

void Graphics_SetAbberation(u8 set){
    glUseProgram(shaders[QUAD_SHADER].program);
    glUniform1i(shaders[QUAD_SHADER].abberationLoc, set); 
}