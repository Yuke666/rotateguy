#include <GL/glew.h>
#include <SDL2/SDL.h>
#include <string.h>
#include "renderer.h"
#include "log.h"
#include "math.h"

#define MILKDROP_DEFAULT_DIVISIONS 16

#define STR(x) #x

#define POS_ATTRIB              "pos"
#define UV_ATTRIB               "uv"
#define COLOR_ATTRIB            "color"
#define NORM_ATTRIB             "norm"
#define TANGENT_ATTRIB          "tangent"
#define WEIGHTS_ATTRIB          "weights"
#define BONE_INDICES_ATTRIB     "boneIndices"
#define RAD_ATTRIB              "rad"
#define ANG_ATTRIB              "ang"

#if defined(USE_INSTANCING) && USE_INSTANCING
static const float RectTriangleStripVerts[] = {0,1,0,0,1,1,1,0};
#endif
static const float RectTriangleVerts[] = {0,0,1,0,1,1,1,1,0,1,0,0};

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
    glBindAttribLocation(shader->program, COLOR_LOC, COLOR_ATTRIB);
    glBindAttribLocation(shader->program, NORM_LOC, NORM_ATTRIB);
    glBindAttribLocation(shader->program, TANGENT_LOC, TANGENT_ATTRIB);
    glBindAttribLocation(shader->program, WEIGHTS_LOC, WEIGHTS_ATTRIB);
    glBindAttribLocation(shader->program, BONE_INDICES_LOC, BONE_INDICES_ATTRIB);

    glAttachShader(shader->program, shader->vShader);
    glLinkProgram(shader->program);

    shader->view = glGetUniformLocation(shader->program, "view");
    shader->proj = glGetUniformLocation(shader->program, "proj");
    shader->color = glGetUniformLocation(shader->program, "uniformColor");
}


static const char *textured2D_VS = "#version 120\n"
STR(
attribute vec2 pos;
attribute vec2 uv;
varying vec2 TexCoord;
uniform mat3 view = mat3(1);
uniform mat4 proj;

void main(){
    TexCoord = uv;
    gl_Position = proj * vec4(view * vec3(pos,0), 1);
}
);

static const char *textured2D_FS = "#version 120\n"
STR(
varying vec2 TexCoord;
uniform sampler2D tex;
uniform vec3 uniformColor = vec3(1,1,1);

void main(){

    vec4 color = vec4(uniformColor,1) * texture2D(tex, TexCoord);
    if(color.a < 0.5) discard;

    gl_FragColor = color;//vec4(1)-vec4(texture2D(tex, TexCoord).rgb, 0);
}
);

static const char *textureless2D_VS = "#version 120\n"
STR(
attribute vec2 pos;
uniform mat3 view = mat3(1);
uniform mat4 proj;

void main(){
    gl_Position = proj * vec4(view * vec3(pos,0), 1);
}
);

static const char *textureless2D_FS = "#version 120\n"
STR(
uniform vec3 uniformColor = vec3(1,1,1);

void main(){
    gl_FragColor = vec4(uniformColor, 1);
}
);


static const char *texturelessParticle2D_VS = "#version 120\n"
STR(
attribute vec3 pos;
attribute vec4 color;
uniform mat3 view = mat3(1);
uniform mat4 proj;
varying vec4 Color;

void main(){
    gl_Position = proj * vec4(view * pos, 1);
    Color = color; 
}
);

static const char *texturelessParticle2D_FS = "#version 120\n"
STR(
varying vec4 Color;
uniform vec3 uniformColor = vec3(1,1,1);

void main(){
    gl_FragColor = vec4(uniformColor,1);
}
);

#if defined(USE_INSTANCING) && USE_INSTANCING

static const char *text_VS = "#version 120\n"
"#extension GL_ARB_draw_instanced : enable\n"
"#define MAX_STRING_CHARS 512\n"
STR(
attribute vec2 pos;
uniform mat4 proj;
uniform sampler1D stringTex;
uniform int length = 1;
uniform float spacing = 0;
uniform float size = 8.0f;
uniform vec2 start = vec2(-1,-1);
varying vec2 TexCoord;

void main(){

    gl_Position = proj * vec4(start.x + (gl_InstanceID * (spacing+size)) + (pos.x*size), start.y + (pos.y*size), -1, 1);

    float character = (texture1D(stringTex, gl_InstanceID * (1.0 / MAX_STRING_CHARS)).r * 255);

    vec2 uv = vec2(0,0);

    uv.x = mod(character, 16) / 16.0f;
    uv.y = (1 - (1.0/16.0f)) - (floor(character / 16) / 16.0f);

    TexCoord = (vec2(pos.x, 1-pos.y)/16.0f) + uv;
}
);

static const char *text_FS = "#version 120\n"
STR(
uniform vec3 uniformColor = vec3(1,1,1);
uniform sampler2D fontTex;
varying vec2 TexCoord;

void main(){
    gl_FragColor = vec4(uniformColor, 1) * texture2D(fontTex, TexCoord).r;
}
);

#else

