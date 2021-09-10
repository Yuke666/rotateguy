#include <malloc.h>
#include <assert.h>
#include "memory.h"

#define MEMORY_DEBUG 1

#define ALIGN_UP(T, offset) (T)(((uintptr_t)offset + MEMORY_ALIGNMENT - 1) & ~(MEMORY_ALIGNMENT-1))
#define METADATA_SIZE (ALIGN_UP(u32, sizeof(u32)))

static void *memory;
static void *ends[2];
static void *caps[2];

int Memory_Init(void){

	u32 size = MEMORY_ALLOCATED;

	size = ALIGN_UP(u32, size) - MEMORY_ALIGNMENT;
	
	memory = malloc(size + MEMORY_ALIGNMENT);

	if(!memory) return MEM_ERR;

	caps[STACK_BOTTOM] = ALIGN_UP(void *, memory);
	caps[STACK_TOP] = ALIGN_UP(void *, memory + size);

	ends[STACK_BOTTOM] = caps[STACK_BOTTOM];
	ends[STACK_TOP] = caps[STACK_TOP];

	return MEM_OK;
}

void Memory_Close(void){

	free(memory);
}

void Memory_Pop(u8 end, u16 num){

	u32 k;
	
	if(end){

#ifdef MEMORY_DEBUG
		assert(ends[STACK_TOP] != caps[STACK_TOP]);
#endif	
		for(k = 0; k < num; k++)
			if(ends[STACK_TOP] < caps[STACK_TOP])
				ends[STACK_TOP] += *((u32 *)ends[STACK_TOP]);

	} else {

#ifdef MEMORY_DEBUG
		assert(ends[STACK_BOTTOM] != caps[STACK_BOTTOM]);
#endif	

		for(k = 0; k < num; k++)
			if(ends[STACK_BOTTOM] > caps[STACK_BOTTOM])
				ends[STACK_BOTTOM] -= *((u32 *)(ends[STACK_BOTTOM] - METADATA_SIZE));
	}

}

void *Memory_Alloc(u8 end, u32 size){

	size = ALIGN_UP(u32, size) + METADATA_SIZE;

#ifdef MEMORY_DEBUG
	assert((uintptr_t)ends[STACK_BOTTOM] + size < (uintptr_t)ends[STACK_TOP]);
	assert((uintptr_t)ends[STACK_TOP] - size > (uintptr_t)ends[STACK_BOTTOM]);
#endif

	void *mem = NULL;

	if(end){

		ends[STACK_TOP] -= size;
		mem = ends[STACK_TOP] + METADATA_SIZE;
		*((u32 *)ends[STACK_TOP]) = size;

	} else {
		
		mem = ends[STACK_BOTTOM];
		ends[STACK_BOTTOM] += size - METADATA_SIZE;
		*((u32 *)ends[STACK_BOTTOM]) = size;
		ends[STACK_BOTTOM] += METADATA_SIZE;
	}

	return mem;
}