#ifndef RAY_H_
#define RAY_H_

#include "vec3.h"

/*
光线:一个起点 O 加一个方向 D(通常已经归一化)。
参数方程:P(t) = O + t*D,t 是沿光线的距离。
*/
typedef struct
{
    Vector_3 origin;
    Vector_3 dir;
} Ray;

/* 求光线上距离 t 处的点:P(t) = O + t*D */
Vector_3 ray_at(Ray ray, float t);

/*
由像素坐标生成光线方向(经典教学写法,垂直视场角 90 度):
1. 像素中心映射到 0..1
2. 再映射到 -1..+1
3. 乘宽高比修正
4. Y 取反(图像坐标向下)
5. 屏幕放在 z=-1,方向指向 -Z
*/
Vector_3 ray_direction(int i, int j, int w, int h);

/*
带垂直视场角 fov(弧度)的像素方向:
ndc_x = (i+0.5)/w*2-1
ndc_y = (j+0.5)/h*2-1
half_h = tan(fov/2)
half_w = half_h * (w/h)
方向 = normalize(ndc_x*half_w, -ndc_y*half_h, -1)
*/
Vector_3 ray_direction_fov(int i, int j, int w, int h, float fov);

/*
子像素采样方向(超采样/抗锯齿用):
同样把 (i,j) 映射到 NDC,但像素内位置由 (ox,oy)(0..1) 控制,
例如 ox=0.5,oy=0.5 就是像素中心。
*/
Vector_3 ray_direction_subpixel(int i, int j, int w, int h, float fov, float ox, float oy);

#endif /* RAY_H_ */
