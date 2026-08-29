#include "vec3.h"

#include <math.h>

/*
这里全是纯函数,不要破坏这个约定:
不修改入参/不使用全局变量/同样的输入永远得到同样的输出.
*/

vec3 vec3_add(vec3 a, vec3 b)
{
    return (vec3){a.x + b.x, a.y + b.y, a.z + b.z};
}

vec3 vec3_sub(vec3 a, vec3 b)
{
    return (vec3){a.x - b.x, a.y - b.y, a.z - b.z};
}

vec3 vec3_scale(vec3 v, float s)
{
    return (vec3){v.x * s, v.y * s, v.z * s};
}

float vec3_dot(vec3 a, vec3 b)
{
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

vec3 vec3_cross(vec3 a, vec3 b)
{
    return (vec3){
        a.y * b.z - a.z * b.y,
        a.z * b.x - a.x * b.z,
        a.x * b.y - a.y * b.x};
}

float vec3_length_squared(vec3 v)
{
    return v.x * v.x + v.y * v.y + v.z * v.z;
}

float vec3_length(vec3 v)
{
    return sqrtf(vec3_length_squared(v));
}

vec3 vec3_normalize(vec3 v)
{
    float len = vec3_length(v);
    if (len < 1e-8f) /* 长度小于这个阈值就当零向量处理 */
        return (vec3){0, 0, 0};
    float inv = 1.0f / len;
    return (vec3){v.x * inv, v.y * inv, v.z * inv};
}

float vec3_distance(vec3 a, vec3 b)
{
    return vec3_length(vec3_sub(a, b));
}

vec3 vec3_lerp(vec3 a, vec3 b, float t)
{
    return vec3_add(a, vec3_scale(vec3_sub(b, a), t));
}

vec3 vec3_reflect(vec3 d, vec3 n)
{
    /* r = d - 2*(d·n)*n */
    return vec3_sub(d, vec3_scale(n, 2.0f * vec3_dot(d, n)));
}
