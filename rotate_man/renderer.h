#ifndef RENDERER_DEF
#define RENDERER_DEF

#include "types.h"
#include "math.h"

#define USE_INSTANCING FALSE
#define GAME_NEAR 0
#define GAME_FAR 10
#define RENDERER_MAX_STRING_CHARS 512
#define TAB_SPACING 4
#define RENDERER_MAX_PARTICLES 512
#define MAX_FB_TEXTURES 3

enum {
    POS_LOC = 0,
    UV_LOC,
    COLOR_LOC,
    BONE_INDICES_LOC,
    NORM_LOC,
	WEIGHTS_LOC,
    TANGENT_LOC,
};

enum {
	SHADER_TEXTURED_2D,
	SHADER_TEXTURELESS_2D,
	SHADER_TEXTURELESS_PARTICLES_2D,
	SHADER_TEXTURED_PARTICLES_2D,
	SHADER_TEXT,
	SHADER_MILKDROP,
	NUM_SHADERS,
};

typedef struct {
	u32 vao;
	u32 vbo;
	u32 ebo;
	u32 nElements;
} Grid_t;

typedef struct {
	
	u32 program;
	u32 fShader;
	u32 vShader;

	u32 view;
	u32 proj;
	u32 color;
	
	union {

		struct {
			u32 fontTex;
#if defined(USE_INSTANCING) && USE_INSTANCING
			u32 stringTex;
			u32 length;
			u32 spacing;
			u32 size;
			u32 start;
#endif
		} text;

		struct {
			u32 time;
			u32 fps;
			u32 frame;
			u32 progress;
			u32 bass;
			u32 mid;
			u32 treb;
			u32 bass_att;
			u32 mid_att;
			u32 treb_att;
			u32 meshx;
			u32 meshy;
			u32 pixelsx;
			u32 pixelsy;
			u32 aspectY;
			u32 aspectX;
			u32 zoom;
			u32 zoomexp;
			u32 rot;
			u32 warp;
			u32 cx;
			u32 cy;
			u32 dx;
			u32 dy;
			u32 sx;
			u32 sy;
		} milkdrop;
	};

} Shader_t;

typedef struct {
	u32 vao;
	u32 vbo;
	u32 num;
} TextRenderer_t;

typedef struct {
	u32 vao;
	u32 vbo;
} ParticleRenderer_t;

typedef struct {
	u32 vao;
	u32 vbo;
} VaoVbo_t;

typedef struct {
	u32 width;
	u32 height;
	u32 nTextures;
	u32 textures[MAX_FB_TEXTURES];
	u32 fb;
} ColorBuffer_t;

typedef struct {
	ColorBuffer_t 		buffers[2];
	Grid_t				grid;
} MilkdropRenderer_t;

typedef struct {
	u32 				width;
	u32 				height;
	float 				aspectX;
	float 				aspectY;
	Shader_t 			shaders[NUM_SHADERS];
	TextRenderer_t		text;
	ParticleRenderer_t	particles;
	VaoVbo_t 			texturedRenderer;
	VaoVbo_t 			texturelessRenderer;
	u32 				fontTex;
#if defined(USE_INSTANCING) && USE_INSTANCING
	u32 				stringTex;
#endif
	MilkdropRenderer_t	milkdrop;
} Renderer_t;

typedef struct {
	u32 			x;
	u32 			y;
	u32 			spacing;
	u32 			size;
	u32 			r;
	u32 			g;
	u32 			b;
	const char 		*text;
} TextLine_t;


void Renderer_Init(Renderer_t *renderer, u32 w, u32 h);
void Renderer_Close(Renderer_t *renderer);
void Renderer_RenderParticle(Renderer_t *renderer, u32 x, u32 y, u8 size, u8 r, u8 g, u8 b);

// text
void Renderer_RenderString(Renderer_t *renderer, TextLine_t *line);
void Renderer_CreateTextVao(u32 *vao, u32 *vbo);
void Renderer_SetTextVaoBufferSize(u32 vao, u32 vbo, u32 len);
void Renderer_InsertTextData(u32 vao, u32 vbo, TextLine_t *line, u32 *num);
void Renderer_RenderTextVao(Renderer_t *renderer, u32 vao, u32 vbo, u32 num);
void Renderer_DestroyTextVao(u32 *vao, u32 *vbo);
void Renderer_CalcTextDataLen(const TextLine_t *line, u32 *num);

// images
void Renderer_CreateImage(u32 *image, const u16 *data, u32 w, u32 h);
void Renderer_DeleteImage(u32 *image);
void Renderer_RenderTexturedRect(Renderer_t *renderer, u32 x, u32 y, u32 w, u32 h, float tx, float ty, float tw, float th, u32 texture);

// framebuffers
void Renderer_ResizeColorBuffer(ColorBuffer_t *cBuffer, u32 w, u32 h, u32 formats[], u32 types[]);
void Renderer_DestroyColorBuffer(ColorBuffer_t *cBuffer);
void Renderer_CreateColorBuffer(u32 w, u32 h, u32 num, u32 formats[], u32 types[], ColorBuffer_t *cBuffer);
void Renderer_BindColorBuffer(u32 mode, ColorBuffer_t cBuffer);

// circles
void Renderer_DrawCircle(Renderer_t *renderer, u32 x, u32 y, u32 radius, u8 num, float rotation);
void Renderer_DrawStrokedCircle(Renderer_t *renderer, u32 x, u32 y, u32 radius, u8 num, float rotation, u8 stroke);

// points
void Renderer_DrawPoints(Renderer_t *renderer, Vec2 *points, int nPoints);

// lines
void Renderer_DrawLines(Renderer_t *renderer, Vec2 *lines, int nLines);

//milkdrop
void Renderer_Milkdrop(Renderer_t *renderer);

#endif