#include <GL/glew.h>
#include <stdio.h>
#include "graphics.h"
#include "math.h"
#include "window.h"
#include "memory.h"
#include "log.h"
#include "rotateguy.h"

#define TEXTURE_PATH "resources/texture.img"
#define FONT_PATH "resources/font.img"
#define SPRITES_PATH "resources/sprites.img"

#define STRDEF(x) STR(x)
#define STR(x) #x

#define POS_ATTRIB "pos"
#define UV_ATTRIB "uv"

// #define MAX_ON_SCREEN_SPITES 128

#define FONT_SIZE 64
#define FONT_SIZE_BITS 3
#define FONT_SIZE_MASK 7

#define TEXTURE_SIZE 128
#define TEXTURE_SIZE_BITS 3
#define TEXTURE_SIZE_MASK 7

#define SPRITES_SIZE 256

#define BUFFER_STRIDE 6
// 6 verts per tri, 2(s16)=4 + 2(u8)=2 = 6 bytes per vert * 6 verts per rect = 36
#define RENDER_VRAM_SIZE ((TILES_X*TILES_Y) * 36)

#ifdef DEBUG
#define TEXTURELESS_RENDER_VRAM_SIZE (256 * TEXTURELESS_BUFFER_STRIDE)
#define TEXTURELESS_BUFFER_STRIDE 4
#define DEBUG_COLOR 1,0,1
#endif

enum {
	POS_LOC = 0,
	UV_LOC,
};

typedef struct {
	u32 	program;
	u32 	fShader;
	u32 	vShader;
	u32		uniColorLoc;
	u32 	invTexSizeLoc;
} Shader_t;

typedef struct {
    s16     x;
    s16     y;
    u8      z;
    u8      w;
} PosUV_t;

static const u8 	RectTriangleVerts[] = {0,0,1,0,1,1,1,1,0,1,0,0};

#ifdef DEBUG
static Shader_t     textureless_shader_g;
static u32          textureless_vbo_g;
static u32          textureless_vao_g;
#endif

static Shader_t     shader_g;
static u32          vbo_g;
static u32          vao_g;
static u32 			texture_g;
static u32 			font_g;
static u32 			sprites_g;
static u32      	fb_g;
static u32      	fbTexture_g;
static u32 			numSprites_g;

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
        LOG(LOG_RED, "FSHADER: %s\n", buffer);
        return;
    }

    glAttachShader(shader->program, shader->fShader);

    glShaderSource(shader->vShader, 1, (const GLchar **)&vSource, NULL);
    glCompileShader(shader->vShader);
    glGetShaderiv(shader->vShader, GL_COMPILE_STATUS, &status);

    if(status != GL_TRUE){
        glGetShaderInfoLog(shader->vShader, 512, NULL, buffer);
        LOG(LOG_RED, "VSHADER: %s\n", buffer);
        return;
    }

    glBindAttribLocation(shader->program, POS_LOC, POS_ATTRIB);
    glBindAttribLocation(shader->program, UV_LOC, UV_ATTRIB);

    glAttachShader(shader->program, shader->vShader);
    glLinkProgram(shader->program);

    glUseProgram(shader->program);

    shader->invTexSizeLoc = glGetUniformLocation(shader->program, "invTexSize");
    shader->uniColorLoc = glGetUniformLocation(shader->program, "uniformColor");
}

static void LoadTexture(u32 *texture, const char *path, u32 w, u32 h){

	glGenTextures(1, texture);
	glBindTexture(GL_TEXTURE_2D, *texture);

	FILE *fp = fopen(path, "rb");

	if(!fp){
		LOG(LOG_RED, "Failed to load texture: %s\n", path)
		return;
	}

	u8 *data = Memory_Alloc(STACK_BOTTOM, w*h*2);

	fread(data, 1, w*h*2, fp);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB5_A1, w, h, 0, GL_BGRA, GL_UNSIGNED_SHORT_5_5_5_1, data);

	Memory_Pop(STACK_BOTTOM, 1);

	fclose(fp);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, 0);
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

uniform vec2 invTexSize;

