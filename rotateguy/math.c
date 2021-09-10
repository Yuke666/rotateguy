#include "math.h"

typedef struct {
	double 	overlap;
	Vec2  	axis;
} Overlap_t;

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

static int CheckAxis(Vec2 *s1, int s1Sides, Vec2 *s2, int s2Sides, Vec2 a, Overlap_t *overlap){

    double min[2] = { HUGE_VAL, HUGE_VAL };
    double max[2] = { -HUGE_VAL, -HUGE_VAL };

    int m;
    for(m = 0; m < s1Sides; m++){
        double proj = Math_Vec2Dot(a, s1[m]);
        if(proj < min[0]) min[0] = proj;
        if(proj > max[0]) max[0] = proj;
    }

    for(m = 0; m < s2Sides; m++){
        double proj = Math_Vec2Dot(a, s2[m]);
        if(proj < min[1]) min[1] = proj;
        if(proj > max[1]) max[1] = proj;
    }

    if(min[0] > max[1] || min[1] > max[0])
        return 1; 

    // if(!(min[0] > min[1] || max[0] < max[1]))
    //  overlap->inside = 1;

    double smallest = HUGE_VAL;
    double max0min1 = max[0] - min[1];
    double max1min0 = max[1] - min[0];
    double min0max1 = min[0] - max[1];


    if(min[0] < min[1]){

        if(max[0] < max[1])
            smallest = max0min1;
        else
            if(max0min1 < max1min0) smallest = max0min1; else smallest = -max1min0;
    
    } else {

        if(max[0] > max[1])
            smallest = min0max1;            
        else
            if(max0min1 < max1min0) smallest = max0min1; else smallest = -max1min0;
    }


    if(fabs(smallest) < overlap->overlap){

        overlap->overlap = fabs(smallest);

        if(smallest < 0)
            overlap->axis = (Vec2){ -a.x, -a.y };
        else
            overlap->axis  = a;
    }

    return 0;

}

int SATShapeShape(Shape_t *s1, Shape_t *s2, Vec2 *vec){

	Vec2 s1Points[MAX_SHAPE_POINTS];
	Vec2 s2Points[MAX_SHAPE_POINTS];

	float sintheta = sinf(s1->rotation);
	float costheta = cosf(s1->rotation);

	int k;
	for(k = 0; k < s1->nPoints; k++){
        s1Points[k].x = s1->pos.x + ( (s1->points[k].x * costheta) - (sintheta * s1->points[k].y) );
        s1Points[k].y = s1->pos.y + ( (s1->points[k].x * sintheta) + (costheta * s1->points[k].y) );
	}
	
	sintheta = sinf(s2->rotation);
	costheta = cosf(s2->rotation);

	for(k = 0; k < s2->nPoints; k++){
        s2Points[k].x = s2->pos.x + ( (s2->points[k].x * costheta) - (sintheta * s2->points[k].y) );
        s2Points[k].y = s2->pos.y + ( (s2->points[k].x * sintheta) + (costheta * s2->points[k].y) );
	}

	Vec2 normal;

    Overlap_t overlap;

    overlap.overlap = HUGE_VAL;

    for(k = 0; k < s1->nPoints; k++){

        normal.x = -(s1Points[k].y - s1Points[k == 0 ? s1->nPoints-1 : k-1].y);
        normal.y = (s1Points[k].x - s1Points[k == 0 ? s1->nPoints-1 : k-1].x);
		normal = Math_Vec2Normalize(normal);

        if(CheckAxis(s1Points, s1->nPoints, s2Points, s2->nPoints, normal, &overlap))
            return 0;
    }

    for(k = 0; k < s2->nPoints; k++){

		normal.x = -(s2Points[k].y - s2Points[k == 0 ? s2->nPoints-1 : k-1].y);
		normal.y = (s2Points[k].x - s2Points[k == 0 ? s2->nPoints-1 : k-1].x);
		normal = Math_Vec2Normalize(normal);
    
        if(CheckAxis(s1Points, s1->nPoints, s2Points, s2->nPoints, normal, &overlap))
            return 0;
    }

    *vec = Math_Vec2MultFloat(overlap.axis, overlap.overlap);

    return 1;
}

Rect2D ShapeAABB(Shape_t *shape){

    float sintheta = sinf(shape->rotation);
    float costheta = cosf(shape->rotation);

    float minX = HUGE_VAL, minY = HUGE_VAL, maxX = -HUGE_VAL, maxY = -HUGE_VAL;

    Vec2 point;

    int k;
    for(k = 0; k < shape->nPoints; k++){
        point.x = shape->pos.x + ( (shape->points[k].x * costheta) - (sintheta * shape->points[k].y) );
        point.y = shape->pos.y + ( (shape->points[k].x * sintheta) + (costheta * shape->points[k].y) );
        
        if(point.x < minX) minX = point.x;
        if(point.x > maxX) maxX = point.x;
        if(point.y < minY) minY = point.y;
        if(point.y > maxY) maxY = point.y;
    }

    return (Rect2D){minX, minY, maxX - minX, maxY - minY};
}