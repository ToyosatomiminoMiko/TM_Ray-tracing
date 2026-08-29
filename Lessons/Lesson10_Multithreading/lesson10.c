#include "lesson10.h"

#include <math.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#include "mat4.h"
#include "plane.h"
#include "sphere.h"

/*
第 10 课(支线):多线程渲染
扩展点:
1. 每个像素互相独立,是典型的"可并行"任务
2. 把画面按"行带"切成若干块,每个线程渲染一块
3. 同一场景左半用单线程、右半用多线程,渲染后打印两种耗时

运行时可设置环境变量 RAY_THREADS=8 来指定线程数,默认按 CPU 核数.
*/

#define DEG2RAD (3.14159265358979323846f / 180.0f)
#define FOV (60.0f * DEG2RAD)   /* 垂直视场角,与第 8 课一致 */
#define SS 2                    /* 每像素 2x2 = 4 条子光线,让渲染有足够工作量又不至于太慢 */
#define MID_X (W / 2)           /* 左/右两个对比区的分界线 */
#define MAX_THREADS 32

typedef struct
{
    Sphere sphere;
    vec3 albedo;
    float shininess;
} SceneObject;

/* 场景:三个球 + 棋盘格地面,场景和相机复用第 8 课 */
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

/* 线程条使用的颜色,方便看出"谁负责哪几行" */
static const pixel_color_t thread_colors[] = {
    0xE74C3Cu, 0x2ECC71u, 0x3498DBu, 0xF1C40Fu,
    0x9B59B6u, 0x1ABC9Cu, 0xE67E22u, 0x95A5A6u,
};
#define THREAD_COLOR_COUNT \
    (int)(sizeof(thread_colors) / sizeof(thread_colors[0]))

/* 对一条光线做着色:找最近交点 -> 漫反射/阴影/高光 */
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

/* 当前墙上时钟(秒),用来量"真实经过了多少时间" */
static double wall_seconds(void)
{
    struct timespec ts;
    clock_gettime(1, &ts);
    return (double)ts.tv_sec + (double)ts.tv_nsec * 1e-9;
}

/*
渲染像素 (i,j):在像素内均匀取 SSxSS 个子像素,
每个子像素发一条光线,颜色平均后写回.
这一步只依赖 (i,j),不依赖其他像素 -> 天然适合并行.
*/
static vec3 sample_pixel(int i, int j, const mat4 view_t, vec3 eye)
{
    vec3 sum = {0.0f, 0.0f, 0.0f};

    for (int sy = 0; sy < SS; sy++)
    {
        for (int sx = 0; sx < SS; sx++)
        {
            float ox = (sx + 0.5f) / (float)SS;
            float oy = (sy + 0.5f) / (float)SS;

            /* 先得到相机空间方向,再用观察矩阵转置回到世界空间 */
            vec3 cam_dir = ray_direction_subpixel(i, j, W, H, FOV, ox, oy);
            vec4 view_dir = {cam_dir.x, cam_dir.y, cam_dir.z, 0.0f};
            vec4 world_dir;
            matrix_vector_multiply(world_dir, view_t, view_dir);

            vec3 dir = vec3_normalize((vec3){world_dir[0], world_dir[1], world_dir[2]});
            vec3 c = trace_color((Ray){eye, dir});
            sum = vec3_add(sum, c);
        }
    }

    return vec3_scale(sum, 1.0f / (float)(SS * SS));
}

/* 一个线程负责的矩形区域;每行像素互相独立,不同线程写不同行,所以没有数据竞争 */
typedef struct
{
    framebuffer_t *fb;
    int x0, x1;
    int y0, y1;
    mat4 view_t;
    vec3 eye;
} render_job_t;

/* 线程入口:只渲染自己分到的行带 */
static void *render_rows(void *arg)
{
    render_job_t *job = (render_job_t *)arg;
    for (int j = job->y0; j < job->y1; j++)
    {
        for (int i = job->x0; i < job->x1; i++)
        {
            vec3 c = sample_pixel(i, j, job->view_t, job->eye);
            fb_set_pixel(job->fb, i, j, color_from_01(c.x, c.y, c.z));
        }
    }
    return NULL;
}

/* 决定线程数:优先读 RAY_THREADS,否则取 CPU 核数 */
static int resolve_thread_count(void)
{
    const char *env = getenv("RAY_THREADS");
    if (env && *env)
    {
        int n = atoi(env);
        if (n >= 1 && n <= MAX_THREADS)
            return n;
        fprintf(stderr, "[lesson10] RAY_THREADS=%s 无效,回退到自动检测\n", env);
    }

    long n = sysconf(_SC_NPROCESSORS_ONLN);
    if (n < 1)
        n = 1;
    if (n > MAX_THREADS)
        n = MAX_THREADS;
    return (int)n;
}

