#ifndef SHOW_H_
#define SHOW_H_

#include <stdint.h>
#include <stdio.h>
#include <string.h>

// resolution: 160*80
#define H (uint8_t)80
#define W (uint8_t)160

// 颜色缓冲区,每个像素一个字节
typedef uint32_t color_buffer_t[W][H];

// 颜色声明
typedef uint32_t pixel_color_t;
extern const pixel_color_t COLOR_PINK;
extern const pixel_color_t COLOR_BLUE;
extern const pixel_color_t COLOR_WHITE;

void graphmem_show();
void set_point_color(const uint8_t x, const uint8_t y, pixel_color_t color);
void clear_gmem();
void draw_line(
    const uint8_t x1, const uint8_t y1,
    const uint8_t x2, const uint8_t y2, pixel_color_t color);

#endif /* SHOW_H_ */
