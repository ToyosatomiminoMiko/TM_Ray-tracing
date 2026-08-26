#ifndef RAY_TRACING_H
#define RAY_TRACING_H

#include <stdio.h>
#include <math.h>
#include "show.h"

typedef struct
{
    float x, y, z;
} Vector_3;

Vector_3 vector3_normalize(Vector_3 v);
Vector_3 ray_direction(int i, int j);

#endif // RAY_TRACING_H