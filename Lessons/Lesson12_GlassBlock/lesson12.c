#include "lesson12.h"

#include <math.h>
#include <stdio.h>

#include "plane.h"
#include "ray.h"
#include "texture.h"

/*
第 12 课(支线):PNG 贴图 + Alpha 混合 -- 展示 Minecraft 风格的玻璃方块
扩展点:
1. 用 PNG 代替 PPM:PNG 带 alpha 通道,玻璃的透明区域就是 alpha=0 的像素;
2. 贴图映射到一个 3D 方块:6 个面各自求交,命中点换算成该面的 (u, v);
3. 透明渲染:把同一个像素上的多个面按距离从远到近做 alpha 混合;
4. 本课只做展示:光线穿过玻璃时方向不变,不做折射/反射(它们属于后续支线).

素材:png_reader/glass.png(16x16 RGBA,MC 玻璃材质;中间透明,最外圈是浅青色边框,四角附近还有几个不透明高光像素).
*/

#define PI_F 3.14159265358979323846f
#define DEG2RAD (PI_F / 180.0f)
#define FOV (36.0f * DEG2RAD) /* 垂直视场角:方块大约占画面高度的一半 */

/* 相机:位置 + 看向的点(方块中心),构成右手坐标系后再生成每个像素的光线 */
static const vec3 camera_eye = {2.2f, 1.5f, 1.8f};
static const vec3 camera_target = {0.0f, 0.0f, -3.4f};

/* 玻璃方块:轴对齐立方体,中心 + 半边长(边长 1.8) */
static const vec3 block_center = {0.0f, 0.0f, -3.4f};
static const float block_half = 0.9f;

/* 背景墙:无限大平面 z = WALL_Z,棋盘格能让人一眼看出玻璃哪些像素是透明的 */
static const float wall_z = -7.5f;

/* 平行光方向(使用前归一化):从右上前方照过来,让三个可见面亮度不同 */
static const vec3 sun_dir = {0.35f, 0.75f, 0.55f};

/* 贴图只加载一次,之后每帧复用;0=还没尝试,1=成功,-1=失败(回退到程序化颜色) */
static Texture glass_texture;
static int glass_texture_state = 0;

static void ensure_glass_texture(void)
{
    if (glass_texture_state != 0)
        return;

    /* ray.sh 会先切到仓库根目录;直接在 build/ 里运行时用 ../ 找 */
    const char *candidates[] = {"png_reader/glass.png", "../png_reader/glass.png"};
    for (int i = 0; i < 2; i++)
    {
        if (texture_load_png(&glass_texture, candidates[i]) == 0)
        {
            fprintf(stderr, "[lesson12] 已加载 PNG 贴图 %s (%dx%d)\n",
                    candidates[i], glass_texture.width, glass_texture.height);
            glass_texture_state = 1;
            return;
        }
    }

    fprintf(stderr, "[lesson12] 找不到 png_reader/glass.png,回退到程序化玻璃色\n");
    glass_texture_state = -1;
}

/*
采样玻璃贴图,返回 RGB 和 alpha.
alpha 是"这个像素有多不透明":255 = 完全不透明(边框),
0 = 完全透明(玻璃中间);混合公式见 lesson12_render 里的说明.
贴图加载失败时用程序化配色:边缘一圈不透明,内部透明.
*/
static vec3 sample_glass(float u, float v, float *out_alpha)
{
    if (glass_texture_state > 0)
        return texture_sample_rgba_nearest(&glass_texture, u, v, out_alpha);

    /* 16x16 玻璃贴图的边框占最外一圈像素,即 UV 的 1/16 宽度 */
    float border = 1.0f / 16.0f;
    int on_edge = (u < border || u > 1.0f - border ||
                   v < border || v > 1.0f - border);
    *out_alpha = on_edge ? 1.0f : 0.0f;
    return (vec3){0.82f, 0.92f, 0.91f};
}

/* 按轴取分量/写分量:方块 6 个面的求交写成循环,需要按轴索引 x/y/z */
static float vec_comp(vec3 v, int axis)
{
    if (axis == 0)
        return v.x;
    if (axis == 1)
        return v.y;
    return v.z;
}

static void vec_set_comp(vec3 *v, int axis, float value)
{
    if (axis == 0)
        v->x = value;
    else if (axis == 1)
        v->y = value;
    else
        v->z = value;
}

