#include "plane.h"

float intersect_plane(Ray ray, vec3 normal, vec3 point)
{
    float denom = vec3_dot(ray.dir, normal);

    // 判断射线与平面平行(法线与方向垂直)
    if (denom > -1e-6f && denom < 1e-6f)
        return NAN;   // 平行,无交点

    float t = vec3_dot(vec3_sub(point, ray.origin), normal) / denom;

    // 如果 t < 0, 交点在射线起点背后, 视为无效
    if (t < 1e-6f)
        return NAN;

    return t;   // 有效交点距离
}
