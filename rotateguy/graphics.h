#ifndef GRAPHICS_DEF
#define GRAPHICS_DEF

#include "types.h"
#include "image_loader.h"
#include "math.h"

enum {
	POS_LOC = 0,
	UV_LOC,
};

typedef struct {
	u32 	program;
	u32 	fShader;
	u32 	vShader;
	u32		uniColorLoc;
	u32 	camPosLoc;
	u32 	zoomLoc;
	u32 	abberationLoc;
} Shader_t;

typedef struct {
    u32 	x;
    u32 	y;
    u16 	u;
    u16 	v;
} PosUV_t;

enum {
	TEXTURELESS_SHADER=0,
	TEXTURED_SHADER,
	QUAD_SHADER,
	NUM_SHADERS
};

void Graphics_Init(void);
void Graphics_Close(void);
void Graphics_Resize(int w, int h);
void Graphics_Render(void);
void Graphics_SetAbberation(u8 set);
void Graphics_RenderString(char *str, u32 x, u32 y, u8 r, u8 g, u8 b);
void Graphics_RenderTileMap(u32 sx, u32 sy, const u8 *map, u16 mapW, u16 mapH, Texture_t tex);
void Graphics_RenderRotatedSprite(float x, float y, u16 w, u16 h, float rotation, s16 ox, s16 oy, u16 tx, u16 ty, Texture_t tex);
void Graphics_RenderRect(float x, float y, u16 w, u16 h, u8 r, u8 g, u8 b, u8 a);
void Graphics_RenderRectLines(float x, float y, u16 w, u16 h, u8 r, u8 g, u8 b, u8 a);
void Graphics_UseShader(int shader);
void Graphics_SetCameraPos(Vec2 camPos);
void Graphics_RenderRotatedRect(float x, float y, u16 w, u16 h, float rotation, s16 ox, s16 oy);
void Graphics_SetCameraZoom(Vec2 zoom);
void Graphics_RenderTile(float x, float y, u16 w, u16 h, u8 tile, Texture_t tex, u8 r, u8 g, u8 b, u8 a);
void Graphics_RenderSprite(float x, float y, u16 w, u16 h, u16 tx, u16 ty, u16 tw, u16 th, Texture_t tex);

#endif