void main(){

    TexCoord = uv * invTexSize;
    gl_Position = vec4(vec2(pos.x * invWidth, 2-(pos.y * InvHeight)) - 1, -1, 1);
}
);

static const char *FS_Source = "#version 120\n"
STR(
varying vec2 TexCoord;
uniform sampler2D tex;
uniform vec3 uniformColor = vec3(1,1,1);

void main(){

    vec4 color = vec4(uniformColor,1) * texture2D(tex, TexCoord);
    if(color.a < 0.5) discard;

    gl_FragColor = color;
}
);

#ifdef DEBUG

static const char *VS_Textureless_Source = "#version 120\n"
STR(
attribute vec2 pos;
)

STR(uniform float invWidth = 2.0f/) STRDEF(WIDTH) STR(;)
STR(uniform float InvHeight = 2.0f/) STRDEF(HEIGHT) STR(;)

STR(

void main(){

    gl_Position = vec4(vec2(pos.x * invWidth, 2-(pos.y * InvHeight)) - 1, -1, 1);
}
);

static const char *FS_Textureless_Source = "#version 120\n"
STR(
uniform vec3 uniformColor = vec3(1,1,1);

void main(){

    gl_FragColor = vec4(uniformColor,1);
}
);

#endif

static void CreateFrameBuffer(void){

    glGenFramebuffers(1,&fb_g);
    glBindFramebuffer(GL_FRAMEBUFFER, fb_g);

    glGenTextures(1, &fbTexture_g);
    glBindTexture(GL_TEXTURE_2D, fbTexture_g);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, WIDTH, HEIGHT, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, fbTexture_g, 0);

    glDrawBuffer(GL_COLOR_ATTACHMENT0);

    if(glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE){
        glDeleteFramebuffers(1, &fb_g);
        glDeleteTextures(1, &fbTexture_g);
        LOG(LOG_RED, "Error creating framebuffer.\n");
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Graphics_Init(void){

    CreateFrameBuffer();

	LoadTexture(&font_g, FONT_PATH, FONT_SIZE, FONT_SIZE);
	LoadTexture(&texture_g, TEXTURE_PATH, TEXTURE_SIZE, TEXTURE_SIZE);
	LoadTexture(&sprites_g, SPRITES_PATH, SPRITES_SIZE, SPRITES_SIZE);

    Compile(&shader_g, VS_Source, FS_Source);
    
#ifdef DEBUG

    Compile(&textureless_shader_g, VS_Textureless_Source, FS_Textureless_Source);

    // textureless

    glGenVertexArrays(1, &textureless_vao_g);
    glBindVertexArray(textureless_vao_g);
    glGenBuffers(1, &textureless_vbo_g);
    glBindBuffer(GL_ARRAY_BUFFER, textureless_vbo_g);
    glBufferData(GL_ARRAY_BUFFER, TEXTURELESS_RENDER_VRAM_SIZE, NULL, GL_DYNAMIC_DRAW);

    glEnableVertexAttribArray(POS_LOC);
    glVertexAttribPointer(POS_LOC, 2, GL_SHORT, GL_FALSE, 0, 0);

#endif
    // textured

	glGenBuffers(1, &vbo_g);
	glBindBuffer(GL_ARRAY_BUFFER, vbo_g);
	glBufferData(GL_ARRAY_BUFFER, RENDER_VRAM_SIZE, NULL, GL_DYNAMIC_DRAW);

	glGenVertexArrays(1, &vao_g);
	glBindVertexArray(vao_g);
	glBindBuffer(GL_ARRAY_BUFFER, vbo_g);

    glEnableVertexAttribArray(POS_LOC);
    glVertexAttribPointer(POS_LOC, 2, GL_SHORT, GL_FALSE, BUFFER_STRIDE, 0);

    glEnableVertexAttribArray(UV_LOC);
    glVertexAttribPointer(UV_LOC, 2, GL_UNSIGNED_BYTE, GL_FALSE, BUFFER_STRIDE, (void*)(sizeof(s16)*2));

    glBindFramebuffer(GL_FRAMEBUFFER, fb_g);
    glViewport(0, 0, WIDTH, HEIGHT);

    glClearColor(1,1,1,1);
    glClear(GL_COLOR_BUFFER_BIT);
}

void Graphics_Close(void){

    glDeleteFramebuffers(1, &fb_g);
    glDeleteTextures(1, &fbTexture_g);

    glDeleteProgram(shader_g.program);
    glDeleteShader(shader_g.fShader);
    glDeleteShader(shader_g.vShader);
    glDeleteVertexArrays(1, &vao_g);
    glDeleteBuffers(1, &vbo_g);

#ifdef DEBUG
    glDeleteProgram(textureless_shader_g.program);
    glDeleteShader(textureless_shader_g.fShader);
    glDeleteShader(textureless_shader_g.vShader);
    glDeleteVertexArrays(1, &textureless_vao_g);
    glDeleteBuffers(1, &textureless_vbo_g);

#endif

    glDeleteTextures(1, &font_g);
    glDeleteTextures(1, &sprites_g);
    glDeleteTextures(1, &texture_g);
}

void Graphics_Resize(void){
}

void Graphics_Render(void){

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    glBindVertexArray(vao_g);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_g);

    u32 offset = 0;

    PosUV_t posuv;

    u32 k;
    for(k = 0; k < 12; k+=2){

        posuv.x = (s16)RectTriangleVerts[k] * WIDTH;
        posuv.y = (s16)RectTriangleVerts[k+1] * HEIGHT;
        posuv.z = (u8)RectTriangleVerts[k];
        posuv.w = (u8)1-RectTriangleVerts[k+1];

        glBufferSubData(GL_ARRAY_BUFFER, offset, sizeof(PosUV_t), &posuv);
        offset += BUFFER_STRIDE;
    }

    glViewport(0, 0, GAME_DEFAULT_WIDTH, GAME_DEFAULT_HEIGHT);

    glUseProgram(shader_g.program);
    glUniform3f(shader_g.uniColorLoc, 1, 1, 1); 
    glUniform2f(shader_g.invTexSizeLoc, 1, 1); 

    glBindTexture(GL_TEXTURE_2D, fbTexture_g);
    glDrawArrays(GL_TRIANGLES, 0, 6);


    glBindFramebuffer(GL_FRAMEBUFFER, fb_g);
    glViewport(0, 0, WIDTH, HEIGHT);

	glClearColor(1,1,1,1);
    glClear(GL_COLOR_BUFFER_BIT);

}