static const char *text_VS = "#version 120\n"
STR(
attribute vec2 pos;
attribute vec2 uv;
attribute vec3 color;
uniform mat4 proj;
varying vec2 TexCoord;
varying vec3 Color;

void main(){

    gl_Position = proj * vec4(pos, -1, 1);
    TexCoord = uv;
    Color = color;
}
);

static const char *text_FS = "#version 120\n"
STR(
uniform vec3 uniformColor = vec3(1,1,1);
uniform sampler2D fontTex;
varying vec2 TexCoord;
varying vec3 Color;

void main(){

    float color = texture2D(fontTex, TexCoord).r;
    
    if(color == 0) discard;
    
    gl_FragColor = vec4(uniformColor * Color, 1) * color;
}
);

#endif

static const char *milkdrop_FS = "#version 120\n"
STR(
uniform vec4 uniformColor = vec4(1,1,1,0.99);
uniform sampler2D tex;
varying vec2 TexCoord;
uniform float time;
uniform float aspectY = 240;
uniform float aspectX = 320;

void main(){

    vec2 coord = TexCoord;

    // float rot = sin(time * 3.14 / 10000)*rad*sin(coord.x * 3.14);

    float rot = 0.1 + (0.01 * sin(time*3.14));
    float zoom = 0.98 + (0.2 * sin(time*3.14));

    coord = ((coord - 0.5) * zoom) + 0.5;

    vec2 orig = vec2(0.5, 0.5);

    float x = coord.x - orig.x;
    float y = coord.y - orig.y;

    coord.x = orig.x + ((x * cos(rot)) - (y * sin(rot)));\n
    coord.y = orig.y + ((x * sin(rot)) + (y * cos(rot)));\n

    vec4 t = texture2D(tex, coord);
    
    if(t.a == 0) discard;

    gl_FragColor = t * uniformColor;
}
);

static const char *milkdrop_VS = "#version 120\n"
STR(

attribute vec2 pos;
attribute float rad;
attribute float ang;

uniform float time;
uniform float fps;
uniform int frame;
uniform float progress;

uniform float bass;
uniform float mid;
uniform float treb;
uniform float bass_att;
uniform float mid_att;
uniform float treb_att;

uniform float meshx;
uniform float meshy;
uniform float pixelsx;
uniform float pixelsy;

uniform float aspectY;
uniform float aspectX;

uniform float zoom;
uniform float zoomexp;
uniform float rot;
uniform float warp;
uniform float cx;
uniform float cy;
uniform float dx;
uniform float dy;
uniform float sx;
uniform float sy;

varying vec2 TexCoord;

varying float zoom_out;
varying float zoomexp_out;
varying float rot_out;
varying float warp_out;
varying float cx_out;
varying float cy_out;
varying float dx_out;
varying float dy_out;
varying float sx_out;
varying float sy_out;

void main(){
    gl_Position = vec4(pos, -1, 1);
    TexCoord = vec2(pos * 0.5 + 0.5);
}
);

static void GenerateColorTex(u32 w, u32 h, GLuint *tex, u32 filterType, u32 format, u32 type){
    glGenTextures(1, tex);
    glBindTexture(GL_TEXTURE_2D, *tex);

    glTexImage2D(GL_TEXTURE_2D, 0, format, w, h, 0, GL_RGBA, type, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filterType);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filterType);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
}


void Renderer_BindColorBuffer(u32 mode, ColorBuffer_t cBuffer){
    
    glBindFramebuffer(mode, cBuffer.fb);    

    GLuint drawBuffers[MAX_FB_TEXTURES];

    u32 k;
    for(k = 0; k < cBuffer.nTextures; k++)
        drawBuffers[k] = GL_COLOR_ATTACHMENT0 + k;
    
    glDrawBuffers(cBuffer.nTextures, drawBuffers);
}

