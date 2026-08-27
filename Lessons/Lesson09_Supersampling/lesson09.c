#include "lesson09.h"

#include <math.h>

#include "sphere.h"

/*
第 9 课:超采样抗锯齿(MSAA)
扩展点:
1. 每个像素不再只发 1 条光线,而是发 4x4=16 条子光线
2. 子光线均匀分布在像素内,颜色取平均
3. 画面左半:每像素 1 条(有锯齿);右半:每像素 16 条(平滑)
*/
typedef struct
{
    Sphere sphere;
    Vector_3 albedo;
    float shininess;
} SceneObject;

/* 场景表:{{球心, 半径}, 颜色, 反光度};与第 6 课同一套场景 */
static const SceneObject objects[] = {
    {{{-1.1f, 0.0f, -3.5f}, 0.9f}, {0.9f, 0.2f, 0.2f}, 32.0f},
    {{{1.1f, -0.15f, -3.2f}, 0.7f}, {0.2f, 0.7f, 0.2f}, 64.0f},
    {{{0.0f, 0.35f, -2.1f}, 0.5f}, {0.2f, 0.3f, 0.9f}, 128.0f},
};
#define OBJECT_COUNT (int)(sizeof(objects) / sizeof(objects[0]))

static const Vector_3 light_pos = {2.0f, 2.2f, -1.0f};
static const float ambient_strength = 0.12f;
static const float specular_strength = 0.8f;

/*
对像素 (i,j) 内偏移 (ox,oy)(0..1 表示像素内相对位置)采样一条光线,
返回该子像素的颜色(0..1).
*/
static Vector_3 sample_color(int i, int j, float ox, float oy)
{
    Vector_3 origin = {0.0f, 0.0f, 0.0f};
    Vector_3 dir = ray_direction_subpixel(
        i, j, W, H,
        (float)(3.14159265358979323846 / 2.0),
        ox, oy);
    Ray ray = {origin, dir};

    /* 与第 6 课相同:找最近球、算漫反射 + 高光 + 阴影 */
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

    if (nearest < 0)
    {
        float sky = (dir.y + 1.0f) * 0.5f;
        return (Vector_3){sky, sky, 1.0f};
    }

    const SceneObject *obj = &objects[nearest];
    Vector_3 P = ray_at(ray, t_nearest);
    Vector_3 N = sphere_normal_at(obj->sphere, P);
    Vector_3 L = vector3_normalize(vector3_sub(light_pos, P));
    Vector_3 V = vector3_normalize(vector3_scale(dir, -1.0f));
    Vector_3 half = vector3_normalize(vector3_add(L, V));

    float diff = vector3_dot(N, L);
    if (diff < 0)
        diff = 0;

    /* 阴影 */
    Vector_3 shadow_origin = vector3_add(P, vector3_scale(N, 0.001f));
    Ray shadow_ray = {shadow_origin, L};
    float dist_to_light = vector3_length(vector3_sub(light_pos, P));
    for (int k = 0; k < OBJECT_COUNT; k++)
    {
        float ts = intersect_sphere_fast(shadow_ray, objects[k].sphere);
        if (ts > 0 && ts < dist_to_light)
        {
            diff = 0;
            break;
        }
    }

    float spec = 0.0f;
    if (diff > 0)
    {
        float ndoth = vector3_dot(N, half);
        if (ndoth > 0)
            spec = powf(ndoth, obj->shininess) * specular_strength;
    }

    float brightness = ambient_strength + (1.0f - ambient_strength) * diff;
    return (Vector_3){
        obj->albedo.x * brightness + spec,
        obj->albedo.y * brightness + spec,
        obj->albedo.z * brightness + spec};
}

int lesson09_render(framebuffer_t *fb, float time_seconds)
{
    (void)time_seconds; /* 静态课程 */

    for (int j = 0; j < H; j++)
    {
        for (int i = 0; i < W; i++)
        {
            float r = 0, g = 0, b = 0;
            int samples;

            if (i < W / 2)
            {
                /* 左半:每像素 1 条光线,走像素中心 */
                samples = 1;
                Vector_3 c = sample_color(i, j, 0.5f, 0.5f);
                r += c.x;
                g += c.y;
                b += c.z;
            }
            else
            {
                /* 右半:每像素 4x4=16 条子光线,均匀分布后平均 */
                samples = 16;
                for (int sy = 0; sy < 4; sy++)
                {
                    for (int sx = 0; sx < 4; sx++)
                    {
                        float ox = (sx + 0.5f) / 4.0f;
                        float oy = (sy + 0.5f) / 4.0f;
                        Vector_3 c = sample_color(i, j, ox, oy);
                        r += c.x;
                        g += c.y;
                        b += c.z;
                    }
                }
            }

            fb_set_pixel(fb, i, j, color_from_01(r / samples, g / samples, b / samples));
        }
    }
    return 0;
}
