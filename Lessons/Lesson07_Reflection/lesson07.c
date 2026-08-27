#include "lesson07.h"

#include <math.h>

#include "plane.h"
#include "sphere.h"

/*
第 7 课:平面棋盘格 + 镜面反射
扩展点:
1. 无限大平面求交(地面),用棋盘格颜色
2. 反射光线:r = d - 2*(d·n)*n,递归追踪(限制最大深度)
3. 金属球反射周围场景,漫反射球基本不反射
*/
#define MAX_DEPTH 4

typedef struct
{
    Sphere sphere;
    Vector_3 albedo;
    float shininess;
    float reflectivity; /* 0 = 不反射,1 = 完全镜面 */
} SceneObject;

static const SceneObject objects[] = {
    /* 镜面球(金属感) */
    {{{0.2f, -0.25f, -2.8f}, 0.7f}, {0.7f, 0.7f, 0.7f}, 64.0f, 0.9f},
    /* 红色漫反射球:放在镜面球右前方,让镜面球能看到它 */
    {{{1.45f, 0.0f, -1.8f}, 0.75f}, {0.9f, 0.2f, 0.2f}, 32.0f, 0.15f},
};
#define OBJECT_COUNT (int)(sizeof(objects) / sizeof(objects[0]))

static const Vector_3 light_pos = {-1.5f, 2.0f, -1.5f};
static const float ambient_strength = 0.12f;
static const float specular_strength = 0.5f;
static const float plane_y = -1.0f;
static const Vector_3 plane_normal = {0.0f, 1.0f, 0.0f};

/* 判断阴影:从 P 沿 L 方向发射光线,任何球挡住都算在阴影里 */
static int in_shadow(Vector_3 P, Vector_3 L, float dist_to_light, Vector_3 offset_dir)
{
    /* 沿表面法线偏移,防止阴影光线撞到自己 */
    Vector_3 shadow_origin = vector3_add(P, vector3_scale(offset_dir, 0.001f));
    Ray shadow_ray = {shadow_origin, L};
    for (int k = 0; k < OBJECT_COUNT; k++)
    {
        float ts = intersect_sphere_fast(shadow_ray, objects[k].sphere);
        if (ts > 0 && ts < dist_to_light)
            return 1;
    }
    return 0;
}

/* 球体局部光照:环境光 + 漫反射 + 高光 */
static Vector_3 shade_sphere(const SceneObject *obj, Vector_3 P, Vector_3 N, Vector_3 view_dir)
{
    Vector_3 L = vector3_normalize(vector3_sub(light_pos, P));
    Vector_3 V = vector3_normalize(vector3_scale(view_dir, -1.0f));
    Vector_3 half = vector3_normalize(vector3_add(L, V));

    float diff = vector3_dot(N, L);
    if (diff < 0)
        diff = 0;

    float spec = 0.0f;
    if (!in_shadow(P, L, vector3_length(vector3_sub(light_pos, P)), N))
    {
        float ndoth = vector3_dot(N, half);
        if (ndoth > 0)
            spec = powf(ndoth, obj->shininess) * specular_strength;
    }
    else
    {
        diff = 0.0f;
    }

    float brightness = ambient_strength + (1.0f - ambient_strength) * diff;
    return (Vector_3){
        obj->albedo.x * brightness + spec,
        obj->albedo.y * brightness + spec,
        obj->albedo.z * brightness + spec};
}

/* 地面棋盘格颜色:根据 (x+z) 的奇偶性决定黑白 */
static Vector_3 checker_color(Vector_3 P)
{
    int parity = ((int)floorf(P.x) + (int)floorf(P.z)) & 1;
    float c = parity ? 0.15f : 0.65f;
    return (Vector_3){c, c, c};
}

/* 递归追踪一条光线,返回 0..1 颜色 */
static Vector_3 trace_color(Ray ray, int depth)
{
    /* 求场景里最近的交点(平面 + 所有球) */
    float t_plane = 0.0f;
    int hit_plane = intersect_plane(ray, plane_normal, (Vector_3){0.0f, plane_y, 0.0f}, &t_plane);

    int nearest = -1;
    float t_nearest = hit_plane ? t_plane : 1e30f;
    for (int k = 0; k < OBJECT_COUNT; k++)
    {
        float t = intersect_sphere_fast(ray, objects[k].sphere);
        if (t > 0 && t < t_nearest)
        {
            t_nearest = t;
            nearest = k;
        }
    }

    if (nearest >= 0)
    {
        const SceneObject *obj = &objects[nearest];
        Vector_3 P = ray_at(ray, t_nearest);
        Vector_3 N = sphere_normal_at(obj->sphere, P);
        Vector_3 local = shade_sphere(obj, P, N, ray.dir);

        /* 反射:r = d - 2*(d·n)*n,再递归一层 */
        if (obj->reflectivity > 0.0f && depth < MAX_DEPTH)
        {
            Vector_3 R = vector3_reflect(ray.dir, N);
            Vector_3 refl_origin = vector3_add(P, vector3_scale(N, 0.001f));
            Vector_3 refl = trace_color((Ray){refl_origin, R}, depth + 1);
            /* 局部颜色和反射颜色按反射率混合 */
            return vector3_lerp(local, refl, obj->reflectivity);
        }
        return local;
    }

    if (hit_plane && t_plane > 0)
    {
        Vector_3 P = ray_at(ray, t_plane);
        Vector_3 base = checker_color(P);

        /* 平面也做简单的漫反射 + 阴影 */
        Vector_3 L = vector3_normalize(vector3_sub(light_pos, P));
        float diff = vector3_dot(plane_normal, L);
        if (diff < 0)
            diff = 0;
        if (in_shadow(P, L, vector3_length(vector3_sub(light_pos, P)), plane_normal))
            diff = 0;

        float brightness = ambient_strength + (1.0f - ambient_strength) * diff;
        return (Vector_3){base.x * brightness, base.y * brightness, base.z * brightness};
    }

    /* 背景:渐变天空 */
    float t = (ray.dir.y + 1.0f) * 0.5f;
    return (Vector_3){t, t, 1.0f};
}

int lesson07_render(framebuffer_t *fb, int frame)
{
    (void)frame;

    Vector_3 origin = {0.0f, 0.0f, 0.0f};
    for (int j = 0; j < H; j++)
    {
        for (int i = 0; i < W; i++)
        {
            Vector_3 dir = ray_direction(i, j, W, H);
            Vector_3 color = trace_color((Ray){origin, dir}, 0);
            fb_set_pixel(fb, i, j, color_from_01(color.x, color.y, color.z));
        }
    }
    return 0;
}