void Renderer_CreateColorBuffer(u32 w, u32 h, u32 num, u32 formats[], u32 types[], ColorBuffer_t *cBuffer){

    cBuffer->width = w;
    cBuffer->height = h;
    cBuffer->nTextures = num;

    glGenFramebuffers(1,&cBuffer->fb);
    glBindFramebuffer(GL_FRAMEBUFFER, cBuffer->fb);

    GLuint drawBuffers[MAX_FB_TEXTURES];

    u32 k;
    for(k = 0; k < cBuffer->nTextures; k++){
        GenerateColorTex(w, h, &cBuffer->textures[k], GL_NEAREST, formats[k], types[k]);
        glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + k, GL_TEXTURE_2D, cBuffer->textures[k], 0);
        drawBuffers[k] = GL_COLOR_ATTACHMENT0 + k;
    }

    glDrawBuffers(cBuffer->nTextures, drawBuffers);

    if(glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE){
        Renderer_DestroyColorBuffer(cBuffer);
        LOG(LOG_RED, "Renderer_CreateColorBuffer: Error creating color buffer.\n");
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Renderer_ResizeColorBuffer(ColorBuffer_t *cBuffer, u32 w, u32 h, u32 formats[], u32 types[]){

    u32 k;
    for(k = 0; k < cBuffer->nTextures; k++){
        glBindTexture(GL_TEXTURE_2D, cBuffer->textures[k]);
        glTexImage2D(GL_TEXTURE_2D, 0, formats[k], w, h, 0, GL_RGBA, types[k], NULL);
    }
}

void Renderer_DestroyColorBuffer(ColorBuffer_t *cBuffer){
    glDeleteFramebuffers(1, &cBuffer->fb);

    u32 k;
    for(k = 0; k < cBuffer->nTextures; k++)
        glDeleteTextures(1, &cBuffer->textures[k]);
}

static void UpdateProjectionMatrix(Renderer_t *renderer, float *proj){
    glUseProgram(renderer->shaders[SHADER_TEXTURED_2D].program);
    glUniformMatrix4fv(renderer->shaders[SHADER_TEXTURED_2D].proj, 1, GL_TRUE, proj);
    glUseProgram(renderer->shaders[SHADER_TEXTURELESS_2D].program);
    glUniformMatrix4fv(renderer->shaders[SHADER_TEXTURELESS_2D].proj, 1, GL_TRUE, proj);
    glUseProgram(renderer->shaders[SHADER_TEXT].program);
    glUniformMatrix4fv(renderer->shaders[SHADER_TEXT].proj, 1, GL_TRUE, proj);
    glUseProgram(renderer->shaders[SHADER_TEXTURELESS_PARTICLES_2D].program);
    glUniformMatrix4fv(renderer->shaders[SHADER_TEXTURELESS_PARTICLES_2D].proj, 1, GL_TRUE, proj);
}

static void ResizeGrid(Grid_t *grid, u32 divisions, float aspectX, float aspectY){

    u32 nVerts = divisions*divisions;
    u32 nElements = nVerts * 6;

    grid->nElements = nElements;

    glBindVertexArray(grid->vao);

    glBindBuffer(GL_ARRAY_BUFFER, grid->vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float)*4*nVerts, NULL, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, grid->ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned short) * nElements * 6, NULL, GL_STATIC_DRAW);

    unsigned short elems[6], y, x;

    long int offset = 0;

    float invMag = 1.0f / sqrt(aspectX*aspectX + aspectY*aspectY);

    float verty, vertx, px, py, rad, ang;

    for(y = 0; y < divisions; y++){
        for(x = 0; x < divisions; x++){

            verty = (y / (float)(divisions-1)) * 2 - 1;
            vertx = (x / (float)(divisions-1)) * 2 - 1;

            px = vertx * aspectX;
            py = verty * aspectY;

            rad = sqrt(px*px + py*py) * invMag;
            ang = atan2(py, px);
            
            glBufferSubData(GL_ARRAY_BUFFER, offset, sizeof(float), &vertx);
            offset += sizeof(float);
            glBufferSubData(GL_ARRAY_BUFFER, offset, sizeof(float), &verty);
            offset += sizeof(float);
            glBufferSubData(GL_ARRAY_BUFFER, offset, sizeof(float), &rad);
            offset += sizeof(float);
            glBufferSubData(GL_ARRAY_BUFFER, offset, sizeof(float), &ang);
            offset += sizeof(float);
        }
    }

    offset = 0;

    for(y = 0; y < divisions; y++){
        for(x = 0; x < divisions; x++){

            elems[0] = (y+1)*divisions + x;
            elems[1] = (y*divisions) + x;
            elems[2] = (y*divisions) + x + 1;
            elems[3] = elems[2];
            elems[4] = elems[0]+1;
            elems[5] = elems[0];

            glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, offset, sizeof(unsigned short)*6, elems);
            offset += sizeof(unsigned short)*6;
        }
    }
}