void Graphics_RenderString(char *str, u32 xpos, u32 ypos, u8 r, u8 g, u8 b){

	glUseProgram(shader_g.program);
	glUniform3f(shader_g.uniColorLoc, r / 255.0f, g  / 255.0f, b  / 255.0f); 
	glUniform2f(shader_g.invTexSizeLoc, 1.0f / (float)FONT_SIZE, 1.0f / (float)FONT_SIZE); 

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, font_g);

	glBindVertexArray(vao_g);
	glBindBuffer(GL_ARRAY_BUFFER, vbo_g);

	u32 offset = 0;
    u32 num = 0;

    PosUV_t posuv;

    u32 k;

    char p;
    while((p = *str++)){

    	if(p >= 'A' && p <= 'Z')
    		p = p-'A';
    	else if(p >= 'a' && p <= 'z')
    		p = p-'a';
    	else if(p == '.')
    		p = 26;
		else if(p >= '0' && p <= '9')
			p = (p-'0') + 27;
		else
			continue;

	    for(k = 0; k < 12; k+=2){

	    	posuv.x = ((s16)RectTriangleVerts[k] << 3) + xpos;
	    	posuv.y = ((s16)RectTriangleVerts[k+1] << 3) + ypos;
	    	posuv.z = (u8)(RectTriangleVerts[k] + (p & FONT_SIZE_MASK)) << 3;
	    	posuv.w = (u8)(RectTriangleVerts[k+1] + (p >> FONT_SIZE_BITS)) << 3;

            glBufferSubData(GL_ARRAY_BUFFER, offset, sizeof(PosUV_t), &posuv);
	    	offset += BUFFER_STRIDE;
	    }
	    
	    xpos += 9;
	    num++;
    }


    glDrawArrays(GL_TRIANGLES, 0, 6*num);
}

