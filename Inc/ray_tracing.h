#ifndef RAY_TRACING_H
#define RAY_TRACING_H

#include <stdio.h>
#include <math.h>
#include "show.h"
#include "vector.h"

Vector_3 ray_direction(int i, int j, int w, int h);

#endif // RAY_TRACING_H