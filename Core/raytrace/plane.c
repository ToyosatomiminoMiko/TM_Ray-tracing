#include "plane.h"

int intersect_plane(Ray ray, Vector_3 normal, Vector_3 point, float *t)
{
    float denom = vector3_dot(ray.dir, normal);
    if (denom > -1e-6f && denom < 1e-6f)
        return 0; /* 光线与平面平行 */

    float result = vector3_dot(vector3_sub(point, ray.origin), normal) / denom;
    if (result < 1e-6f)
        return 0; /* 交点在光线背后 */
    *t = result;
    return 1;
}