void Graphics_RenderTileMap(u32 sx, u32 sy, const u8 *map, u16 mapW, u16 mapH){

    glBindVertexArray(vao_g);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_g);

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

	u32 offset = 0, num = 0;

	u32 x, y, k;

    PosUV_t posuv;

	for(y = 0; y < MIN(TILES_Y, mapH-sy); y++){
		
		for(x = 0; x < MIN(TILES_X, mapW-sx); x++){

			tile = map[((y+sy)*mapW) + x + sx];

			if(!tile) continue;

			--tile;

		    for(k = 0; k < 12; k+=2){

		    	posuv.x = (((s16)RectTriangleVerts[k]+x) << TILE_SIZE_BITS) - cx;
		    	posuv.y = (((s16)RectTriangleVerts[k+1]+y) << TILE_SIZE_BITS) - cy;
		    	posuv.z = ((u8)(RectTriangleVerts[k] + (tile & TEXTURE_SIZE_MASK)) << TILE_SIZE_BITS);
		    	posuv.w = ((u8)(RectTriangleVerts[k+1] + (tile >> TEXTURE_SIZE_BITS)) << TILE_SIZE_BITS);

                glBufferSubData(GL_ARRAY_BUFFER, offset, sizeof(PosUV_t), &posuv);
		    	offset += BUFFER_STRIDE;
		    }

		    ++num;
		}
	}

	glUseProgram(shader_g.program);
	glUniform3f(shader_g.uniColorLoc, 1, 1, 1); 
	glUniform2f(shader_g.invTexSizeLoc, 1.0f / (float)TEXTURE_SIZE, 1.0f / (float)TEXTURE_SIZE); 

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture_g);

    glDrawArrays(GL_TRIANGLES, 0, 6*num);
}

void Graphics_BeginRenderSprites(void){

	numSprites_g = 0;
}

void Graphics_EndRenderSprites(void){

	glUseProgram(shader_g.program);
	glUniform3f(shader_g.uniColorLoc, 1, 1, 1); 
	glUniform2f(shader_g.invTexSizeLoc, 1.0f / (float)SPRITES_SIZE, 1.0f / (float)SPRITES_SIZE); 

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, sprites_g);

	glBindVertexArray(vao_g);
	glBindBuffer(GL_ARRAY_BUFFER, vbo_g);

    glDrawArrays(GL_TRIANGLES, 0, 6*numSprites_g);
}

void Graphics_RenderSprite(u16 x, u16 y, u8 w, u8 h, float rotation, s16 ox, s16 oy, u8 tx, u8 ty){

    glBindVertexArray(vao_g);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_g);

    float sintheta = sinf(rotation);
    float costheta = cosf(rotation);

    ox += w/2;
    oy += h/2;

    u32 offset = numSprites_g * BUFFER_STRIDE * 6;

    PosUV_t posuv;

    s16 relx, rely;

    u32 k;
    for(k = 0; k < 12; k+=2){

    	relx = ((s16)RectTriangleVerts[k] * w) - ox;
    	rely = ((s16)RectTriangleVerts[k+1] * h) - oy;

        posuv.x = x + ox + ( (relx * costheta) - (sintheta * rely) );
        posuv.y = y + oy + ( (relx * sintheta) + (costheta * rely) );

    	posuv.z = ((u8)(RectTriangleVerts[k] * w) + tx);
    	posuv.w = ((u8)(RectTriangleVerts[k+1] * h) + ty);

        glBufferSubData(GL_ARRAY_BUFFER, offset, sizeof(PosUV_t), &posuv);
    	offset += BUFFER_STRIDE;
    }

    ++numSprites_g;

}

#ifdef DEBUG

