#include "lesson04.h"

#include "sphere.h"

/*
第 4 课:渲染球体
用一元二次方程求光线与球的交点,
把命中点的法线方向映射成颜色,并保留渐变天空背景.
*/
int lesson04_render(framebuffer_t *fb, int frame_index, int frame_count)
{
    (void)frame_index; /* 静态课程:只渲染一帧 */
    (void)frame_count;

    /* 定义一个球:放在 (0, 0, -3) 处,半径 1.0 */
    Sphere sphere = {{0.0f, 0.0f, -3.0f}, 1.0f};
    vec3 origin = {0.0f, 0.0f, 0.0f};

    for (int j = 0; j < H; j++)
    {
        for (int i = 0; i < W; i++)
        {
            vec3 dir = ray_direction(i, j, W, H);
            Ray ray = {origin, dir};

            /* 调用性能优化版求交 */
            float t = intersect_sphere_fast(ray, sphere);
            /* float t = intersect_sphere_simple(ray, sphere); */
            int r, g, b;
            if (t > 0)
            {
                /* 命中!计算命中点的位置 */
                vec3 hit_point = ray_at(ray, t);
                /* 计算法线(从球心指向命中点,并归一化) */
                vec3 normal = sphere_normal_at(sphere, hit_point);

                /* 把法线方向映射成颜色 */
                r = (int)((normal.x + 1.0f) * 0.5f * 255.0f);
                g = (int)((normal.y + 1.0f) * 0.5f * 255.0f);
                b = (int)((normal.z + 1.0f) * 0.5f * 255.0f);
            }
            else
            {
                /* 背景色(渐变天空) */
                r = (int)((dir.y + 1.0f) * 0.5f * 255.0f);
                g = (int)((dir.y + 1.0f) * 0.5f * 255.0f);
                b = 255;
            }
            fb_set_pixel(fb, i, j, color_rgb((uint8_t)r, (uint8_t)g, (uint8_t)b));
        }
    }
    return 0;
}
