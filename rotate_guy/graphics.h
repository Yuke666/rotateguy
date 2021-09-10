#ifndef GRAPHICS_DEF
#define GRAPHICS_DEF

#include "rotateguy.h"
#include "math.h"

void Graphics_Init(void);
void Graphics_Close(void);
void Graphics_Resize(void);
void Graphics_Render(void);
void Graphics_RenderString(char *str, u32 x, u32 y, u8 r, u8 g, u8 b);
void Graphics_RenderTileMap(u32 sx, u32 sy, const u8 *map, u16 mapW, u16 mapH);
void Graphics_RenderSprite(u16 x, u16 y, u8 w, u8 h, float rotation, s16 ox, s16 oy, u8 tx, u8 ty);
void Graphics_BeginRenderSprites(void);
void Graphics_EndRenderSprites(void);

#ifdef DEBUG
void Graphics_RenderCapsule(s16 x, s16 y, s16 radius, s16 segment, float rotation);
void Graphics_RenderCircle(s16 x, s16 y, s16 radius, u16 num, float rotation);
void Graphics_RenderRect(u16 x, u16 y, u8 w, u8 h, float rotation, s16 ox, s16 oy);
#endif

#endif