void Renderer_Init(Renderer_t *renderer, u32 w, u32 h){

    float proj[16];
    Math_Ortho(proj, 0, w, 0, h, GAME_NEAR, GAME_FAR);

    renderer->width = w;
    renderer->height = h;
    renderer->aspectX = h/(float)w;
    renderer->aspectY = w/(float)h;

	Compile(&renderer->shaders[SHADER_TEXTURED_2D], textured2D_VS, textured2D_FS);
    Compile(&renderer->shaders[SHADER_TEXTURELESS_2D], textureless2D_VS, textureless2D_FS);
    Compile(&renderer->shaders[SHADER_TEXT], text_VS, text_FS);
    Compile(&renderer->shaders[SHADER_TEXTURELESS_PARTICLES_2D], texturelessParticle2D_VS, texturelessParticle2D_FS);

    // init particles
    glGenVertexArrays(1, &renderer->particles.vao);
    glBindVertexArray(renderer->particles.vao);
    glGenBuffers(1, &renderer->particles.vbo);
 
    glBindBuffer(GL_ARRAY_BUFFER,renderer->particles.vbo);
 
// #if defined(USE_INSTANCING) && USE_INSTANCING
    // glVertexAttribDivisor(POS_LOC, 0);
    // glVertexAttribDivisor(UV_LOC, 0);
    // glVertexAttribDivisor(COLOR_LOC, 0);
// #else
    // 6 verts per particle
    glEnableVertexAttribArray(POS_LOC);
    glVertexAttribPointer(POS_LOC, 3, GL_FLOAT, GL_FALSE, sizeof(float)*9, 0);
    glEnableVertexAttribArray(UV_LOC);
    glVertexAttribPointer(UV_LOC, 2, GL_FLOAT, GL_FALSE, sizeof(float)*9, (void*)sizeof(Vec3));
    glEnableVertexAttribArray(COLOR_LOC);
    glVertexAttribPointer(COLOR_LOC, 4, GL_FLOAT, GL_FALSE, sizeof(float)*9, (void*)(sizeof(float)*5));
    glBufferData(GL_ARRAY_BUFFER, (sizeof(float)*9) * RENDERER_MAX_PARTICLES * 6, NULL, GL_STATIC_DRAW);
// #endif

    glBindVertexArray(0);
    
    // init text

    Shader_t *shader = &renderer->shaders[SHADER_TEXT];

#if defined(USE_INSTANCING) && USE_INSTANCING
    shader->text.fontTex = glGetUniformLocation(shader->program, "fontTex");
    shader->text.stringTex = glGetUniformLocation(shader->program, "stringTex");
    shader->text.start = glGetUniformLocation(shader->program, "start");
    shader->text.size = glGetUniformLocation(shader->program, "size");
    shader->text.spacing = glGetUniformLocation(shader->program, "spacing");
    shader->text.length = glGetUniformLocation(shader->program, "length");

    glGenTextures(1, &renderer->stringTex);
    glBindTexture(GL_TEXTURE_1D, renderer->stringTex);

    glTexImage1D(GL_TEXTURE_1D, 0, GL_RED, RENDERER_MAX_STRING_CHARS, 0, GL_RED, GL_UNSIGNED_BYTE, 0);

    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);RectTriangleVerts[
    glTexParameteri(GL_TEXTURE_1D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_1D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_1D, 0);
#endif


#if defined(USE_INSTANCING) && USE_INSTANCING
    glGenVertexArrays(1, &renderer->text.vao);
    glBindVertexArray(renderer->text.vao);
    glGenBuffers(1, &renderer->text.vbo);
    glBindBuffer(GL_ARRAY_BUFFER,renderer->text.vbo);
    glEnableVertexAttribArray(POS_LOC);
    glVertexAttribPointer(POS_LOC, 2, GL_FLOAT, GL_FALSE, 0, 0);
    glBufferData(GL_ARRAY_BUFFER, sizeof(RectTriangleStripVerts), (void *)RectTriangleStripVerts, GL_STATIC_DRAW);
    glVertexAttribDivisor(POS_LOC, 0);
#else
    Renderer_CreateTextVao(&renderer->text.vao, &renderer->text.vbo);
#endif

    glBindVertexArray(0);

    extern const u8 font[];

    glGenTextures(1, &renderer->fontTex);
    glBindTexture(GL_TEXTURE_2D, renderer->fontTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, 128, 128, 0, GL_RED, GL_UNSIGNED_BYTE, font);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, 0);

    glUseProgram(shader->program);

#if defined(USE_INSTANCING) && USE_INSTANCING
    glUniform1i(shader->text.stringTex, 1);
