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

#endif /* SPHERE_H_ */
