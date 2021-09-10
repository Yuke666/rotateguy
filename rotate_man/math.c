#include "math.h"

Vec3 LerpVec3(Vec3 a1, Vec3 a2, float t){
	if(t > 1) return a2;
	if(t < 0) return a1;
	return VEC3V(VEC3F(a1,1.0-t,*), VEC3F(a2, t,*),+);
}

void Math_Ortho(float *matrix, float l, float r, float t, float b, float n, float f){
	matrix[0] = 2/(r-l);
	matrix[1] = 0;
	matrix[2] = 0;
	matrix[3] = -((r+l)/(r-l));
	matrix[4] = 0;
	matrix[5] = 2/(t-b);
	matrix[6] = 0;
	matrix[7] = -((t+b)/(t-b));
	matrix[8] = 0;
	matrix[9] = 0;
	matrix[10] = 1/(f-n);
	matrix[11] = n/(f-n);
	matrix[12] = 0;
	matrix[13] = 0;
	matrix[14] = 0;
	matrix[15] = 1;
}