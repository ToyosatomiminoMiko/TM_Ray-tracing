#ifndef VEC3_H_
#define VEC3_H_

/*
三维向量 Vector_3
本课程所有向量运算都写成"纯函数":
只读参数、不修改全局状态、返回新值,方便教学与调试。
*/
typedef struct
{
    float x, y, z;
} Vector_3;

/* 加法: a + b = (ax+bx, ay+by, az+bz) */
Vector_3 vector3_add(Vector_3 a, Vector_3 b);

/* 减法: a - b = (ax-bx, ay-by, az-bz) */
Vector_3 vector3_sub(Vector_3 a, Vector_3 b);

/* 数乘: s * v = (s*vx, s*vy, s*vz) */
Vector_3 vector3_scale(Vector_3 v, float s);

/* 点积: a·b = ax*bx + ay*by + az*bz (结果是一个标量) */
float vector3_dot(Vector_3 a, Vector_3 b);

/* 叉积: a×b = (ay*bz-az*by, az*bx-ax*bz, ax*by-ay*bx) (结果是垂直于a、b的向量) */
Vector_3 vector3_cross(Vector_3 a, Vector_3 b);

/* 长度平方: |v|² = vx² + vy² + vz² (省一次开方,用于快速比较) */
float vector3_length_squared(Vector_3 v);

/* 长度: |v| = sqrt(vx² + vy² + vz²) */
float vector3_length(Vector_3 v);

/* 归一化: v̂ = v / |v|,让长度变成 1,零向量返回 (0,0,0) */
Vector_3 vector3_normalize(Vector_3 v);

/* 两点距离: |a - b| */
float vector3_distance(Vector_3 a, Vector_3 b);

/* 线性插值: a + t*(b-a),t=0 得到 a,t=1 得到 b */
Vector_3 vector3_lerp(Vector_3 a, Vector_3 b, float t);

/*
反射: r = d - 2*(d·n)*n
前提是法线 n 已经归一化,常用于镜面反射方向。
*/
Vector_3 vector3_reflect(Vector_3 d, Vector_3 n);

#endif /* VEC3_H_ */
