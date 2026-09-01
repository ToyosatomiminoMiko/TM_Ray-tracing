#include "lesson11.h"

#include <math.h>
#include <stdio.h>

#include "ray.h"
#include "sphere.h"
#include "texture.h"

/*
第 11 课(支线):贴图与材质 -- 世界地图做成旋转地球
扩展点:
1. 从 PPM 文件加载贴图(Texture),把"固定颜色"换成"UV 采样颜色";
2. 球面 UV 映射:等距圆柱投影,把经纬度换算成 (u, v);
3. 材质(Material):贴图只负责漫反射颜色,高光/环境光/大气光由材质参数控制;
4. 动画:地球自转 = 采样时让 u 随时间滚动,几何体不用旋转.

素材:仓库根目录的 world.ppm(1275x638,等距圆柱世界地图).
*/

#define PI_F 3.14159265358979323846f
#define DEG2RAD (PI_F / 180.0f)
#define FOV (42.0f * DEG2RAD) /* 垂直视场角:地球大约占画面高度的八成 */

static const vec3 earth_center = {0.0f, 0.0f, -3.2f};
static const float earth_radius = 1.0f;

/*
太阳光用平行光:太阳离地球极远,在场景尺度下所有光线方向相同.
凸球面被平行光照时也不会自遮挡,所以这里不需要阴影射线.
*/
static const vec3 sun_dir = {0.42f, 0.62f, 0.66f}; /* 使用前会归一化 */

/*
材质 = 贴图 + 光照参数.
贴图提供每个点的"基础颜色",材质决定这个颜色怎么被光改变.
*/
typedef struct
{
    const Texture *diffuse_map; /* 漫反射贴图:命中点的颜色从这里采样 */
    vec3 specular;              /* 高光颜色(带一点蓝,更像水面/云层反光) */
    float shininess;            /* 反光度:越大高光越小越亮 */
    float ambient;              /* 环境光比例:暗面不至于全黑 */
    float atmosphere;           /* 大气边缘光强度(简化近似,见 shade_earth) */
} Material;

static Material earth_material = {
    NULL,                  /* 贴图在 ensure_texture() 里加载后填入 */
    {0.55f, 0.65f, 0.85f},
    96.0f,
    0.05f,
    0.35f,
};

static Texture earth_texture;
static int texture_ready = 0; /* 0=还没尝试加载,1=已经尝试过(成功或失败) */

/* 贴图只加载一次,之后每帧复用;失败时给出提示并回退到程序化配色 */
static int ensure_texture(void)
{
    if (texture_ready)
        return earth_material.diffuse_map != NULL;

    const char *candidates[] = {"world.ppm", "../world.ppm"};
    for (int i = 0; i < 2; i++)
    {
        if (texture_load_ppm(&earth_texture, candidates[i]) == 0)
        {
            earth_material.diffuse_map = &earth_texture;
            fprintf(stderr, "[lesson11] 已加载贴图 %s (%dx%d)\n",
                    candidates[i], earth_texture.width, earth_texture.height);
            texture_ready = 1;
            return 1;
        }
    }

    fprintf(stderr, "[lesson11] 找不到 world.ppm,回退到程序化配色\n");
    texture_ready = 1;
    return 0;
}

/* 贴图缺失时的回退配色:蓝色海洋 + 绿色陆地斑块 */
static vec3 fallback_diffuse(float u, float v)
{
    float land = sinf(u * 7.0f) * sinf(v * 9.0f) +
                 sinf(u * 13.0f) * sinf(v * 5.0f);
    vec3 ocean = {0.05f, 0.18f, 0.42f};
    vec3 landc = {0.12f, 0.45f, 0.15f};
    float t = 0.5f + 0.5f * land;
    return vec3_lerp(ocean, landc, t * 0.65f);
}

/*
星空背景:把光线方向量化成小格子,再用格子坐标做哈希.
同一个方向永远得到同一个哈希 -> 每一帧星星位置不变,画面不闪烁.
*/
static uint32_t hash3(int x, int y, int z)
{
    uint32_t h = (uint32_t)(x * 73856093) ^
                 (uint32_t)(y * 19349663) ^
                 (uint32_t)(z * 83492791);
    h ^= h >> 13;
    h *= 0x5bd1e995u;
    h ^= h >> 15;
    return h;
}