void Graphics_RenderCircle(s16 x, s16 y, s16 radius, u16 num, float rotation){

    glUseProgram(textureless_shader_g.program);
    glUniform3f(textureless_shader_g.uniColorLoc, DEBUG_COLOR); 

    glBindVertexArray(textureless_vao_g);
    glBindBuffer(GL_ARRAY_BUFFER, textureless_vbo_g);

#define MAX_CIRCLE_VERTS 32

    num = num > MAX_CIRCLE_VERTS ? MAX_CIRCLE_VERTS : num;

    float radians = (PI*2)/num;

    s16 pos[2];

    pos[0] = (s16)x;
    pos[1] = (s16)y;
    
    glBufferSubData(GL_ARRAY_BUFFER, 0, TEXTURELESS_BUFFER_STRIDE, pos);
    
    u32 offset = TEXTURELESS_BUFFER_STRIDE;

    u16 k;
    for(k = 1; k < num+2; k++){

        pos[0] = (cosf(rotation) * radius) + x;
        pos[1] = (-sinf(rotation) * radius) + y;

        glBufferSubData(GL_ARRAY_BUFFER, offset, TEXTURELESS_BUFFER_STRIDE, pos);
        offset += TEXTURELESS_BUFFER_STRIDE;

        rotation += radians;

    }

    glCullFace(GL_BACK);
    glDrawArrays(GL_TRIANGLE_FAN, 0, num+2);
    glCullFace(GL_FRONT);
}

void Graphics_RenderCapsule(s16 x, s16 y, s16 radius, s16 segment, float rotation){


    float sintheta = sinf(rotation);
    float costheta = cosf(rotation);

    Vec2 segmentVector = (Vec2){-sintheta * segment, costheta * segment};
    // Vec2 segmentVectorNorm = (Vec2){segmentVector.y, -segmentVector.x};

    Vec2 segPoint1 = (Vec2){x - (segmentVector.x/2), y - (segmentVector.y/2)};
    Vec2 segPoint2 = (Vec2){x + (segmentVector.x/2), y + (segmentVector.y/2)};

    Graphics_RenderCircle(segPoint1.x, segPoint1.y, radius, 16, rotation);
    Graphics_RenderCircle(segPoint2.x, segPoint2.y, radius, 16, rotation);

    s16 pos[2];

    u32 offset = 0;

    float relx, rely;

    glUseProgram(textureless_shader_g.program);
    glUniform3f(textureless_shader_g.uniColorLoc, DEBUG_COLOR); 

    glBindVertexArray(textureless_vao_g);
    glBindBuffer(GL_ARRAY_BUFFER, textureless_vbo_g);


    u32 k;
    for(k = 0; k < 12; k+=2){

        relx = (((float)RectTriangleVerts[k] - 0.5) * radius * 2);
        rely = (((float)RectTriangleVerts[k+1] - 0.5) * segment);

        pos[0] = x + ( (relx * costheta) - (sintheta * rely) );
        pos[1] = y + ( (relx * sintheta) + (costheta * rely) );

        glBufferSubData(GL_ARRAY_BUFFER, offset, TEXTURELESS_BUFFER_STRIDE, pos);
        offset += TEXTURELESS_BUFFER_STRIDE;
    }

    glDrawArrays(GL_TRIANGLES, 0, 6);
}

void Graphics_RenderRect(u16 x, u16 y, u8 w, u8 h, float rotation, s16 ox, s16 oy){

    glUseProgram(textureless_shader_g.program);
    glUniform3f(textureless_shader_g.uniColorLoc, DEBUG_COLOR); 

    glBindVertexArray(textureless_vao_g);
    glBindBuffer(GL_ARRAY_BUFFER, textureless_vbo_g);

    float sintheta = sinf(rotation);
    float costheta = cosf(rotation);

    ox += w/2;
    oy += h/2;

    u32 offset = 0;

    s16 pos[2];

    s16 relx, rely;

    u32 k;
    for(k = 0; k < 12; k+=2){

        relx = ((s16)RectTriangleVerts[k] * w) - ox;
        rely = ((s16)RectTriangleVerts[k+1] * h) - oy;

        pos[0] = x + ox + ( (relx * costheta) - (sintheta * rely) );
        pos[1] = y + oy + ( (relx * sintheta) + (costheta * rely) );

        glBufferSubData(GL_ARRAY_BUFFER, offset, TEXTURELESS_BUFFER_STRIDE, pos);
        offset += TEXTURELESS_BUFFER_STRIDE;
    }

    glDrawArrays(GL_TRIANGLES, 0, 6);

}

#endif