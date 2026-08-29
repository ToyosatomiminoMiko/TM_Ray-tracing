#include "ray.h"

#include <math.h>

vec3 ray_at(Ray ray, float t)
{
    return vec3_add(ray.origin, vec3_scale(ray.dir, t));
}

vec3 ray_direction(int i, int j, int w, int h)
{
    /* 90 度垂直视场角等价于 fov = PI/2 */
    return ray_direction_fov(i, j, w, h, (float)(3.14159265358979323846 / 2.0));
}

vec3 ray_direction_fov(int i, int j, int w, int h, float fov)
{
    /*
    1.算 0..1 的像素中心
    屏幕中心是(0,0),这个像素在屏幕上的位置(百分比)
    */
    float u = (i + 0.5f) / (float)w;
    float v = (j + 0.5f) / (float)h;

    /*
    2.映射到 -1 到 +1
    0% -> -1, 50% -> 0, 100% -> +1
    */
    float x = 2.0f * u - 1.0f;
    float y = 2.0f * v - 1.0f;

    /* 3.修正宽高比(让 X 轴适应屏幕宽度) */
    float aspect_ratio = (float)w / (float)h;

    /* 4.Y 轴取反(图像坐标 Y 向下,指上为天) */
    y = -y;

    /*
    5.用视场角决定"能看到多宽"
    像素在 z=-1 平面上的坐标是 (x*half_w, y*half_h, -1)
    */
    float half_h = tanf(fov * 0.5f);
    float half_w = half_h * aspect_ratio;
    vec3 dir = {x * half_w, y * half_h, -1.0f};

    /* 6.归一化:只保留方向信息,长度为 1 */
    return vec3_normalize(dir);
}

vec3 ray_direction_subpixel(int i, int j, int w, int h, float fov, float ox, float oy)
{
    /* 像素内偏移:ox/oy 属于 0..1 */
    float u = (i + ox) / (float)w;
    float v = (j + oy) / (float)h;

    float x = 2.0f * u - 1.0f;
    float y = 2.0f * v - 1.0f;

    float aspect_ratio = (float)w / (float)h;
    y = -y;

    float half_h = tanf(fov * 0.5f);
    float half_w = half_h * aspect_ratio;
    vec3 dir = {x * half_w, y * half_h, -1.0f};
    return vec3_normalize(dir);
}
