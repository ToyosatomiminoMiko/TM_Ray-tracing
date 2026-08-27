#include "lesson06.h"

#include <math.h>

#include "sphere.h"

/*
第 6 课:多个球 + 镜面高光(Blinn-Phong)
扩展点:
1. 场景里有多个球,每束光线找"最近的"那个交点
2. 阴影射线要检测所有球体
3. 加入高光:半程向量 H = normalize(L+V),高光 = pow(N·H, 反光度)
*/
typedef struct
{
    Sphere sphere;
    Vector_3 albedo; /* 基础颜色 */
    float shininess; /* 反光度,越大高光越集中 */
} SceneObject;

/*
场景表:每行是一个球
格式:{{球心(x,y,z), 半径}, 颜色(RGB), 反光度}
反光度越大,高光越小越亮(更像光滑塑料/金属).
*/
static const SceneObject objects[] = {
    {{{-1.1f, 0.0f, -3.5f}, 0.9f}, {0.9f, 0.2f, 0.2f}, 32.0f},
    {{{1.1f, -0.15f, -3.2f}, 0.7f}, {0.2f, 0.7f, 0.2f}, 64.0f},
    {{{0.0f, 0.35f, -2.1f}, 0.5f}, {0.2f, 0.3f, 0.9f}, 128.0f},
};
#define OBJECT_COUNT (int)(sizeof(objects) / sizeof(objects[0]))

/* 点光源位置;ambient 兜底亮度;specular 高光强度上限 */
static const Vector_3 light_pos = {2.0f, 2.2f, -1.0f};
static const float ambient_strength = 0.12f;
static const float specular_strength = 0.8f;

int lesson06_render(framebuffer_t *fb, float time_seconds)
{
    (void)time_seconds; /* 静态课程 */

    Vector_3 origin = {0.0f, 0.0f, 0.0f};

    for (int j = 0; j < H; j++)
    {
        for (int i = 0; i < W; i++)
        {
            Vector_3 dir = ray_direction(i, j, W, H);
            Ray ray = {origin, dir};

            /* 找到最近的球:t_nearest 初始为"无穷大",保证第一次命中必然更近 */
            int nearest = -1;
            float t_nearest = 1e30f;
            for (int k = 0; k < OBJECT_COUNT; k++)
            {
                float t = intersect_sphere_fast(ray, objects[k].sphere);
                if (t > 0 && t < t_nearest)
                {
                    t_nearest = t;
                    nearest = k;
                }
            }

            Vector_3 color;
            if (nearest >= 0)
            {
                const SceneObject *obj = &objects[nearest];

                /* 命中点与法线 */
                Vector_3 P = ray_at(ray, t_nearest);
                Vector_3 N = sphere_normal_at(obj->sphere, P);

                /* 光源方向 L(命中点指向灯)与视线方向 V(命中点指向相机) */
                Vector_3 L = vector3_normalize(vector3_sub(light_pos, P));
                Vector_3 V = vector3_normalize(vector3_sub(origin, P));
                /* 半程向量 H = normalize(L + V) */
                Vector_3 half = vector3_normalize(vector3_add(L, V));

                /* 漫反射系数:法线与光线夹角越小越亮,背光取 0 */
                float diff = vector3_dot(N, L);
                if (diff < 0)
                    diff = 0;

                /* 阴影判定:检查所有球体是否挡住光源 */
                float in_shadow = 0.0f;
                Vector_3 shadow_origin = vector3_add(P, vector3_scale(N, 0.001f));
                Ray shadow_ray = {shadow_origin, L};
                float dist_to_light = vector3_length(vector3_sub(light_pos, P));
                for (int k = 0; k < OBJECT_COUNT; k++)
                {
                    float ts = intersect_sphere_fast(shadow_ray, objects[k].sphere);
                    if (ts > 0 && ts < dist_to_light)
                    {
                        in_shadow = 1.0f;
                        break; /* 提前退出:只需要知道"有没有被挡" */
                    }
                }

                if (in_shadow > 0.5f)
                    diff = 0.0f;

                /* 镜面高光:pow(max(N·H,0), shininess) */
                float spec = 0.0f;
                if (in_shadow < 0.5f)
                {
                    float ndoth = vector3_dot(N, half);
                    if (ndoth > 0)
                        spec = powf(ndoth, obj->shininess) * specular_strength;
                }

                /* 最终颜色 = 底色 * (环境光保底 + 漫反射) + 白色高光 */
                float brightness = ambient_strength + (1.0f - ambient_strength) * diff;
                color.x = obj->albedo.x * brightness + spec;
                color.y = obj->albedo.y * brightness + spec;
                color.z = obj->albedo.z * brightness + spec;
            }
            else
            {
                /* 背景色(渐变天空) */
                float t = (dir.y + 1.0f) * 0.5f;
                color = (Vector_3){t, t, 1.0f};
            }

            fb_set_pixel(fb, i, j, color_from_01(color.x, color.y, color.z));
        }
    }
    return 0;
}
