#include "lesson08.h"

#include <math.h>

#include "lesson.h"
#include "mat4.h"
#include "plane.h"
#include "sphere.h"

#define DEG2RAD (3.14159265358979323846f / 180.0f)
#define FOV (60.0f * DEG2RAD) /* 垂直视场角 60 度 */

/*
第 8 课:轨道相机 + 透视
扩展点:
1. LookAt 观察矩阵:从眼睛看向目标点
2. 视空间里按视场角生成光线,再乘观察矩阵的转置转回世界空间
3. 相机绕 Y 轴旋转,产生环绕动画
*/
typedef struct
{
    Sphere sphere;
    vec3 albedo;
    float shininess;
} SceneObject;

/* 场景表:{{球心, 半径}, 颜色, 反光度};相机绕着这三个球转 */
static const SceneObject objects[] = {
    {{{-1.1f, 0.0f, -2.5f}, 0.9f}, {0.9f, 0.2f, 0.2f}, 32.0f},
    {{{1.1f, -0.15f, -2.2f}, 0.7f}, {0.2f, 0.7f, 0.2f}, 64.0f},
    {{{0.0f, 0.35f, -1.1f}, 0.5f}, {0.2f, 0.3f, 0.9f}, 128.0f},
};
#define OBJECT_COUNT (int)(sizeof(objects) / sizeof(objects[0]))

static const vec3 light_pos = {2.0f, 2.2f, 0.0f};
static const float ambient_strength = 0.12f;
static const float specular_strength = 0.8f;
static const float plane_y = -1.0f;
static const vec3 plane_normal = {0.0f, 1.0f, 0.0f};

/* 场景着色(球 + 地面):对一条光线找最近交点,再按命中物算光照 */
static vec3 trace_color(Ray ray)
{
    /* 先算地面,再算所有球,t 最小的才是"这束光线看见的表面" */
    float t_plane = 0.0f;
    int hit_plane = intersect_plane(ray, plane_normal, (vec3){0.0f, plane_y, 0.0f}, &t_plane);

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

    vec3 L, N, P;
    float diff, dist_to_light;
    int occluded = 0;

    if (nearest >= 0)
    {
        const SceneObject *obj = &objects[nearest];
        P = ray_at(ray, t_nearest);
        N = sphere_normal_at(obj->sphere, P);
        L = vec3_normalize(vec3_sub(light_pos, P));
        dist_to_light = vec3_length(vec3_sub(light_pos, P));
        diff = vec3_dot(N, L);
        if (diff < 0)
            diff = 0;

        vec3 shadow_origin = vec3_add(P, vec3_scale(N, 0.001f));
        Ray shadow_ray = {shadow_origin, L};
        for (int k = 0; k < OBJECT_COUNT; k++)
        {
            float ts = intersect_sphere_fast(shadow_ray, objects[k].sphere);
            if (ts > 0 && ts < dist_to_light)
            {
                occluded = 1;
                break;
            }
        }
        if (occluded)
            diff = 0;

        vec3 V = vec3_normalize(vec3_scale(ray.dir, -1.0f));
        vec3 half = vec3_normalize(vec3_add(L, V));
        float spec = 0.0f;
        if (!occluded)
        {
            float ndoth = vec3_dot(N, half);
            if (ndoth > 0)
                spec = powf(ndoth, obj->shininess) * specular_strength;
        }

        float brightness = ambient_strength + (1.0f - ambient_strength) * diff;
        return (vec3){
            obj->albedo.x * brightness + spec,
            obj->albedo.y * brightness + spec,
            obj->albedo.z * brightness + spec};
    }

    if (hit_plane && t_plane > 0)
    {
        P = ray_at(ray, t_plane);
        int parity = ((int)floorf(P.x) + (int)floorf(P.z)) & 1;
        float base = parity ? 0.15f : 0.65f;

        N = plane_normal;
        L = vec3_normalize(vec3_sub(light_pos, P));
        dist_to_light = vec3_length(vec3_sub(light_pos, P));
        diff = vec3_dot(N, L);
        if (diff < 0)
            diff = 0;

        vec3 shadow_origin = vec3_add(P, vec3_scale(N, 0.001f));
        Ray shadow_ray = {shadow_origin, L};
        for (int k = 0; k < OBJECT_COUNT; k++)
        {
            float ts = intersect_sphere_fast(shadow_ray, objects[k].sphere);
            if (ts > 0 && ts < dist_to_light)
            {
                occluded = 1;
                break;
            }
        }
        if (occluded)
            diff = 0;

        float brightness = ambient_strength + (1.0f - ambient_strength) * diff;
        return (vec3){base * brightness, base * brightness, base * brightness};
    }

    /* 背景:渐变天空 */
    float t = (ray.dir.y + 1.0f) * 0.5f;
    return (vec3){t, t, 1.0f};
}

int lesson08_render(framebuffer_t *fb, int frame_index, int frame_count)
{
    /*
    帧驱动动画:一整个循环 = frame_count 帧,相机绕 Y 轴转一整圈.
    角度只由帧号决定,和秒/帧率无关;首尾相接,循环播放无跳变.
    */
    float u = lesson_loop_progress(frame_index, frame_count);
    float angle = 2.0f * (float)M_PI * u;
    vec3 eye = {5.0f * cosf(angle), 1.2f, 5.0f * sinf(angle)};
    vec3 center = {0.0f, 0.0f, -1.0f};
    vec3 up = {0.0f, 1.0f, 0.0f};

    /* LookAt 观察矩阵 + 转置(旋转部分求逆) */
    mat4 view, view_t;
    look_at_4x4(view, eye, center, up);
    matrix_transpose_4x4(view_t, view);

    float aspect = (float)W / (float)H;
    float half_h = tanf(FOV * 0.5f);
    float half_w = half_h * aspect;

    for (int j = 0; j < H; j++)
    {
        for (int i = 0; i < W; i++)
        {
            /* 像素中心 -> NDC(-1..1) */
            float u = (i + 0.5f) / (float)W * 2.0f - 1.0f;
            float v = (j + 0.5f) / (float)H * 2.0f - 1.0f;

            /* 视空间光线方向(屏幕放在 z=-1) */
            vec4 view_dir = {u * half_w, -v * half_h, -1.0f, 0.0f};
            vec4 world_dir;
            matrix_vector_multiply(world_dir, view_t, view_dir);

            vec3 dir = vec3_normalize((vec3){world_dir[0], world_dir[1], world_dir[2]});
            vec3 color = trace_color((Ray){eye, dir});
            fb_set_pixel(fb, i, j, color_from_01(color.x, color.y, color.z));
        }
    }
    return 0;
}
