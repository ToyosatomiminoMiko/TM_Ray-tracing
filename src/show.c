#include "show.h"

// 颜色缓冲区
color_buffer_t Color_Buffer;

// 颜色值定义
const pixel_color_t COLOR_PINK = 0xF5A9B8u;
const pixel_color_t COLOR_BLUE = 0x5BCEFAu;
const pixel_color_t COLOR_WHITE = 0xFFFFFFu;

void graphmem_show()
{
    for (uint8_t y = 0; y < H; y++)
    {
        for (uint8_t x = 0; x < W; x++)
        {
            uint32_t color = Color_Buffer[x][y];
            printf(
                "\033[48;2;%d;%d;%dm  \033[0m",
                (color >> 16) & 0xFF,
                (color >> 8) & 0xFF,
                color & 0xFF);
        }
        printf("\r\n");
    }
}

void set_point_color(const uint8_t x, const uint16_t y, pixel_color_t color)
{
    // 保存颜色
    if (x < W && y < H)
        Color_Buffer[x][y] = (uint32_t)color;
}

void clear_gmem()
{
    memset(Color_Buffer, 0, sizeof(color_buffer_t));
}

void draw_line(const uint8_t x1, const uint8_t y1, const uint8_t x2, const uint8_t y2, pixel_color_t color)
{
    uint16_t t;
    int16_t xerr = 0, yerr = 0, delta_x, delta_y, distance;
    int16_t incx, incy, row, col;

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
        set_point_color(row, col, color);
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