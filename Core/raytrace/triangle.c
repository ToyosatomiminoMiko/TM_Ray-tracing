#include "triangle.h"

int ray_triangle_intersect(
    Ray ray, vec3 V0, vec3 V1, vec3 V2,
    float *t, float *u, float *v)
{
    vec3 edge1 = vec3_sub(V1, V0);
    vec3 edge2 = vec3_sub(V2, V0);
    vec3 h = vec3_cross(ray.dir, edge2);
    float a = vec3_dot(edge1, h);

    if (a > -1e-6f && a < 1e-6f)
        return 0; /* 平行或共面 */

    float inv_det = 1.0f / a;
    vec3 s = vec3_sub(ray.origin, V0);
    *u = inv_det * vec3_dot(s, h);
    if (*u < 0.0f || *u > 1.0f)
        return 0;

    vec3 q = vec3_cross(s, edge1);
    *v = inv_det * vec3_dot(ray.dir, q);
    if (*v < 0.0f || *u + *v > 1.0f)
        return 0;

    *t = inv_det * vec3_dot(edge2, q);
    if (*t > 1e-6f)
        return 1; /* 确保交点在光线前方 */
    return 0;
}

vec3 triangle_normal(vec3 V0, vec3 V1, vec3 V2)
{
    return vec3_normalize(vec3_cross(
        vec3_sub(V1, V0),
        vec3_sub(V2, V0)));
}
