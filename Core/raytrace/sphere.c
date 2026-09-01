#include "sphere.h"

#include <math.h>

float intersect_sphere_simple(Ray ray, Sphere sphere)
{
    /* 1. 计算从球心指向光线起点的向量(注意方向) */
    vec3 oc = vec3_sub(ray.origin, sphere.center);
    /* 2. 一元二次方程的三个系数 */
    float a = vec3_dot(ray.dir, ray.dir);
    float b = 2.0f * vec3_dot(ray.dir, oc);
    float c = vec3_dot(oc, oc) - sphere.radius * sphere.radius;
    /* 3. 判别式 Δ = b²-4ac,小于 0 说明光线擦肩而过 */
    float delta = b * b - 4.0f * a * c;
    if (delta < 0)
        return -1;
    /* 4. 取最近的根(减号对应最近交点) */
    float t = (-b - sqrtf(delta)) / (2.0f * a);
    if (t < 0)
        return -1; /* 球在相机背后 */
    return t;
}

float intersect_sphere_fast(Ray ray, Sphere sphere)
{
    /* 1. 计算从球心指向光线起点的向量 */
    vec3 oc = vec3_sub(ray.origin, sphere.center);
    /* 2. 计算 b 的一半(点积) */
    float b_half = vec3_dot(ray.dir, oc);
    /* 3. 计算 c */
    float c = vec3_dot(oc, oc) - sphere.radius * sphere.radius;
    /* 4. 判别式(因为 |D|²=1,优化掉乘法) */
    float delta = b_half * b_half - c;
    if (delta < 0)
        return -1; /* 光线擦肩而过 */
    /* 5. 求最近的根 */
    float sqrt_delta = sqrtf(delta);
    float t = -b_half - sqrt_delta;
    if (t < 0)
        return -1; /* 球在相机背后 */
    return t;
}

vec3 sphere_normal_at(Sphere sphere, vec3 point)
{
    return vec3_normalize(vec3_sub(point, sphere.center));
}

void sphere_uv_at(Sphere sphere, vec3 point, float *out_u, float *out_v)
{
    static const float PI_F = 3.14159265358979323846f;

    vec3 n = sphere_normal_at(sphere, point);

    /* 经度:取 atan2(n.x, n.z).
       相机在原点朝 -Z 看,可见半球的外法线朝 +Z,
       这样正对本初子午线(u=0.5)看地球时,东经在右、西经在左. */
    float lon = atan2f(n.x, n.z);
    /* 纬度:y 轴正方向是北极,所以 lat 范围 [-π/2, π/2] */
    float lat = asinf(n.y);

    /* u:经度占整圈 360° 的比例,中心 u=0.5;v:纬度占 180° 的比例,北极 v=0 */
    *out_u = 0.5f + lon / (2.0f * PI_F);
    *out_v = 0.5f - lat / PI_F;
}