#endif
    glUniform1i(shader->text.fontTex, 0);

    // textured rect

    glGenVertexArrays(1, &renderer->texturedRenderer.vao);
    glBindVertexArray(renderer->texturedRenderer.vao);
    glGenBuffers(1, &renderer->texturedRenderer.vbo);

    glBindBuffer(GL_ARRAY_BUFFER,renderer->texturedRenderer.vbo);

    glEnableVertexAttribArray(POS_LOC);
    glVertexAttribPointer(POS_LOC, 2, GL_FLOAT, GL_FALSE, sizeof(Vec4), 0);

    glEnableVertexAttribArray(UV_LOC);
    glVertexAttribPointer(UV_LOC, 2, GL_FLOAT, GL_FALSE, sizeof(Vec4), (void*)sizeof(Vec2));

    glBindVertexArray(0);

    // textureless rect

    glGenVertexArrays(1, &renderer->texturelessRenderer.vao);
    glBindVertexArray(renderer->texturelessRenderer.vao);
    glGenBuffers(1, &renderer->texturelessRenderer.vbo);

    glBindBuffer(GL_ARRAY_BUFFER,renderer->texturelessRenderer.vbo);

    glEnableVertexAttribArray(POS_LOC);
    glVertexAttribPointer(POS_LOC, 2, GL_FLOAT, GL_FALSE, 0, 0);

    glBindVertexArray(0);


    // milkdrop

    shader = &renderer->shaders[SHADER_MILKDROP];

    Compile(shader, milkdrop_VS, milkdrop_FS);

    glUseProgram(shader->program);
    Renderer_CreateColorBuffer(w, h, 1, (u32[]){GL_RGBA}, (u32[]){GL_UNSIGNED_BYTE}, &renderer->milkdrop.buffers[0]);
    Renderer_CreateColorBuffer(w, h, 1, (u32[]){GL_RGBA}, (u32[]){GL_UNSIGNED_BYTE}, &renderer->milkdrop.buffers[1]);

    shader->milkdrop.time       = glGetUniformLocation(shader->program, "time");
    shader->milkdrop.fps        = glGetUniformLocation(shader->program, "fps");
    shader->milkdrop.frame      = glGetUniformLocation(shader->program, "frame");
    shader->milkdrop.progress   = glGetUniformLocation(shader->program, "progress");
    shader->milkdrop.bass       = glGetUniformLocation(shader->program, "bass");
    shader->milkdrop.mid        = glGetUniformLocation(shader->program, "mid");
    shader->milkdrop.treb       = glGetUniformLocation(shader->program, "treb");
    shader->milkdrop.bass_att   = glGetUniformLocation(shader->program, "bass_att");
    shader->milkdrop.mid_att    = glGetUniformLocation(shader->program, "mid_att");
    shader->milkdrop.treb_att   = glGetUniformLocation(shader->program, "treb_att");
    shader->milkdrop.meshx      = glGetUniformLocation(shader->program, "meshx");
    shader->milkdrop.meshy      = glGetUniformLocation(shader->program, "meshy");
    shader->milkdrop.pixelsx    = glGetUniformLocation(shader->program, "pixelsx");
    shader->milkdrop.pixelsy    = glGetUniformLocation(shader->program, "pixelsy");
    shader->milkdrop.aspectY    = glGetUniformLocation(shader->program, "aspectY");
    shader->milkdrop.aspectX    = glGetUniformLocation(shader->program, "aspectX");
    shader->milkdrop.zoom       = glGetUniformLocation(shader->program, "zoom");
    shader->milkdrop.zoomexp    = glGetUniformLocation(shader->program, "zoomexp");
    shader->milkdrop.rot        = glGetUniformLocation(shader->program, "rot");
    shader->milkdrop.warp       = glGetUniformLocation(shader->program, "warp");
    shader->milkdrop.cx         = glGetUniformLocation(shader->program, "cx");
    shader->milkdrop.cy         = glGetUniformLocation(shader->program, "cy");
    shader->milkdrop.dx         = glGetUniformLocation(shader->program, "dx");
    shader->milkdrop.dy         = glGetUniformLocation(shader->program, "dy");
    shader->milkdrop.sx         = glGetUniformLocation(shader->program, "sx");
    shader->milkdrop.sy         = glGetUniformLocation(shader->program, "sy");


    Renderer_BindColorBuffer(GL_FRAMEBUFFER, renderer->milkdrop.buffers[0]);
    glClear(GL_COLOR_BUFFER_BIT);
    Renderer_BindColorBuffer(GL_FRAMEBUFFER, renderer->milkdrop.buffers[1]);
    glClear(GL_COLOR_BUFFER_BIT);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    glGenVertexArrays(1, &renderer->milkdrop.grid.vao);
    glBindVertexArray(renderer->milkdrop.grid.vao);
    glGenBuffers(1, &renderer->milkdrop.grid.vbo);
    glGenBuffers(1, &renderer->milkdrop.grid.ebo);

    glBindBuffer(GL_ARRAY_BUFFER, renderer->milkdrop.grid.vbo);

    {
        u32 stride = sizeof(float)*4;

        glEnableVertexAttribArray(POS_LOC);
        glVertexAttribPointer(POS_LOC, 2, GL_FLOAT, GL_FALSE, stride, 0);

        u32 radLoc = glGetAttribLocation(shader->program, "rad");
        u32 angLoc = glGetAttribLocation(shader->program, "ang");

        glEnableVertexAttribArray(radLoc);
        glVertexAttribPointer(radLoc, 1, GL_FLOAT, GL_FALSE, stride, (void *)(sizeof(float)*2));

        glEnableVertexAttribArray(angLoc);
        glVertexAttribPointer(angLoc, 1, GL_FLOAT, GL_FALSE, stride, (void *)(sizeof(float)*3));

    }

    glUniform1f(shader->milkdrop.aspectX, renderer->aspectX);
    glUniform1f(shader->milkdrop.aspectY, renderer->aspectY);

    ResizeGrid(&renderer->milkdrop.grid, MILKDROP_DEFAULT_DIVISIONS, renderer->aspectX, renderer->aspectY);


    // set projection uniforms

    UpdateProjectionMatrix(renderer, proj);
}

void Renderer_Close(Renderer_t *renderer){

	int k;
	for(k = 0; k < NUM_SHADERS; k++){
	    glDeleteProgram(renderer->shaders[k].program);
	    glDeleteShader(renderer->shaders[k].fShader);
	    glDeleteShader(renderer->shaders[k].vShader);
	}

    glDeleteTextures(1, &renderer->fontTex);
#if defined(USE_INSTANCING) && USE_INSTANCING
    glDeleteTextures(1, &renderer->stringTex);
#endif
    Renderer_DestroyTextVao(&renderer->text.vao, &renderer->text.vbo);
    glDeleteVertexArrays(1, &renderer->particles.vao);
    glDeleteBuffers(1, &renderer->particles.vbo);


    Renderer_DestroyColorBuffer(&renderer->milkdrop.buffers[0]);
    Renderer_DestroyColorBuffer(&renderer->milkdrop.buffers[1]);

    glDeleteVertexArrays(1, &renderer->milkdrop.grid.vao);
    glDeleteBuffers(1, &renderer->milkdrop.grid.vbo);
    glDeleteBuffers(1, &renderer->milkdrop.grid.ebo);

}

