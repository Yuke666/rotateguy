#ifndef DEFINES_DEF
#define DEFINES_DEF

#include <stdint.h>

#ifndef DEBUG
#define DEBUG 1
#endif

#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif

#define WIDTH 480
#define HEIGHT 272

#define TILE_SIZE_BITS 4
#define TILE_SIZE_MASK 0xF
#define TILE_SIZE 16

#define TILES_X ((WIDTH >> TILE_SIZE_BITS)+2)
#define TILES_Y ((HEIGHT >> TILE_SIZE_BITS)+2)

#define LEVEL_HEIGHT 16
#define LEVEL_WIDTH 1024

#define VSYNC 1
#define UNCAPPED 0

#ifndef NULL
#define NULL 0
#endif

#define ROTATE_WORD(w) ((((w) >> 8) & 0xFF) | ((w) << 8))

#define ROTATE_INT(i) ((((i) & 0xff000000) >> 24) | (((i) & 0x00ff0000) >>  8) | \
					  (((i) & 0x0000ff00) <<  8) | (((i) & 0x000000ff) << 24))

typedef uint64_t u64;
typedef uint32_t u32;
typedef uint16_t u16;
typedef uint8_t u8;
typedef int64_t s64;
typedef int32_t s32;
typedef int16_t s16;
typedef int8_t s8;


typedef union {
	u32 	ival;
	float 	fval;
	char	*sval;
	void	*pval;
} EventArg_t;

enum {
	EVENT_NONE = 0,
	EVENT_ROTATECW,
	EVENT_ROTATECCW,
	EVENT_STOPROTATING,
	EVENT_JUMP,
	EVENT_FIRE,
};

extern const s8 Sinus[];

#endif