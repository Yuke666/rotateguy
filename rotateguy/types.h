#ifndef DEFINES_DEF
#define DEFINES_DEF

#define GAME_TITLE "ROTATEGUY"

#define ROTATE_WORD(w) ((((w) >> 8) & 0xFF) | ((w) << 8))

#define ROTATE_INT(i) ((((i) & 0xff000000) >> 24) | (((i) & 0x00ff0000) >>  8) | \
					  (((i) & 0x0000ff00) <<  8) | (((i) & 0x000000ff) << 24))

#define WIDTH 					640
#define HEIGHT 					480
#define GAME_DEFAULT_WIDTH 		800
#define GAME_DEFAULT_HEIGHT 	600
#define FALSE 					0
#define TRUE 					1
#define ERROR 					FALSE
#define SUCCESS 				TRUE

#ifndef NULL
#define NULL 					0
#endif

#define FPS_CAP 				1

#define TILE_SIZE_BITS 4
#define TILE_SIZE_MASK 0xF
#define TILE_SIZE 16

#define TILES_X ((WIDTH >> TILE_SIZE_BITS)+2)
#define TILES_Y ((HEIGHT >> TILE_SIZE_BITS)+2)

#define LEVEL_WIDTH 1000
#define LEVEL_HEIGHT 30

#include <stdint.h>

typedef uint64_t u64;
typedef uint32_t u32;
typedef uint16_t u16;
typedef uint8_t u8;
typedef int64_t s64;
typedef int32_t s32;
typedef int16_t s16;
typedef int8_t s8;

#endif