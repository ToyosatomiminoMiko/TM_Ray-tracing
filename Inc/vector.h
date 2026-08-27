#ifndef VECTOR_H_
#define VECTOR_H_

typedef struct
{
    float x, y, z;
} Vector_3;

Vector_3 vector3_normalize(Vector_3 v);

// 向量加减乘除工具(全都是逐分量操作)
Vector_3 vector3_add(Vector_3 a, Vector_3 b);
Vector_3 vector3_sub(Vector_3 a, Vector_3 b);
Vector_3 vector3_scale(Vector_3 v, float s);
float vector3_dot(Vector_3 a, Vector_3 b);

#endif // VECTOR_H_