static vec3 background(vec3 dir)
{
    /* 深空底色,几乎全黑 */
    vec3 color = {0.01f, 0.012f, 0.025f};

    int gx = (int)floorf(dir.x * 1024.0f);
    int gy = (int)floorf(dir.y * 1024.0f);
    int gz = (int)floorf(dir.z * 1024.0f);
    uint32_t h = hash3(gx, gy, gz);

    /* 约 1/8192 的格子有一颗星,星星亮度由哈希的高位决定 */
    if ((h & 0x1FFFu) == 0u)
    {
        float bright = 0.35f + ((h >> 16) & 0xFFu) / 255.0f * 0.65f;
        color.x += bright;
        color.y += bright;
        color.z += bright * 0.85f; /* 星星略偏蓝白 */
    }
    return color;
}

/*
地球材质着色:贴图取漫反射颜色,再叠平行光漫反射 + Blinn-Phong 高光 +
大气边缘光(菲涅尔近似:视线越贴地表,看到的散射蓝光越强).
*/
static vec3 shade_earth(vec3 P, vec3 N, vec3 view_dir, float u, float v)
{
    /* 1. 漫反射颜色:贴图采样(这就是"贴图"替代固定 albedo 的地方) */
    vec3 diffuse = earth_material.diffuse_map
                       ? texture_sample(earth_material.diffuse_map, u, v)
                       : fallback_diffuse(u, v);

    /* 2. 平行光漫反射:亮度 = max(N·L, 0),法线越朝向太阳越亮 */
    vec3 L = vec3_normalize(sun_dir);
    float ndotl = vec3_dot(N, L);
    if (ndotl < 0.0f)
        ndotl = 0.0f;
    float brightness = earth_material.ambient +
                       (1.0f - earth_material.ambient) * ndotl;

    /* 3. Blinn-Phong 高光:H = normalize(L+V),高光 = pow(N·H, 反光度) */
    vec3 V = vec3_normalize(vec3_scale(view_dir, -1.0f)); /* 命中点指向相机 */
    vec3 half = vec3_normalize(vec3_add(L, V));
    float spec = 0.0f;
    if (ndotl > 0.0f)
    {
        float ndoth = vec3_dot(N, half);
        if (ndoth > 0.0f)
            spec = powf(ndoth, earth_material.shininess);
    }

    vec3 color = vec3_add(
        vec3_scale(diffuse, brightness),
        vec3_scale(earth_material.specular, spec));

    /* 4. 大气边缘光:视线擦过地表时 N·V 接近 0,叠一层蓝光(简化近似) */
    float ndotv = vec3_dot(N, V);
    if (ndotv < 0.0f)
        ndotv = 0.0f;
    float rim = powf(1.0f - ndotv, 3.0f) * earth_material.atmosphere;
    color = vec3_add(color, (vec3){0.25f * rim, 0.45f * rim, 0.90f * rim});

    return color;
}

int lesson11_render(framebuffer_t *fb, int frame_index, int frame_count)
{
    ensure_texture();

    Sphere earth = {earth_center, earth_radius};
    vec3 origin = {0.0f, 0.0f, 0.0f};

    /* 一整个动画循环转一圈,进度 = 已转圈数(0..1) */
    float progress = (float)frame_index / (float)frame_count;

    for (int j = 0; j < H; j++)
    {
        for (int i = 0; i < W; i++)
        {
            vec3 dir = ray_direction_fov(i, j, W, H, FOV);
            Ray ray = {origin, dir};

            float t = intersect_sphere_fast(ray, earth);
            vec3 color;
            if (t > 0.0f)
            {
                vec3 P = ray_at(ray, t);
                vec3 N = sphere_normal_at(earth, P);

                /* 经纬度 -> UV;地球自西向东自转,采样 u 随时间减小 */
                float u, v;
                sphere_uv_at(earth, P, &u, &v);
                u -= progress;

                color = shade_earth(P, N, dir, u, v);
            }
            else
            {
                color = background(dir);
            }

            fb_set_pixel(fb, i, j, color_from_01(color.x, color.y, color.z));
        }
    }
    return 0;
}