/*
6 个面的定义:axis 是面法线所在的轴,side 是正负方向.
(+X,-X,+Y,-Y,+Z,-Z) 六个面各自对应一个矩形求交.
*/
static const int face_axis[6] = {0, 0, 1, 1, 2, 2};
static const int face_side[6] = {1, -1, 1, -1, 1, -1};

/* 一次玻璃面命中:距离,法线,该面的纹理坐标 */
typedef struct
{
    float t;
    vec3 normal;
    float u, v;
} GlassHit;

/*
方块贴图的 UV 约定(和图片方向一致:u 向右,v 向下):
每个面都从"站在面外看这个面"的视角定方向,这样贴图不会镜像.
例如 +Z 面(正对相机的正面):u 沿 +X,v 沿 -Y;
-X 面(从左边看):u 沿 +Z,所以 u = (P.z - lo.z) / size,等等.
*/
static void face_uv(int axis, int side, vec3 P, vec3 lo, vec3 hi, float *out_u, float *out_v)
{
    float size = hi.x - lo.x; /* 正方体,x/y/z 边长相同 */
    float u, v;

    if (axis == 0) /* 法线沿 X:面内横向是 Z,纵向是 Y */
    {
        u = (side > 0) ? (hi.z - P.z) : (P.z - lo.z);
        v = hi.y - P.y;
    }
    else if (axis == 1) /* 法线沿 Y:面内横向是 X,纵向是 Z */
    {
        u = P.x - lo.x;
        v = (side > 0) ? (P.z - lo.z) : (hi.z - P.z);
    }
    else /* 法线沿 Z:面内横向是 X,纵向是 Y */
    {
        u = (side > 0) ? (P.x - lo.x) : (hi.x - P.x);
        v = hi.y - P.y;
    }

    *out_u = u / size;
    *out_v = v / size;
}

/*
与轴对齐方块求交:对 6 个面逐一当平面求交,再检查交点是否落在该面的矩形内.
返回命中面数量(一条射线最多穿过 6 个面里的 2 个);hits 按 t 从小到大排序,
渲染透明方块时要从远到近混合,所以排序是必要的.
*/
static int intersect_glass_block(Ray ray, vec3 center, float half, GlassHit hits[6])
{
    vec3 lo = vec3_sub(center, (vec3){half, half, half});
    vec3 hi = vec3_add(center, (vec3){half, half, half});

    int count = 0;
    for (int f = 0; f < 6; f++)
    {
        int axis = face_axis[f];
        int side = face_side[f];

        /* 面所在平面:该轴坐标 = 中心的坐标 ± 半边长 */
        float plane = vec_comp(center, axis) + (float)side * half;
        float denom = vec_comp(ray.dir, axis);
        if (fabsf(denom) < 1e-6f)
            continue; /* 光线平行于这个面 */

        float t = (plane - vec_comp(ray.origin, axis)) / denom;
        if (t <= 1e-4f)
            continue; /* 交点在相机背后(或太近) */

        vec3 P = ray_at(ray, t);
        /* 另外两个轴上的坐标必须落在方块范围内,否则交点在该面之外 */
        int a1 = (axis + 1) % 3;
        int a2 = (axis + 2) % 3;
        if (fabsf(vec_comp(P, a1) - vec_comp(center, a1)) > half + 1e-5f ||
            fabsf(vec_comp(P, a2) - vec_comp(center, a2)) > half + 1e-5f)
            continue;

        /* 面法线:单位向量,方向由 side 决定 */
        vec3 N = {0.0f, 0.0f, 0.0f};
        vec_set_comp(&N, axis, (float)side);

        float u, v;
        face_uv(axis, side, P, lo, hi, &u, &v);
        hits[count].t = t;
        hits[count].normal = N;
        hits[count].u = u;
        hits[count].v = v;
        count++;
    }

    /* 插入排序:命中面最多 6 个,数量小,插入排序最简单 */
    for (int i = 1; i < count; i++)
    {
        GlassHit key = hits[i];
        int j = i - 1;
        while (j >= 0 && hits[j].t > key.t)
        {
            hits[j + 1] = hits[j];
            j--;
        }
        hits[j + 1] = key;
    }
    return count;
}

/*
玻璃面的明暗:只做简单的平行光漫反射,不加高光/环境反射.
目的是区分 6 个面(正面/侧面/顶面亮度不同),不是模拟真实玻璃.
*/
static float face_brightness(vec3 N)
{
    float ndotl = vec3_dot(N, vec3_normalize(sun_dir));
    if (ndotl < 0.0f)
        ndotl = 0.0f;
    return 0.55f + 0.45f * ndotl;
}

