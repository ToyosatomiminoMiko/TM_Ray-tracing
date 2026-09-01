#ifndef SPHERE_H_
#define SPHERE_H_

#include "ray.h"

/* 球:球心 + 半径 */
typedef struct
{
    vec3 center;
    float radius;
} Sphere;

/*
球面求交(简单写法,直接套一元二次方程):
把光线 P(t)=O+t*D 代入球面方程 |P-C|²=R²:
  |D|²*t² + 2*D·(O-C)*t + |O-C|²-R² = 0
取判别式 Δ = b²-4ac,最近交点 t = (-b-sqrt(Δ))/(2a)
返回值:t(距离);没命中或交点在背后返回 -1.
*/
float intersect_sphere_simple(Ray ray, Sphere sphere);

/*
球面求交(性能优化版):
利用 D 已归一化(|D|²=1),a=1,b=2*(D·OC),
优化成 b_half = D·OC,判别式 Δ = b_half²-c,最近根 t = -b_half-sqrt(Δ).
返回值:t(距离);没命中或交点在背后返回 -1.
*/
float intersect_sphere_fast(Ray ray, Sphere sphere);

/* 球面法线:N = (P-C)/R,即球心指向命中点再归一化 */
vec3 sphere_normal_at(Sphere sphere, vec3 point);

/*
球面贴图 UV(等距圆柱投影,对应 world.ppm 这类 2:1 世界地图):
把命中点 P 换成单位法线 n = (P-C)/R,再映射经纬度.
- 纬度 φ = asin(n.y),贴图 v = 0.5 - φ/π,v=0 是北极(图片第一行);
- 经度 λ = atan2(n.x, n.z):本初子午线朝 +Z(相机方向),东经朝 +X,
  u = 0.5 + λ/(2π),所以 u=0.5 是本初子午线,右半边是东半球.
u/v 都落在 [0,1),通过 out 参数返回.
*/
void sphere_uv_at(Sphere sphere, vec3 point, float *out_u, float *out_v);

#endif /* SPHERE_H_ */
