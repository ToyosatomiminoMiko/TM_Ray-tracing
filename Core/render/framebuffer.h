#ifndef FRAMEBUFFER_H_
#define FRAMEBUFFER_H_

#include <stdint.h>

/*
内存帧缓冲:所有课程统一"先渲染到内存,再输出".
终端动画和 PPM 文件都从这里的像素数据读取,不再允许直接写文件.
*/

/* 分辨率:160*80 */
#define W (int)160
#define H (int)80

/* 一个像素:32 位里低 24 位存放 0xRRGGBB */
typedef uint32_t pixel_color_t;

typedef struct
{
    pixel_color_t pixels[H][W]; /* 行优先:pixels[y][x] */
} framebuffer_t;

/* 常用颜色 */
extern const pixel_color_t COLOR_PINK;
extern const pixel_color_t COLOR_BLUE;
extern const pixel_color_t COLOR_WHITE;
extern const pixel_color_t COLOR_BLACK;

/* 把 r,g,b(0..255) 打包成 0xRRGGBB */
pixel_color_t color_rgb(uint8_t r, uint8_t g, uint8_t b);

/* 把 0..1 浮点颜色打包,自动夹到 0..255 */
pixel_color_t color_from_01(float r, float g, float b);

/* 颜色整体乘亮度(会夹到 0..255),用于光照 */
pixel_color_t color_scale(pixel_color_t c, float s);

/* 两个颜色相加(会夹到 0..255) */
pixel_color_t color_add(pixel_color_t a, pixel_color_t b);

/* 清空整个缓冲(全部变成黑色) */
void fb_clear(framebuffer_t *fb);

/* 画一个像素(越界自动忽略) */
void fb_set_pixel(framebuffer_t *fb, int x, int y, pixel_color_t color);

/* 读一个像素 */
pixel_color_t fb_get_pixel(const framebuffer_t *fb, int x, int y);

/*
Bresenham 直线算法:
用整数误差增量决定每一步是走 x 还是走 y,
避免浮点运算和斜率除零,适合像素网格画线.
*/
void fb_draw_line(framebuffer_t *fb, int x1, int y1, int x2, int y2, pixel_color_t color);

#endif /* FRAMEBUFFER_H_ */
