#include "vector.h"
#include <math.h>
/*
这里都是纯函数,不要破坏
*/

// 向量归一化 把长度变成 1
Vector_3 vector3_normalize(Vector_3 v)
{
    float len = sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
    if (len < 1e-8f) // 长度小于这个阈值就当零向量处理
        return (Vector_3){0, 0, 0};
    float inv = 1.0f / len;
    return (Vector_3){v.x * inv, v.y * inv, v.z * inv};
}

Vector_3 vector3_add(Vector_3 a, Vector_3 b) { return (Vector_3){a.x + b.x, a.y + b.y, a.z + b.z}; }
Vector_3 vector3_sub(Vector_3 a, Vector_3 b) { return (Vector_3){a.x - b.x, a.y - b.y, a.z - b.z}; }
Vector_3 vector3_scale(Vector_3 v, float s) { return (Vector_3){v.x * s, v.y * s, v.z * s}; }
float vector3_dot(Vector_3 a, Vector_3 b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
