#include "lesson03.h"

#include <math.h>
#include <string.h>

#include "mat4.h"
#include "triangle.h"

#define DEG2RAD (3.14159265358979323846f / 180.0f)

/*
第 3 课:光线追踪立方体
把立方体拆成 12 个三角形,对每个像素发射一条光线,
用 Möller–Trumbore 求最近交点,再用法线方向上色。
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

int lesson03_render(framebuffer_t *fb, int frame)
{
    /* 12 个三角形索引 */
    const uint8_t tri_indices[12][3] = {
        {0, 1, 2},
        {0, 2, 3}, /* 底面 */
        {5, 6, 7},
        {5, 7, 4}, /* 顶面 */
        {3, 2, 7},
        {3, 7, 4}, /* 前面 */
        {0, 5, 6},
        {0, 6, 1}, /* 后面 */
        {0, 3, 4},
        {0, 4, 5}, /* 左面 */
        {1, 6, 7},
        {1, 7, 2} /* 右面 */
    };

    box b2;
    Vector_3 origin = {0.0f, 0.0f, 0.0f};
    Matrix_4x4 S, R, T;

    /* 每帧旋转 6 度,总共 60 帧转完一圈 */
    float r = DEG2RAD * (float)(frame * 6);

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

    /* 变换顶点得到世界坐标 */
    for (int i = 0; i < 8; i++)
    {
        Vector_4 temp = {b1[i][0], b1[i][1], b1[i][2], 1};
        matrix_vector_multiply(temp, model, temp);
        memcpy(b2[i], temp, sizeof(Vector_4));
    }

    /* 光线追踪渲染 */
    for (int j = 0; j < H; j++)
    {
        for (int i = 0; i < W; i++)
        {
            Vector_3 dir = ray_direction(i, j, W, H);
            Ray ray = {origin, dir};
            float t_min = 1e30f;
            int hit = 0;
            Vector_3 hit_normal = {0, 0, 0};

            for (int k = 0; k < 12; k++)
            {
                Vector_3 V0 = {b2[tri_indices[k][0]][0], b2[tri_indices[k][0]][1], b2[tri_indices[k][0]][2]};
                Vector_3 V1 = {b2[tri_indices[k][1]][0], b2[tri_indices[k][1]][1], b2[tri_indices[k][1]][2]};
                Vector_3 V2 = {b2[tri_indices[k][2]][0], b2[tri_indices[k][2]][1], b2[tri_indices[k][2]][2]};

                float t, u, v;
                if (ray_triangle_intersect(ray, V0, V1, V2, &t, &u, &v))
                {
                    if (t > 0 && t < t_min)
                    {
                        t_min = t;
                        hit = 1;
                        /* 面法线 */
                        hit_normal = triangle_normal(V0, V1, V2);
                    }
                }
            }

            int r, g, b;
            if (hit)
            {
                r = (int)((hit_normal.x + 1.0f) * 0.5f * 255.0f);
                g = (int)((hit_normal.y + 1.0f) * 0.5f * 255.0f);
                b = (int)((hit_normal.z + 1.0f) * 0.5f * 255.0f);
            }
            else
            {
                /* 背景(渐变天空) */
                /* r = (int)((dir.y + 1.0f) * 0.5f * 255); */
                /* g = (int)((dir.y + 1.0f) * 0.5f * 255); */
                /* b = 255; */
                r = g = b = 0;
            }
            fb_set_pixel(fb, i, j, color_rgb((uint8_t)r, (uint8_t)g, (uint8_t)b));
        }
    }
    return 0;
}