void Renderer_DestroyTextVao(u32 *vao, u32 *vbo){
    glDeleteVertexArrays(1, vao);
    glDeleteBuffers(1, vbo);
}

void Renderer_CreateTextVao(u32 *vao, u32 *vbo){

    glGenVertexArrays(1, vao);
    glBindVertexArray(*vao);
    glGenBuffers(1, vbo);
 
    glBindBuffer(GL_ARRAY_BUFFER,*vbo);
 
    u32 stride = sizeof(Vec4)+sizeof(Vec3);

    glEnableVertexAttribArray(POS_LOC);
    glVertexAttribPointer(POS_LOC, 2, GL_FLOAT, GL_FALSE, stride, 0);

    glEnableVertexAttribArray(UV_LOC);
    glVertexAttribPointer(UV_LOC, 2, GL_FLOAT, GL_FALSE, stride, (void*)sizeof(Vec2));

    glEnableVertexAttribArray(COLOR_LOC);
    glVertexAttribPointer(COLOR_LOC, 3, GL_FLOAT, GL_FALSE, stride, (void*)sizeof(Vec4));
 
    glBindVertexArray(0);
}

void Renderer_SetTextVaoBufferSize(u32 vao, u32 vbo, u32 len){
    
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER,vbo);
    glBufferData(GL_ARRAY_BUFFER, (sizeof(Vec4)+sizeof(Vec3)) * len * 6, NULL, GL_STATIC_DRAW);
    glBindVertexArray(0);
}

void Renderer_CalcTextDataLen(const TextLine_t *line, u32 *num){

    char *str = (char *)line->text;

    char p;
    while((p = *str++))
        if(p != 32 && p != '\t') ++*num;
}

void Renderer_InsertTextData(u32 vao, u32 vbo, TextLine_t *line, u32 *num){

    Vec3 color = (Vec3){line->r/255.0f, line->g/255.0f, line->b/255.0f};

    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);

    u32 spacing = line->spacing + line->size;

    float tx, ty;

    u32 offset = *num * (sizeof(Vec4)+sizeof(Vec3))*6;

    static const float table[] = {0.0000, 0.0625, 0.1250, 0.1875, 0.2500, 0.3125, 0.3750, 0.4375, 
                                    0.5000, 0.5625, 0.6250, 0.6875, 0.7500, 0.8125, 0.8750, 0.9375};

    char *str = (char *)line->text;

    u32 x = line->x;

    u32 p, k;
    while((p = *str++)){

        if(p == '\t'){
            x += spacing * TAB_SPACING;
            continue;         

        } else if(p == 32) {

            x += spacing;
            continue;         
        }

        tx = table[p % 16];
        ty = table[15 - (p >> 4)];

        for(k = 0; k < 6; k++){

            Vec4 uvpos;

            uvpos.x = (RectTriangleVerts[k*2]     * line->size)   + x;
            uvpos.y = (RectTriangleVerts[(k*2)+1] * line->size)   + line->y;
            uvpos.z = (RectTriangleVerts[k*2]     * 0.0625) + tx;
            uvpos.w = ((1-RectTriangleVerts[(k*2)+1]) * 0.0625) + ty;

            glBufferSubData(GL_ARRAY_BUFFER, offset, sizeof(Vec4), &uvpos.x);
            offset += sizeof(Vec4);
            glBufferSubData(GL_ARRAY_BUFFER, offset, sizeof(Vec3), &color);
            offset += sizeof(Vec3);
        }

        x += spacing;

        ++*num;
    }
}

void Renderer_RenderTextVao(Renderer_t *renderer, u32 vao, u32 vbo, u32 num){

    glUseProgram(renderer->shaders[SHADER_TEXT].program);

    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, renderer->fontTex);

    glDrawArrays(GL_TRIANGLES, 0, 6*num);
    glBindVertexArray(0);
}

void Renderer_RenderString(Renderer_t *renderer, TextLine_t *line){
    

#if defined(USE_INSTANCING) && USE_INSTANCING
    
    int len = strlen(line->text);

    glBindVertexArray(renderer->text.vao);
    glUseProgram(renderer->shaders[SHADER_TEXT].program);
    glUniform3f(renderer->shaders[SHADER_TEXT].color, line->r/255.0f, line->g/255.0f, line->b/255.0f);

    glBindTexture(GL_TEXTURE_1D, renderer->text.stringTex);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    glTexSubImage1D(GL_TEXTURE_1D, 0, 0, len, GL_RED, GL_UNSIGNED_BYTE, line->text);

    glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
    glBindTexture(GL_TEXTURE_1D, 0);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, renderer->fontTex);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_1D, renderer->text.stringTex);

    glUniform1i(renderer->shaders[SHADER_TEXT].text.length, len);
    glUniform1i(renderer->shaders[SHADER_TEXT].text.spacing, line->spacing);
    glUniform2f(renderer->shaders[SHADER_TEXT].text.start, line->x, line->y);
    glUniform1i(renderer->shaders[SHADER_TEXT].text.size, line->size);
    glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, len);

