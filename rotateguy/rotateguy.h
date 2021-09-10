#ifndef DEFINES_DEF
#define DEFINES_DEF

#define GAME_TITLE "ROTATEGUY"

#define WIDTH 					320
#define HEIGHT 					240
#define GAME_DEFAULT_WIDTH 		800
#define GAME_DEFAULT_HEIGHT 	600
#define FALSE 					0
#define TRUE 					1
#define ERROR 					FALSE
#define SUCCESS 				TRUE

#define TILE_SIZE_BITS 4
#define TILE_SIZE_MASK 0xF
#define TILE_SIZE 16

#define TILES_X ((WIDTH >> TILE_SIZE_BITS)+2)
#define TILES_Y ((HEIGHT >> TILE_SIZE_BITS)+2)

#include <stdint.h>

typedef uint32_t u32;
typedef uint16_t u16;
typedef uint8_t u8;
typedef int32_t s32;
typedef int16_t s16;
typedef int8_t s8;

#endif