#ifndef MATH_DEF
#define MATH_DEF

#include <math.h>

#define PI 3.1415
#define SWAP(x, y, T) 			do { T SWAP = x; x = y; y = SWAP; } while(0)
#define MIN(x,y) 				((x)<(y)?(x):(y))
#define MAX(x,y) 				((x)>(y)?(x):(y))
#define CLAMP(v, min, max) 		( v > min ? (v > max ? max : v) : min )
#define SIGN(x) 				(x > 0 ? 1 : (x < 0 ? -1 : 0))

#define VEC3F(v, f, op) 		((Vec3){(v).x op f, (v).y op f, (v).z op f})
#define VEC3V(v1, v2, op) 		((Vec3){(v1).x op (v2).x, (v1).y op (v2).y, (v1).z op (v2).z})
#define VEC3MAG(v) 				(sqrtf((v).x*(v).x + (v).y*(v).y + (v).z*(v).z))
#define VEC3NORM(v) 			(VEC3F(v, VEC3MAG(v), /))
#define VEC3DOT(v1, v2) 		(((v1).x * v2.x) + ((v1).y * v2.y) + ((v1).z * v2.z))
#define VEC3CROSS(v1, v2) 		((Vec3){((v1).y * (v2).z) - ((v1).z * (v2).y), \
								((v1).z * (v2).x) - ((v1).x * (v2).z), ((v1).x * (v2).y) - ((v1).y * (v2).x) })

#define VEC2F(v, f, op) 		((Vec2){(v).x op f, (v).y op f})
#define VEC2V(v1, v2, op) 		((Vec2){(v1).x op (v2).x, (v1).y op (v2).y})
#define VEC2MAG(v) 				(sqrtf((v).x*(v).x + (v).y*(v).y))
#define VEC2NORM(v) 			(VEC2F(v, VEC2MAG(v), /))
#define VEC2DOT(v1, v2) 		(((v1).x * v2.x) + ((v1).y * v2.y))

typedef struct {
	float 	x;
	float 	y;
	float 	z;
} Vec3;

typedef struct {
	float 	x;
	float 	y;
} Vec2;

typedef struct {
	float 	x;
	float 	y;
	float 	z;
	float 	w;
} Vec4;

Vec3 LerpVec3(Vec3 a1, Vec3 a2, float t);
void Math_Ortho(float *matrix, float l, float r, float t, float b, float n, float f);

#endif