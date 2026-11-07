#ifndef TEXTURE_H_
#define TEXTURE_H_

#include <stdint.h>

#include "vec3.h"

/*
贴图(Texture):从图片文件加载,按 UV 坐标采样的数据.

纹理坐标 UV 约定:
- u 是水平方向,0 = 图片最左,1 = 图片最右;
- v 是垂直方向,0 = 图片最上,1 = 图片最下;
- 对球体世界地图,u 对应经度,v 对应纬度(见 sphere_uv_at).

像素统一按 RGBA 存储(每像素 4 字节):PPM 没有 alpha 通道,加载时 alpha 填 255;
PNG 带 alpha,玻璃这类半透明材质要保留 alpha 才能做透明混合.
采样函数是"纯函数":同一个贴图 + 同一组 UV,永远返回同一个颜色,
不会修改贴图内容,所以可以放心在光线追踪的递归里反复调用.
*/
typedef struct
{
    int width;        /* 像素宽 */
    int height;       /* 像素高 */
    uint8_t *pixels;  /* RGBA 连续存储,每像素 4 字节,行优先 */
} Texture;

/*
从 PPM 文件加载贴图,支持:
- P3(纯文本):每像素三个十进制数;
- P6(二进制):每像素三个字节.
两种格式都自动跳过 # 开头的注释行;PPM 没有 alpha,加载后 alpha=255.
成功返回 0;失败返回 -1,并且 tex 不持有任何内存.
*/
int texture_load_ppm(Texture *tex, const char *path);

/*
从 PNG 文件加载贴图(由 Core/raytrace/png.c 解码成 RGBA8).
支持非隔行,8 位深的灰度/RGB/调色板/灰度+alpha/RGBA PNG;
玻璃素材里的透明像素会原样保留 alpha.
成功返回 0;失败返回 -1,并且 tex 不持有任何内存.
*/
int texture_load_png(Texture *tex, const char *path);

/* 释放贴图占用的内存,并把指针置空 */
void texture_free(Texture *tex);

/*
最近邻采样:直接取离 (u,v) 最近的像素.
速度快,能清楚看到像素块,适合教学对照;
u 做循环包裹,v 夹紧到 [0,1].
返回 0..1 的 RGB 颜色,忽略 alpha.
*/
vec3 texture_sample_nearest(const Texture *tex, float u, float v);

/*
最近邻采样并输出 alpha(玻璃等半透明材质用):
返回 0..1 的 RGB(未预乘 alpha),同时把该像素的 alpha 写进 out_alpha.
调用方做 alpha 混合时用 out = src.rgb*alpha + dst*(1-alpha).
u/v 的包裹与夹紧规则和 texture_sample_nearest 相同.
*/
vec3 texture_sample_rgba_nearest(const Texture *tex, float u, float v, float *out_alpha);

/*
双线性采样:取 2x2 相邻像素,先在水平方向插值,再在垂直方向插值.
画面平滑,是渲染器里最常用的采样方式;
u 循环包裹(地图左右边缘无缝),v 夹紧(北极/南极不包裹).
返回 0..1 的 RGB 颜色,忽略 alpha.
*/
vec3 texture_sample(const Texture *tex, float u, float v);

#endif /* TEXTURE_H_ */