/*
背景墙:解析求交 z = wall_z 的平面,命中点换算成世界坐标的棋盘格.
棋盘格是透明的"参照物":透过玻璃中间能看到背景,说明那些像素 alpha=0.
*/
static vec3 shade_backdrop(Ray ray)
{
    float t = intersect_plane(ray, (vec3){0.0f, 0.0f, 1.0f}, (vec3){0.0f, 0.0f, wall_z});
    if (isnan(t))
    {
        /* 理论上看不到这里(相机朝 -Z,所有视线都会打到墙),留一个天空色兜底 */
        float k = 0.5f * (ray.dir.y + 1.0f);
        vec3 low = {0.55f, 0.78f, 0.86f};
        vec3 high = {0.86f, 0.95f, 0.99f};
        return vec3_lerp(low, high, k);
    }

    vec3 P = ray_at(ray, t);

    /* 1 米一格的大棋盘;奇偶格两种颜色,方便观察透明区域 */
    int cell_x = (int)floorf(P.x / 1.0f);
    int cell_y = (int)floorf(P.y / 1.0f);
    vec3 dark = {0.34f, 0.38f, 0.45f};
    vec3 pale = {0.60f, 0.64f, 0.70f};
    vec3 base = ((cell_x + cell_y) & 1) ? dark : pale;

    /* 背景越靠上越亮,给平面加一点"摄影棚打光"的层次 */
    float level = P.y / 4.0f;
    if (level < -1.0f)
        level = -1.0f;
    if (level > 1.0f)
        level = 1.0f;
    float shade = 0.72f + 0.28f * level;
    return vec3_scale(base, shade);
}

int lesson12_render(framebuffer_t *fb, int frame_index, int frame_count)
{
    (void)frame_index; /* 静态课程:只渲染一帧 */
    (void)frame_count;

    ensure_glass_texture();

    /*
    相机坐标系(和轨道相机课同一套):
    forward 指向视线方向,right 是屏幕右方,up 是屏幕上方的基向量.
    right = normalize(forward × 世界向上),up = right × forward.
    */
    vec3 forward = vec3_normalize(vec3_sub(camera_target, camera_eye));
    vec3 right = vec3_normalize(vec3_cross(forward, (vec3){0.0f, 1.0f, 0.0f}));
    vec3 up = vec3_cross(right, forward);

    float half_h = tanf(FOV * 0.5f);
    float half_w = half_h * ((float)W / (float)H);

    for (int j = 0; j < H; j++)
    {
        for (int i = 0; i < W; i++)
        {
            /* 像素中心 -> NDC [-1,1];图像坐标 y 向下,所以 ndc_y 要取反 */
            float ndc_x = 2.0f * ((float)i + 0.5f) / (float)W - 1.0f;
            float ndc_y = 1.0f - 2.0f * ((float)j + 0.5f) / (float)H;

            vec3 dir = vec3_add(
                forward,
                vec3_add(
                    vec3_scale(right, ndc_x * half_w),
                    vec3_scale(up, ndc_y * half_h)));
            dir = vec3_normalize(dir);

            Ray ray = {camera_eye, dir};

            /* 1. 先算背景:玻璃透明的地方露出来的就是这层颜色 */
            vec3 color = shade_backdrop(ray);

            /* 2. 找出这条射线穿过方块的所有面 */
            GlassHit hits[6];
            int hit_count = intersect_glass_block(ray, block_center, block_half, hits);

            /*
            3. 从远到近做 alpha 混合:
               C = src.rgb * alpha + dst * (1 - alpha)
               alpha=1 完全遮住背后;alpha=0 完全不遮挡.
               因为先画远的再画近的,近处的透明像素才不会错误地挡住远处面.
            */
            for (int k = hit_count - 1; k >= 0; k--)
            {
                float alpha;
                vec3 rgb = sample_glass(hits[k].u, hits[k].v, &alpha);
                float brightness = face_brightness(hits[k].normal);
                vec3 lit = vec3_scale(rgb, brightness);

                color = vec3_add(
                    vec3_scale(lit, alpha),
                    vec3_scale(color, 1.0f - alpha));
            }

            fb_set_pixel(fb, i, j, color_from_01(color.x, color.y, color.z));
        }
    }
    return 0;
}
