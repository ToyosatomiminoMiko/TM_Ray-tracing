#ifndef VEC3_H_
#define VEC3_H_

/*
三维向量 vec3
本课程所有向量运算都写成"纯函数":
只读参数/不修改全局状态/返回新值,方便教学与调试.
*/
typedef struct
{
    float x, y, z;
} vec3;

/* 加法: a + b = (ax+bx, ay+by, az+bz) */
vec3 vec3_add(vec3 a, vec3 b);

/* 减法: a - b = (ax-bx, ay-by, az-bz) */
vec3 vec3_sub(vec3 a, vec3 b);

/* 数乘: s * v = (s*vx, s*vy, s*vz) */
vec3 vec3_scale(vec3 v, float s);

/* 点积: a·b = ax*bx + ay*by + az*bz (结果是一个标量) */
float vec3_dot(vec3 a, vec3 b);

/* 叉积: a×b = (ay*bz-az*by, az*bx-ax*bz, ax*by-ay*bx) (结果是垂直于a/b的向量) */
vec3 vec3_cross(vec3 a, vec3 b);

/* 长度平方: |v|² = vx² + vy² + vz² (省一次开方,用于快速比较) */
float vec3_length_squared(vec3 v);

/* 长度: |v| = sqrt(vx² + vy² + vz²) */
float vec3_length(vec3 v);

/* 归一化: v̂ = v / |v|,让长度变成 1,零向量返回 (0,0,0) */
vec3 vec3_normalize(vec3 v);

/* 两点距离: |a - b| */
float vec3_distance(vec3 a, vec3 b);

/* 线性插值: a + t*(b-a),t=0 得到 a,t=1 得到 b */
vec3 vec3_lerp(vec3 a, vec3 b, float t);

/*
反射: r = d - 2*(d·n)*n
前提是法线 n 已经归一化,常用于镜面反射方向.
*/
vec3 vec3_reflect(vec3 d, vec3 n);

#endif /* VEC3_H_ */
