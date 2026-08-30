#ifndef PLANE_H_
#define PLANE_H_

#include "ray.h"
#include <math.h>

/*
平面求交:
平面由法线 n 和平面上一点 P0 定义,平面方程 n·(X-P0)=0.
代入光线 X=O+t*D:
  t = ((P0-O)·n) / (D·n)
当 |D·n| 接近 0 时光线与平面平行,没有交点.
返回值:t(距离);未命中或交点在射线背后返回 NAN.
*/
float intersect_plane(Ray ray, vec3 normal, vec3 point);

#endif /* PLANE_H_ */
