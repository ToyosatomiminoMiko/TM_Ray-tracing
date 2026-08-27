#include "vec3.h"

#include <math.h>

/*
这里全是纯函数,不要破坏这个约定:
不修改入参/不使用全局变量/同样的输入永远得到同样的输出.
*/

Vector_3 vector3_add(Vector_3 a, Vector_3 b)
{
    return (Vector_3){a.x + b.x, a.y + b.y, a.z + b.z};
}

Vector_3 vector3_sub(Vector_3 a, Vector_3 b)
{
    return (Vector_3){a.x - b.x, a.y - b.y, a.z - b.z};
}

Vector_3 vector3_scale(Vector_3 v, float s)
{
    return (Vector_3){v.x * s, v.y * s, v.z * s};
}

float vector3_dot(Vector_3 a, Vector_3 b)
{
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

Vector_3 vector3_cross(Vector_3 a, Vector_3 b)
{
    return (Vector_3){
        a.y * b.z - a.z * b.y,
        a.z * b.x - a.x * b.z,
        a.x * b.y - a.y * b.x};
}

float vector3_length_squared(Vector_3 v)
{
    return v.x * v.x + v.y * v.y + v.z * v.z;
}

float vector3_length(Vector_3 v)
{
    return sqrtf(vector3_length_squared(v));
}

Vector_3 vector3_normalize(Vector_3 v)
{
    float len = vector3_length(v);
    if (len < 1e-8f) /* 长度小于这个阈值就当零向量处理 */
        return (Vector_3){0, 0, 0};
    float inv = 1.0f / len;
    return (Vector_3){v.x * inv, v.y * inv, v.z * inv};
}

float vector3_distance(Vector_3 a, Vector_3 b)
{
    return vector3_length(vector3_sub(a, b));
}

Vector_3 vector3_lerp(Vector_3 a, Vector_3 b, float t)
{
    return vector3_add(a, vector3_scale(vector3_sub(b, a), t));
}

Vector_3 vector3_reflect(Vector_3 d, Vector_3 n)
{
    /* r = d - 2*(d·n)*n */
    return vector3_sub(d, vector3_scale(n, 2.0f * vector3_dot(d, n)));
}
