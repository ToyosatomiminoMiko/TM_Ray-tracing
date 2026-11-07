#ifndef PNG_H_
#define PNG_H_

#include <stdint.h>

/*
PNG 图片(PngImage):解码后的统一 RGBA8 位图.

所有输入格式最终都转换成同一种布局,上层贴图代码只认这一种:
- 每个像素 4 字节,按 R,G,B,A 顺序;
- 行优先:先存第 0 行从左到右,再存第 1 行,依此类推;
- alpha=255 表示不透明,0..254 表示半透明(PNG 的 alpha 不预乘).

为什么统一成 RGBA8:上层 texture.c 不用关心源文件是灰度,RGB 还是调色板;
8 位/通道也够教学演示,省掉 16 位样本缩放的复杂度.
*/
typedef struct
{
    int width;        /* 像素宽 */
    int height;       /* 像素高 */
    uint8_t *pixels;  /* RGBA 连续存储,每像素 4 字节,行优先 */
} PngImage;

/*
从 PNG 文件加载并解码成 RGBA8.

支持的教学子集:
- 非隔行(interlace=0),位深 8;
- 颜色类型 0(灰度),2(RGB),3(调色板),4(灰度+alpha),6(RGBA);
- PLTE 调色板,以及 tRNS 透明(调色板 alpha 或灰度/RGB 颜色键).

不支持的文件(16 位深,Adam7 隔行等)会往 stderr 打印一条明确的错误
并返回 -1,而不是返回"看着能用但颜色错了"的图片.

成功返回 0,img 接管一块 malloc 的内存,用完必须调 png_free;
失败返回 -1,并且 img 被清零(width=height=0,pixels=NULL),不持有内存.
*/
int png_load(const char *path, PngImage *img);

/* 释放 png_load 分配的内存,并把 width/height/pixels 全部清零 */
void png_free(PngImage *img);

#endif /* PNG_H_ */
