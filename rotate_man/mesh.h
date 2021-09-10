#ifndef MESH_DEF
#define MESH_DEF

#include "types.h"

typedef struct {
	u32 	tex;
	u32 	texflags;
	float 	color1[3];
	float 	color2[3];
} Material_t;

typedef struct {
	s32 		vao;
	s32 		vbo;
	s32 		ebo;
	Material_t 	material;
} Mesh_t;

void Mesh_Load(Mesh_t *mesh, const char *path);

#endif