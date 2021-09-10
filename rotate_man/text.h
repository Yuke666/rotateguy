#ifndef TEXT_DEF
#define TEXT_DEF

#include "types.h"

void Text_Init(void);
void Text_Draw(u32 x, u32 y, u32 hSpacing, u32 vSpacing, u32 maxWidth, const char *text);
void Text_Close(void);

#endif