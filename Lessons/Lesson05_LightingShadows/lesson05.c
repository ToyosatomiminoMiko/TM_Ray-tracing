#include "lesson05.h"

#include "sphere.h"

/*
第 5 课:光照与阴影
在球体求交基础上加入:
1. 命中点 P 与法线 N
2. 阴影判定(从 P 沿光源方向发一条阴影光线)
3. 漫反射光照(环境光保底 + N·L)
画面只渲染进内存帧缓冲,不再直接写文件.
*/
int lesson05_render(framebuffer_t *fb, float time_seconds)
{
    (void)time_seconds; /* 静态课程 */

    /* --- 场景设置 --- */
    /* 1. 球体:放在正前方,红色 */
    Sphere sphere = {{0.0f, 0.0f, -3.0f}, 1.0f};
    Vector_3 albedo = {1.0f, 0.2f, 0.2f};

    /* 2. 点光源:放在右上方(观察者的视角) */
    Vector_3 light_pos = {1.5f, 1.5f, -2.0f};
    float ambient_strength = 0.15f; /* 环境光(防止死黑) */

    Vector_3 origin = {0.0f, 0.0f, 0.0f};

    for (int j = 0; j < H; j++)
    {
        for (int i = 0; i < W; i++)
        {
            Vector_3 dir = ray_direction(i, j, W, H);
            Ray ray = {origin, dir};

            /* -------- 第一部分:主光线撞球 -------- */
            float t = intersect_sphere_fast(ray, sphere);

            int r, g, b;
            if (t > 0)
            {
                /* 1. 计算命中点 P */
                Vector_3 P = ray_at(ray, t);

                /* 2. 计算法线 N(球心指向P,再归一化) */
                Vector_3 N = sphere_normal_at(sphere, P);

                /* -------- 第二部分:阴影判定 -------- */
                /* 光源方向 L = (光源位置 - P) 归一化 */
                Vector_3 L = vector3_normalize(vector3_sub(light_pos, P));
                float dist_to_light = vector3_length(vector3_sub(light_pos, P));

                /* [性能优化第1招:微小偏移] 防止阴影光线撞到自己脸上 */
                Vector_3 shadow_origin = vector3_add(P, vector3_scale(N, 0.001f));

                /* 发射阴影光线,判断是否被遮挡 */
                /* [性能优化第2招:只算有没有挡,不算最近交点(提前退出)] */
                Ray shadow_ray = {shadow_origin, L};
                float t_shadow = intersect_sphere_fast(shadow_ray, sphere);

                float in_shadow = 0.0f;
                if (t_shadow > 0 && t_shadow < dist_to_light)
                    in_shadow = 1.0f; /* 处于阴影中 */

                /* -------- 第三部分:漫反射光照 -------- */
                /* 光线和法线的夹角越小越亮 */
                float diff = vector3_dot(N, L);
                if (diff < 0)
                    diff = 0; /* 光从背后打过来就是纯黑 */

                if (in_shadow > 0.5f)
                    diff = 0.0f; /* 被挡住了没光 */

                /* [最终亮度]环境光保底 + 漫反射光照 */
                float brightness = ambient_strength + (1.0f - ambient_strength) * diff;

                /* 乘上球的颜色,映射到 0~255 */
                r = (int)(albedo.x * brightness * 255.0f);
                g = (int)(albedo.y * brightness * 255.0f);
                b = (int)(albedo.z * brightness * 255.0f);
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
