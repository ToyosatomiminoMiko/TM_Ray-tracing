#include "lesson02.h"

#include <math.h>
#include <string.h>

#include "mat4.h"

#define DEG2RAD (3.14159265358979323846f / 180.0f)

/*
第 2 课:立方体线框动画
用模型矩阵(缩放 -> 旋转 -> 平移)变换 8 个顶点,
再用 Bresenham 直线把 12 条棱画进内存帧缓冲.
*/
static const box b1 = {
    {-1, -1, -1, 1},
    {1, -1, -1, 1},
    {1, -1, 1, 1},
    {-1, -1, 1, 1},
    {-1, 1, 1, 1},
    {-1, 1, -1, 1},
    {1, 1, -1, 1},
    {1, 1, 1, 1}};

/* 声明式绘图:把世界坐标(以屏幕中心为原点)换算成像素坐标 */
static void line(framebuffer_t *fb, float x0, float y0, float x1, float y1, pixel_color_t color)
{
    fb_draw_line(
        fb,
        (int)(x0 + W / 2), (int)(y0 + H / 2),
        (int)(x1 + W / 2), (int)(y1 + H / 2), color);
}

int lesson02_render(framebuffer_t *fb, float time_seconds)
{
    /* 变换结果储存 */
    box b2;
    Matrix_4x4 S, R, T;

    /*
    时间驱动动画:每秒转一整圈(360 度).
    60fps 时每帧自然相差 6 度;以后改帧率,转圈速度也不会变.
    */
    float r = DEG2RAD * (time_seconds * 360.0f);

    /* 构造模型矩阵:先缩放,再旋转,再平移 */
    Matrix_4x4 model = {
        {1, 0, 0, 0},
        {0, 1, 0, 0},
        {0, 0, 1, 0},
        {0, 0, 0, 1}};
    scale_3d(S, 15, 15, 15);    /* 缩放 */
    rotate_zyx_3d(R, r, -r, r); /* 旋转 绕自身轴 */
    translate_3d(T, 0, 0, -50); /* 平移到相机前方 */

    /* 矩阵乘法:model = T * R * S 顺序很重要 */
    matrix_multiply_4x4(model, S, model); /* model = S */
    matrix_multiply_4x4(model, R, model); /* model = R*S */
    matrix_multiply_4x4(model, T, model); /* model = T*R*S */

    /* 变换 8 个顶点 */
    for (int i = 0; i < 8; i++)
    {
        Vector_4 temp_vec0 = {b1[i][0], b1[i][1], b1[i][2], 1};
        matrix_vector_multiply(temp_vec0, model, temp_vec0);
        memcpy(b2[i], temp_vec0, sizeof(Vector_4));
    }

    /* 底面边 - 白色 */
    line(fb, b2[0][0], b2[0][1], b2[1][0], b2[1][1], COLOR_WHITE);
    line(fb, b2[1][0], b2[1][1], b2[2][0], b2[2][1], COLOR_WHITE);
    line(fb, b2[2][0], b2[2][1], b2[3][0], b2[3][1], COLOR_WHITE);
    line(fb, b2[3][0], b2[3][1], b2[0][0], b2[0][1], COLOR_WHITE);

    /* 垂直棱 - 粉色 */
    line(fb, b2[3][0], b2[3][1], b2[4][0], b2[4][1], COLOR_PINK);
    line(fb, b2[0][0], b2[0][1], b2[5][0], b2[5][1], COLOR_PINK);
    line(fb, b2[1][0], b2[1][1], b2[6][0], b2[6][1], COLOR_PINK);
    line(fb, b2[2][0], b2[2][1], b2[7][0], b2[7][1], COLOR_PINK);

    /* 顶面边 - 蓝色 */
    line(fb, b2[5][0], b2[5][1], b2[6][0], b2[6][1], COLOR_BLUE);
    line(fb, b2[6][0], b2[6][1], b2[7][0], b2[7][1], COLOR_BLUE);
    line(fb, b2[7][0], b2[7][1], b2[4][0], b2[4][1], COLOR_BLUE);
    line(fb, b2[4][0], b2[4][1], b2[5][0], b2[5][1], COLOR_BLUE);

    return 0;
}
