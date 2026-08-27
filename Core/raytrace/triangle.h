#ifndef TRIANGLE_H_
#define TRIANGLE_H_

#include "ray.h"

/*
三角形与光线求交(Möller–Trumbore 算法):
先用叉积求三角形法线方向的交点,再用重心坐标 (u,v) 判断是否在三角形内:
  edge1 = V1-V0, edge2 = V2-V0
  h = D × edge2
  a = edge1 · h (行列式,接近 0 表示平行/共面)
  u = (s · h) / a,  v = (D · q) / a,  t = (edge2 · q) / a
返回 1 相交,0 不相交;t 为距离,u,v 为重心坐标.
*/
int ray_triangle_intersect(
    Ray ray, Vector_3 V0, Vector_3 V1, Vector_3 V2,
    float *t, float *u, float *v);

/* 三角形平面法线(叉积后归一化) */
Vector_3 triangle_normal(Vector_3 V0, Vector_3 V1, Vector_3 V2);

#endif /* TRIANGLE_H_ */