/* 把"谁负责哪几行"画成两侧的色条,并画底部耗时对比条 */
static void draw_overlays(framebuffer_t *fb, int thread_count,
                          double single_ms, double multi_ms)
{
    /* 左侧窄条:整列同一颜色 = 一个线程负责全部 */
    for (int j = 0; j < H; j++)
        for (int i = 0; i < 8; i++)
            fb_set_pixel(fb, i, j, thread_colors[0]);

    /* 右侧窄条:每个线程一行一色,可以直观看到行带划分 */
    int y = 0;
    int rows_per_thread = H / thread_count;
    int extra = H % thread_count;
    for (int t = 0; t < thread_count; t++)
    {
        int rows = rows_per_thread + (t < extra ? 1 : 0);
        pixel_color_t c = thread_colors[t % THREAD_COLOR_COUNT];
        for (int r = 0; r < rows; r++, y++)
            for (int i = W - 8; i < W; i++)
                fb_set_pixel(fb, i, y, c);
    }

    /* 左右分界线,以及每个线程行带之间的分隔线 */
    for (int j = 0; j < H; j++)
        fb_set_pixel(fb, MID_X, j, COLOR_WHITE);

    y = 0;
    for (int t = 0; t < thread_count; t++)
    {
        int rows = rows_per_thread + (t < extra ? 1 : 0);
        if (t > 0)
            for (int i = MID_X; i < W; i++)
                fb_set_pixel(fb, i, y, COLOR_WHITE);
        y += rows;
    }

    /* 底部两条耗时条:长度按"多线程时间 / 单线程时间"缩放 */
    const int bar_y = H - 30;
    const int bar_h = 16;
    const int bar_max = MID_X - 24;
    int single_w = bar_max;
    int multi_w = (int)((double)bar_max * multi_ms / (single_ms > 0 ? single_ms : 1.0));
    if (multi_w < 1)
        multi_w = 1;
    if (multi_w > W - MID_X - 8)
        multi_w = W - MID_X - 8;

    for (int j = bar_y; j < bar_y + bar_h; j++)
    {
        for (int i = 0; i < single_w; i++)
            fb_set_pixel(fb, i, j, thread_colors[0]);
        for (int i = MID_X; i < MID_X + multi_w; i++)
            fb_set_pixel(fb, i, j, thread_colors[1]);
    }
}

int lesson10_render(framebuffer_t *fb, int frame_index, int frame_count)
{
    (void)frame_index; /* 静态课程:只渲染一帧 */
    (void)frame_count;
    fb_clear(fb);

    /* 相机和观察矩阵与第 8 课相同 */
    vec3 eye = {0.0f, 1.2f, 5.0f};
    vec3 center = {0.0f, 0.0f, -1.0f};
    vec3 up = {0.0f, 1.0f, 0.0f};
    mat4 view, view_t;
    look_at_4x4(view, eye, center, up);
    matrix_transpose_4x4(view_t, view);

    /*
    左半:单线程,直接在一个线程里渲染整个区域.
    时间用墙钟量,因为 clock() 会把所有线程的 CPU 时间加起来,
    那样多线程看起来反而更慢.
    */
    double t0 = wall_seconds();
    render_job_t single_job = {0};
    single_job.fb = fb;
    single_job.x0 = 0;
    single_job.x1 = MID_X;
    single_job.y0 = 0;
    single_job.y1 = H;
    for (int r = 0; r < 4; r++)
        for (int c = 0; c < 4; c++)
            single_job.view_t[r][c] = view_t[r][c];
    single_job.eye = eye;
    render_rows(&single_job);
    double single_ms = (wall_seconds() - t0) * 1000.0;

    /*
    右半:多线程.按行切成 thread_count 块,每块一个线程.
    线程只写自己那块行,y 范围不重叠,所以不需要加锁.
    */
    int thread_count = resolve_thread_count();
    pthread_t threads[MAX_THREADS];
    render_job_t jobs[MAX_THREADS];
    int spawned[MAX_THREADS] = {0};

    int rows_per_thread = H / thread_count;
    int extra = H % thread_count;
    int y = 0;

    double t1 = wall_seconds();
    for (int t = 0; t < thread_count; t++)
    {
        int rows = rows_per_thread + (t < extra ? 1 : 0);
        jobs[t].fb = fb;
        jobs[t].x0 = MID_X;
        jobs[t].x1 = W;
        jobs[t].y0 = y;
        jobs[t].y1 = y + rows;
        for (int r = 0; r < 4; r++)
            for (int c = 0; c < 4; c++)
                jobs[t].view_t[r][c] = view_t[r][c];
        jobs[t].eye = eye;
        y += rows;

        if (pthread_create(&threads[t], NULL, render_rows, &jobs[t]) == 0)
            spawned[t] = 1;
        else
            render_rows(&jobs[t]); /* 线程创建失败时退化成主线程直接渲染 */
    }

    for (int t = 0; t < thread_count; t++)
        if (spawned[t])
            pthread_join(threads[t], NULL);
    double multi_ms = (wall_seconds() - t1) * 1000.0;

    fprintf(stderr,
            "[lesson10] 左半单线程 %.2f ms | 右半 %d 线程 %.2f ms | 加速 %.2fx\n",
            single_ms, thread_count, multi_ms,
            multi_ms > 0 ? single_ms / multi_ms : 0.0);

    draw_overlays(fb, thread_count, single_ms, multi_ms);
    return 0;
}
