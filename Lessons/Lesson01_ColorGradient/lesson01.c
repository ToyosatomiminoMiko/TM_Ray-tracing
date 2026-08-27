#include "lesson01.h"

#include "ray.h"

/*
第 1 课:颜色渐变
把每个像素对应的光线方向映射成颜色,理解"方向 -> 颜色".
*/
int lesson01_render(framebuffer_t *fb, float time_seconds)
{
    (void)time_seconds; /* 静态课程:无论哪个时刻,画面都一样 */

    /* 遍历每一行(从上到下) */
    for (int j = 0; j < H; j++)
    {
        for (int i = 0; i < W; i++)
        {
            /* 生成这束光的方向 */
            Vector_3 dir = ray_direction(i, j, W, H);
            /* 把方向向量映射成 0..255 的颜色值 */
            /* 如果方向朝右(x正)红色多,朝上(y正)绿色多 */
            int r = (int)((dir.x + 1.0f) * 0.5f * 255.0f); /* 把 -1..1 映射到 0..255 */
            int g = (int)((dir.y + 1.0f) * 0.5f * 255.0f);
            int b = (int)((dir.z + 1.0f) * 0.5f * 255.0f); /* 由于看向 -Z,这里会偏暗蓝 */
            fb_set_pixel(fb, i, j, color_rgb((uint8_t)r, (uint8_t)g, (uint8_t)b));
        }
    }
    return 0;
}
