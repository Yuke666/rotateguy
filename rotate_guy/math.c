#include "math.h"

typedef struct {
	float 	overlap;
	Vec2  	axis;
} Overlap_t;

static int CheckAxis(Vec2 *s1, int s1Sides, Vec2 *s2, int s2Sides, Vec2 a, Overlap_t *overlap){

    float min[2] = { HUGE_VAL, HUGE_VAL };
    float max[2] = { -HUGE_VAL, -HUGE_VAL };

    int m;
    for(m = 0; m < s1Sides; m++){
        float proj = VEC2DOT(a, s1[m]);
        if(proj < min[0]) min[0] = proj;
        if(proj > max[0]) max[0] = proj;
    }

    for(m = 0; m < s2Sides; m++){
        float proj = VEC2DOT(a, s2[m]);
        if(proj < min[1]) min[1] = proj;
        if(proj > max[1]) max[1] = proj;
    }

    if(min[0] > max[1] || min[1] > max[0])
		return 1; 

    // if(!(min[0] > min[1] || max[0] < max[1]))
    // 	overlap->inside = 1;

    float smallest;
    float max0min1 = max[0] - min[1];
    float max1min0 = max[1] - min[0];
    float min0max1 = min[0] - max[1];

    if(min[0] < min[1])
        if(max[0] < max[1])
			smallest = max0min1;
        else
            if(max0min1 < max1min0) smallest = max0min1; else smallest = -max1min0;
    else
        if(max[0] > max[1])
			smallest = min0max1;            
        else
            if(max0min1 < max1min0) smallest = max0min1; else smallest = -max1min0;

    if(fabs(smallest) < overlap->overlap ) {

        overlap->overlap = fabs(smallest);

        if(smallest < 0)
            overlap->axis = (Vec2){ -a.x, -a.y };
        else
	        overlap->axis  = a;
    }

    return 0;

}

int SATShapeShape(ConvexShape_t *s1, ConvexShape_t *s2, Vec2 *vec){

	Vec2 s1Points[MAX_SHAPE_POINTS];
	Vec2 s2Points[MAX_SHAPE_POINTS];

	float sintheta1 = sinf(s1->rotation);
	float costheta1 = cosf(s1->rotation);

	int k;
	for(k = 0; k < s1->nPoints; k++){
        s1Points[k].x = s1->pos.x + ( (s1->points[k].x * costheta1) - (sintheta1 * s1->points[k].y) );
        s1Points[k].y = s1->pos.y + ( (s1->points[k].x * sintheta1) + (costheta1 * s1->points[k].y) );
	}
	
	float sintheta2 = sinf(s2->rotation);
	float costheta2 = cosf(s2->rotation);

	for(k = 0; k < s2->nPoints; k++){
        s2Points[k].x = s2->pos.x + ( (s2->points[k].x * costheta2) - (sintheta2 * s2->points[k].y) );
        s2Points[k].y = s2->pos.y + ( (s2->points[k].x * sintheta2) + (costheta2 * s2->points[k].y) );
	}

	Vec2 normal;

    Overlap_t overlap;

    overlap.overlap = HUGE_VAL;

	for(k = 0; k < s1->nNormals; k++){

        normal.x = ( (s1->normals[k].x * costheta1) - (sintheta1 * s1->normals[k].y) );
        normal.y = ( (s1->normals[k].x * sintheta1) + (costheta1 * s1->normals[k].y) );

        if(CheckAxis(s1Points, s1->nPoints, s2Points, s2->nPoints, normal, &overlap))
        	return 0;
	}

	for(k = 0; k < s2->nNormals; k++){

        normal.x = ( (s2->normals[k].x * costheta2) - (sintheta2 * s2->normals[k].y) );
        normal.y = ( (s2->normals[k].x * sintheta2) + (costheta2 * s2->normals[k].y) );

        if(CheckAxis(s1Points, s1->nPoints, s2Points, s2->nPoints, normal, &overlap))
        	return 0;
	}

	*vec = VEC2F(overlap.axis, overlap.overlap, *);

    return 1;
}