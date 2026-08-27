#include "triangle.h"

int ray_triangle_intersect(
    Ray ray, Vector_3 V0, Vector_3 V1, Vector_3 V2,
    float *t, float *u, float *v)
{
    Vector_3 edge1 = vector3_sub(V1, V0);
    Vector_3 edge2 = vector3_sub(V2, V0);
    Vector_3 h = vector3_cross(ray.dir, edge2);
    float a = vector3_dot(edge1, h);

    if (a > -1e-6f && a < 1e-6f)
        return 0; /* 平行或共面 */

    float inv_det = 1.0f / a;
    Vector_3 s = vector3_sub(ray.origin, V0);
    *u = inv_det * vector3_dot(s, h);
    if (*u < 0.0f || *u > 1.0f)
        return 0;

    Vector_3 q = vector3_cross(s, edge1);
    *v = inv_det * vector3_dot(ray.dir, q);
    if (*v < 0.0f || *u + *v > 1.0f)
        return 0;

    *t = inv_det * vector3_dot(edge2, q);
    if (*t > 1e-6f)
        return 1; /* 确保交点在光线前方 */
    return 0;
}

Vector_3 triangle_normal(Vector_3 V0, Vector_3 V1, Vector_3 V2)
{
    return vector3_normalize(vector3_cross(
        vector3_sub(V1, V0),
        vector3_sub(V2, V0)));
}
