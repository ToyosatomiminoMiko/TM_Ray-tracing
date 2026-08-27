#include "framebuffer.h"

#include <string.h>

/* 颜色值定义 */
const pixel_color_t COLOR_PINK = 0xF5A9B8u;
const pixel_color_t COLOR_BLUE = 0x5BCEFAu;
const pixel_color_t COLOR_WHITE = 0xFFFFFFu;
const pixel_color_t COLOR_BLACK = 0x000000u;

static uint8_t clamp_u8(float v)
{
    if (v < 0.0f)
        return 0;
    if (v > 255.0f)
        return 255;
    return (uint8_t)v;
}

pixel_color_t color_rgb(uint8_t r, uint8_t g, uint8_t b)
{
    return ((uint32_t)r << 16) | ((uint32_t)g << 8) | (uint32_t)b;
}

pixel_color_t color_from_01(float r, float g, float b)
{
    return color_rgb(
        clamp_u8(r * 255.0f),
        clamp_u8(g * 255.0f),
        clamp_u8(b * 255.0f));
}

pixel_color_t color_scale(pixel_color_t c, float s)
{
    return color_rgb(
        clamp_u8(((c >> 16) & 0xFF) * s),
        clamp_u8(((c >> 8) & 0xFF) * s),
        clamp_u8((c & 0xFF) * s));
}

pixel_color_t color_add(pixel_color_t a, pixel_color_t b)
{
    return color_rgb(
        clamp_u8(((a >> 16) & 0xFF) + ((b >> 16) & 0xFF)),
        clamp_u8(((a >> 8) & 0xFF) + ((b >> 8) & 0xFF)),
        clamp_u8((a & 0xFF) + (b & 0xFF)));
}

void fb_clear(framebuffer_t *fb)
{
    memset(fb->pixels, 0, sizeof(fb->pixels));
}

void fb_set_pixel(framebuffer_t *fb, int x, int y, pixel_color_t color)
{
    if (x >= 0 && x < W && y >= 0 && y < H)
        fb->pixels[y][x] = color;
}

pixel_color_t fb_get_pixel(const framebuffer_t *fb, int x, int y)
{
    if (x >= 0 && x < W && y >= 0 && y < H)
        return fb->pixels[y][x];
    return 0;
}

void fb_draw_line(framebuffer_t *fb, int x1, int y1, int x2, int y2, pixel_color_t color)
{
    int t;
    int xerr = 0, yerr = 0, delta_x, delta_y, distance;
    int incx, incy, row, col;

    delta_x = x2 - x1;
    delta_y = y2 - y1;
    row = x1;
    col = y1;

    if (delta_x > 0)
        incx = 1;
    else if (delta_x == 0)
        incx = 0;
    else
    {
        incx = -1;
        delta_x = -delta_x;
    }

    if (delta_y > 0)
        incy = 1;
    else if (delta_y == 0)
        incy = 0;
    else
    {
        incy = -1;
        delta_y = -delta_y;
    }

    if (delta_x > delta_y)
        distance = delta_x;
    else
        distance = delta_y;

    for (t = 0; t <= distance + 1; t++)
    {
        fb_set_pixel(fb, row, col, color);
        xerr += delta_x;
        yerr += delta_y;
        if (xerr > distance)
        {
            xerr -= distance;
            row += incx;
        }
        if (yerr > distance)
        {
            yerr -= distance;
            col += incy;
        }
    }
}
