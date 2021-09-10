#ifndef MEMORY_DEF
#define MEMORY_DEF

#include "types.h"

#define MEMORY_ALIGNMENT 16
#define MEMORY_ALLOCATED (0x01 << 20) * 128

#define STACK_BOTTOM 	0
#define STACK_TOP 		1

int Memory_Init				(void);
void Memory_Close			(void);
void Memory_Pop				(u8 end, u16 num);
void *Memory_Alloc 			(u8 end, u32 size);


#endif