#else

    glUseProgram(renderer->shaders[SHADER_TEXT].program);
    glUniform3f(renderer->shaders[SHADER_TEXT].color, 255.0f, 255.0f, 255.0f);

    renderer->text.num = 0;
    Renderer_CalcTextDataLen((const TextLine_t *)line, &renderer->text.num);

    Renderer_SetTextVaoBufferSize(renderer->text.vao, renderer->text.vbo, renderer->text.num);


    renderer->text.num = 0;
    Renderer_InsertTextData(renderer->text.vao, renderer->text.vbo, line, &renderer->text.num);
    Renderer_RenderTextVao(renderer, renderer->text.vao, renderer->text.vbo, renderer->text.num);

    Renderer_SetTextVaoBufferSize(renderer->text.vao, renderer->text.vbo, 0);

#endif

    glBindVertexArray(0);
}

void Renderer_RenderParticle(Renderer_t *renderer, u32 x, u32 y, u8 size, u8 r, u8 g, u8 b){
    
    glBindVertexArray(renderer->particles.vao);
    glUseProgram(renderer->shaders[SHADER_TEXTURELESS_PARTICLES_2D].program);
    glUniform3f(renderer->shaders[SHADER_TEXTURELESS_PARTICLES_2D].color, r/255.0f, g/255.0f, b/255.0f);

    glBindBuffer(GL_ARRAY_BUFFER, renderer->particles.vbo);


    int k;
    for(k = 0; k < 6; k++){

        Vec3 vert = (Vec3){x + (RectTriangleVerts[(k*2)] * size), y + (RectTriangleVerts[(k*2)+1] * size), 0};
        Vec2 uv = (Vec2){RectTriangleVerts[(k*2)], RectTriangleVerts[(k*2)+1]};
        Vec4 color = (Vec4){1,1,1,1};

        u32 offset = sizeof(float)*9*k;
        glBufferSubData(GL_ARRAY_BUFFER, offset, sizeof(Vec3), &vert.x);

        offset += sizeof(Vec3);
        glBufferSubData(GL_ARRAY_BUFFER, offset, sizeof(Vec2), &uv.x);

        offset += sizeof(Vec2);
        glBufferSubData(GL_ARRAY_BUFFER, offset, sizeof(Vec4), &color.x);
    }

    glDrawArrays(GL_TRIANGLES, 0, 6);

    glBindVertexArray(0);
}

void Renderer_CreateImage(u32 *image, const u16 *data, u32 w, u32 h){

    glGenTextures(1, image);
    glBindTexture(GL_TEXTURE_2D, *image);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB5_A1, w, h, 0, GL_RGBA, GL_UNSIGNED_SHORT_5_5_5_1, data);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);

    glBindTexture(GL_TEXTURE_2D, 0);
}

void Renderer_DeleteImage(u32 *image){

    glDeleteTextures(1, image);
}

void Renderer_RenderTexturedRect(Renderer_t *renderer, u32 x, u32 y, u32 w, u32 h, 
    float tx, float ty, float tw, float th, u32 texture){
    
    glBindVertexArray(renderer->texturedRenderer.vao);
    glUseProgram(renderer->shaders[SHADER_TEXTURED_2D].program);
    // glUniform3f(renderer->shaders[SHADER_TEXTURED_2D].color, r/255.0f, g/255.0f, b/255.0f);

    glBindBuffer(GL_ARRAY_BUFFER, renderer->texturedRenderer.vbo);

    glBufferData(GL_ARRAY_BUFFER, sizeof(RectTriangleVerts)*2, NULL, GL_STATIC_DRAW);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture);

    int k;
    for(k = 0; k < 6; k++){

        Vec4 posuv;
        posuv.x = (RectTriangleVerts[(k*2)] * w) + x;
        posuv.y = (RectTriangleVerts[(k*2)+1] * h) + y;

        posuv.z = (RectTriangleVerts[(k*2)] * tw) + tx;
        posuv.w = (1-RectTriangleVerts[(k*2)+1] * th) + ty;

        u32 offset = sizeof(Vec4)*k;
        glBufferSubData(GL_ARRAY_BUFFER, offset, sizeof(Vec4), &posuv.x);
    }

    glDrawArrays(GL_TRIANGLES, 0, 6);

    glBindVertexArray(0);
}

