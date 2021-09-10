#include "object.h"

void Object_ApplyForce(Object_t *obj, Vec2 force, Vec2 point){

	obj->current->externalForce = Math_Vec2AddVec2(obj->current->externalForce, force);
	
	obj->current->externalTorque = Math_Vec2AddVec2(obj->current->externalTorque,
		Math_Vec2Cross(Math_Vec2SubVec2(point, obj->current->worldOrigin), force));
}