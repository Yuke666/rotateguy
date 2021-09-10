#ifndef MATH_DEF
#define MATH_DEF

#include <math.h>
#include "types.h"

#define MAX_SHAPE_POINTS 16

#define PI 3.1415
#define SWAP(x, y, T) 			do { T SWAP = x; x = y; y = SWAP; } while(0)
#define MIN(x,y) 				((x)<(y)?(x):(y))
#define MAX(x,y) 				((x)>(y)?(x):(y))
#define CLAMP(v, min, max) 		( v > min ? (v > max ? max : v) : min )
#define SIGN(x) 				(x > 0 ? 1 : (x < 0 ? -1 : 0))

typedef struct {
	float 	x;
	float 	y;
} Vec2;

typedef struct {
	s32 x;
	s32 y;
} IVec2;

typedef struct {
	float 	x;
	float 	y;
	float 	z;
	float 	w;
} Vec4;

typedef struct {
	float 	x;
	float 	y;
	float 	w;
	float 	h;
} Rect2D;

#define SHAPE_STRUCTURE 	\
	u8 		nPoints; 			\
	float 	rotation;			\
	Vec2 	pos;				\
	Vec2 	points[MAX_SHAPE_POINTS];

typedef struct {
	SHAPE_STRUCTURE
} Shape_t;

void Math_Ortho(float *matrix, float l, float r, float t, float b, float n, float f);

int SATShapeShape(Shape_t *s1, Shape_t *s2, Vec2 *vec);
Rect2D ShapeAABB(Shape_t *shape);

static inline Vec2 Math_Vec2AddVec2(Vec2 v1, Vec2 v2){ return (Vec2){v1.x + v2.x, v1.y + v2.y }; }

static inline Vec2 Math_Vec2MultVec2(Vec2 v1, Vec2 v2){ return (Vec2){v1.x * v2.x, v1.y * v2.y }; }

static inline Vec2 Math_Vec2SubVec2(Vec2 v1, Vec2 v2){ return (Vec2){v1.x - v2.x, v1.y - v2.y }; }

static inline Vec2 Math_Vec2MultFloat(Vec2 v, float s){ return (Vec2){v.x * s, v.y * s  }; }

static inline Vec2 Math_Vec2DivideFloat(Vec2 v, float s){ s = 1.0f / s; return (Vec2){v.x * s, v.y * s  }; }

static inline float Math_Vec2Magnitude(Vec2 v) { float dot = v.x*v.x + v.y*v.y; return dot != 0 ? sqrt(dot) : 0; }

static inline float Math_Vec2Dot(Vec2 v1, Vec2 v2){ return (v1.x * v2.x) + (v1.y * v2.y); }

static inline Vec2 Math_Vec2Cross(Vec2 v1, Vec2 v) { return (Vec2){v1.y - v.y, v.x - v1.x }; }

static inline float Math_Lerp(float a1, float a2, float t){ return (a1 * (1.0 - t)) + (a2 * t); }

static inline Vec2 Math_Vec2Normalize(Vec2 v){ return Math_Vec2DivideFloat(v, Math_Vec2Magnitude(v)); }

static inline Vec2 Math_Vec2Reflect(Vec2 v, Vec2 l){
    return Math_Vec2SubVec2(Math_Vec2MultFloat(Math_Vec2MultFloat(l, 2), (Math_Vec2Dot(v, l) / Math_Vec2Dot(l, l))), v);
}

static inline char Math_CheckCollisionRect2D(Rect2D r1, Rect2D r){
    if(r1.x <= r.x + r.w  && r1.w  + r1.x >= r.x &&
       r1.y <= r.y + r.h && r1.h + r1.y >= r.y) return 1;

    return 0;
}

static inline u8 Math_CheckRect2DInside(Rect2D b1, Rect2D b2) {
    if(b1.x >= b2.x && b1.x + b1.w <= b2.x + b2.w && 
       b1.y >= b2.y && b1.y + b1.h <= b2.y + b2.h) return 1;

    return 0;
}


#endif