void Renderer_DrawCircle(Renderer_t *renderer, u32 x, u32 y, u32 radius, u8 num, float rotation){

    Vec2 circleVerts[num+2];

    float radians = (360/(num))*(PI/180);
    Vec2 vert = (Vec2){ x, y };
    
    circleVerts[0] = vert;

    int k;
    float nx, ny;

    for(k = 0; k <= num; k++){
        float angle = rotation+(radians*k);

        if(k == num){
            ny = -sin(rotation) * radius;
            nx =  cos(rotation) * radius;
            nx+=x; ny+=y;
            vert.x = nx; vert.y = ny;
        } else {
            ny = -sin(angle) * radius;
            nx =  cos(angle) * radius;
            nx+=x; ny+=y;
            vert.x = nx; vert.y = ny;
        }

        circleVerts[k+1] = vert;
    }

    glUseProgram(renderer->shaders[SHADER_TEXTURELESS_2D].program);
    glBindVertexArray(renderer->texturelessRenderer.vao);
    glBindBuffer(GL_ARRAY_BUFFER, renderer->texturelessRenderer.vbo);
    glBufferData( GL_ARRAY_BUFFER, (num+2)*sizeof(Vec2), circleVerts, GL_STATIC_DRAW);
    glDrawArrays(GL_TRIANGLE_FAN,0,num+2);
    glBindVertexArray(0);
}

void Renderer_DrawStrokedCircle(Renderer_t *renderer, u32 x, u32 y, u32 radius, u8 num, float rotation, u8 stroke){

    float theta = (PI*2)/num;

    Vec2 verts[num*6];

    Vec2 pos = (Vec2){x, y};

    int k;
    for(k = 0; k < num; k++){

        Vec2 point1, point2, point3, point4;
        point1.y = -sin((k * theta) + rotation) * radius;
        point1.x = cos((k * theta) + rotation) * radius;

        point2.y = -sin(((k+1) * theta) + rotation) * radius;
        point2.x = cos(((k+1) * theta) + rotation) * radius;

        point3.y = -sin(((k+1) * theta) + rotation) * (radius - stroke);
        point3.x = cos(((k+1) * theta) + rotation) * (radius - stroke);

        point4.y = -sin((k * theta) + rotation) * (radius - stroke);
        point4.x = cos((k * theta) + rotation) * (radius - stroke);

        verts[(k*6) + 0] = VEC2V(point1, pos, +);
        verts[(k*6) + 1] = VEC2V(point2, pos, +);
        verts[(k*6) + 2] = VEC2V(point3, pos, +);
        verts[(k*6) + 3] = VEC2V(point3, pos, +);
        verts[(k*6) + 4] = VEC2V(point4, pos, +);
        verts[(k*6) + 5] = VEC2V(point1, pos, +);
    }

    glUseProgram(renderer->shaders[SHADER_TEXTURELESS_2D].program);
    glBindVertexArray(renderer->texturelessRenderer.vao);
    glBindBuffer(GL_ARRAY_BUFFER, renderer->texturelessRenderer.vbo);
    glBufferData( GL_ARRAY_BUFFER, num*6*sizeof(Vec2), verts, GL_STATIC_DRAW);
    glDrawArrays(GL_TRIANGLES,0,num*6);
    glBindVertexArray(0);
}



void Renderer_DrawPoints(Renderer_t *renderer, Vec2 *points, int nPoints){

    glUseProgram(renderer->shaders[SHADER_TEXTURELESS_2D].program);
    glBindVertexArray(renderer->texturelessRenderer.vao);
    glBindBuffer(GL_ARRAY_BUFFER, renderer->texturelessRenderer.vbo);

    glBufferData(GL_ARRAY_BUFFER, nPoints*sizeof(Vec2), points, GL_STATIC_DRAW);

    glDrawArrays(GL_POINTS, 0, nPoints);

    glBindVertexArray(0);
}

void Renderer_DrawLines(Renderer_t *renderer, Vec2 *lines, int nLines){

    glUseProgram(renderer->shaders[SHADER_TEXTURELESS_2D].program);
    glBindVertexArray(renderer->texturelessRenderer.vao);
    glBindBuffer(GL_ARRAY_BUFFER, renderer->texturelessRenderer.vbo);

    glBufferData(GL_ARRAY_BUFFER, nLines*sizeof(Vec2), lines, GL_STATIC_DRAW);

    glDrawArrays(GL_LINE_STRIP, 0, nLines);

    glBindVertexArray(0);
}

void Renderer_Milkdrop(Renderer_t *renderer){

    glBindVertexArray(renderer->milkdrop.grid.vao);
    glUseProgram(renderer->shaders[SHADER_MILKDROP].program);
    // glUniform3f(renderer->shaders[SHADER_MILKDROP].color, r/255.0f, g/255.0f, b/255.0f);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, renderer->milkdrop.buffers[0].textures[0]);

    // glDisable(GL_CULL_FACE);

    glUniform1f(renderer->shaders[SHADER_MILKDROP].milkdrop.time, SDL_GetTicks() / 1000.0f);

   glDrawElements(GL_TRIANGLES, renderer->milkdrop.grid.nElements, GL_UNSIGNED_SHORT, NULL);

    glBindVertexArray(0);

}