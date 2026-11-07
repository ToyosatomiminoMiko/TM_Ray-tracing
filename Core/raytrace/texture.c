#include "texture.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include "png.h"

/*
读取一个十进制整数,自动跳过空白和 # 注释.
PPM 头部允许在宽/高/最大色阶之间夹注释,不能只用一次 fscanf 读干净.
成功返回 0,读到文件尾返回 -1.
*/
static int read_int(FILE *fp, int *out)
{
    int c;
    for (;;)
    {
        c = fgetc(fp);
        if (c == '#')
        {
            /* 注释:吃掉一直到行尾 */
            while ((c = fgetc(fp)) != '\n' && c != EOF)
                ;
            continue;
        }
        if (c == ' ' || c == '\t' || c == '\r' || c == '\n')
            continue;
        break;
    }
    if (c == EOF)
        return -1;

    int value = 0;
    while (c >= '0' && c <= '9')
    {
        value = value * 10 + (c - '0');
        c = fgetc(fp);
    }
    *out = value;
    return 0;
}

/* 把 0..maxval 的颜色值缩放到 0..255 并夹紧 */
static uint8_t scale_channel(int value, int maxval)
{
    if (value < 0)
        value = 0;
    if (value > maxval)
        value = maxval;
    return (uint8_t)((value * 255) / maxval);
}

/* 加载失败时统一的收尾:释放像素并清零贴图信息,返回 -1 */
static int texture_fail(Texture *tex)
{
    free(tex->pixels);
    tex->pixels = NULL;
    tex->width = 0;
    tex->height = 0;
    return -1;
}

int texture_load_ppm(Texture *tex, const char *path)
{
    tex->pixels = NULL;
    tex->width = 0;
    tex->height = 0;

    FILE *fp = fopen(path, "rb");
    if (!fp)
        return -1;

    /* 文件头必须是 P3(文本)或 P6(二进制) */
    int magic1 = fgetc(fp);
    int magic2 = fgetc(fp);
    if (magic1 != 'P' || (magic2 != '3' && magic2 != '6'))
    {
        fclose(fp);
        return texture_fail(tex);
    }

    int width, height, maxval;
    if (read_int(fp, &width) != 0 ||
        read_int(fp, &height) != 0 ||
        read_int(fp, &maxval) != 0 ||
        width <= 0 || height <= 0 || maxval <= 0 || maxval > 65535)
    {
        fclose(fp);
        return texture_fail(tex);
    }

    tex->width = width;
    tex->height = height;

    size_t pixel_count = (size_t)width * (size_t)height;
    /* 统一按 RGBA 存放:PPM 只有 RGB,alpha 通道稍后统一填 255 */
    tex->pixels = (uint8_t *)malloc(pixel_count * 4);
    if (!tex->pixels)
    {
        fclose(fp);
        return texture_fail(tex);
    }

    if (magic2 == '3')
    {
        /* P3 文本:每个像素三个十进制数,读到数量不足就算失败 */
        for (size_t i = 0; i < pixel_count; i++)
        {
            for (int ch = 0; ch < 3; ch++)
            {
                int value;
                if (read_int(fp, &value) != 0)
                {
                    fclose(fp);
                    return texture_fail(tex);
                }
                tex->pixels[i * 4 + ch] = scale_channel(value, maxval);
            }
        }
    }
    else
    {
        /* P6 二进制:先按文件里的每像素 3 字节读进临时缓冲,再展开成 RGBA */
        uint8_t *rgb = (uint8_t *)malloc(pixel_count * 3);
        if (!rgb)
        {
            fclose(fp);
            return texture_fail(tex);
        }

        if (maxval <= 255)
        {
            /* 每通道 1 字节:先原样读入,最大色阶不是 255 时逐字节缩放 */
            size_t bytes = pixel_count * 3;
            if (fread(rgb, 1, bytes, fp) != bytes)
            {
                free(rgb);
                fclose(fp);
                return texture_fail(tex);
            }
            if (maxval != 255)
            {
                for (size_t i = 0; i < bytes; i++)
                    rgb[i] = scale_channel(rgb[i], maxval);
            }
        }
        else
        {
            /* 最大色阶大于 255:每通道 2 字节,大端存储,边读边缩放到 0..255 */
            for (size_t i = 0; i < pixel_count; i++)
            {
                for (int ch = 0; ch < 3; ch++)
                {
                    int hi = fgetc(fp);
                    int lo = fgetc(fp);
                    if (hi == EOF || lo == EOF)
                    {
                        free(rgb);
                        fclose(fp);
                        return texture_fail(tex);
                    }
                    int value = (hi << 8) | lo;
                    rgb[i * 3 + ch] = scale_channel(value, maxval);
                }
            }
        }

        /* 把 3 通道临时缓冲展开成 RGBA 布局,alpha 稍后统一填 255 */
        for (size_t i = 0; i < pixel_count; i++)
        {
            tex->pixels[i * 4 + 0] = rgb[i * 3 + 0];
            tex->pixels[i * 4 + 1] = rgb[i * 3 + 1];
            tex->pixels[i * 4 + 2] = rgb[i * 3 + 2];
        }
        free(rgb);
    }

    /* PPM 没有 alpha 通道:一律当作完全不透明 */
    for (size_t i = 0; i < pixel_count; i++)
        tex->pixels[i * 4 + 3] = 255;

    fclose(fp);
    return 0;
}

int texture_load_png(Texture *tex, const char *path)
{
    tex->pixels = NULL;
    tex->width = 0;
    tex->height = 0;

    PngImage img;
    if (png_load(path, &img) != 0)
        return -1; /* png_load 失败时已经把 img 清零,这里没有内存要释放 */

    /* 解码结果就是 RGBA8,直接接管像素所有权,不再复制一次 */
    tex->width = img.width;
    tex->height = img.height;
    tex->pixels = img.pixels;
    return 0;
}

void texture_free(Texture *tex)
{
    free(tex->pixels);
    tex->pixels = NULL;
    tex->width = 0;
    tex->height = 0;
}

/* 取第 (x,y) 个像素的 RGB,转换成 0..1 浮点颜色(忽略 alpha) */
static vec3 texel(const Texture *tex, int x, int y)
{
    const uint8_t *p = tex->pixels + ((size_t)y * (size_t)tex->width + (size_t)x) * 4;
    return (vec3){p[0] / 255.0f, p[1] / 255.0f, p[2] / 255.0f};
}

/* 取第 (x,y) 个像素的 alpha,转换成 0..1 */
static float texel_alpha(const Texture *tex, int x, int y)
{
    return tex->pixels[((size_t)y * (size_t)tex->width + (size_t)x) * 4 + 3] / 255.0f;
}

/* 把 UV 换算成最近邻像素下标:u 循环包裹,v 夹紧到 [0,1] */
static void nearest_texel_index(const Texture *tex, float u, float v, int *out_x, int *out_y)
{
    u -= floorf(u); /* u 循环:等距圆柱地图左右边缘本来就该拼起来 */
    if (v < 0.0f)
        v = 0.0f;
    if (v > 1.0f)
        v = 1.0f;

    int x = (int)(u * (float)tex->width);
    int y = (int)(v * (float)tex->height);
    if (x >= tex->width)
        x = tex->width - 1;
    if (y >= tex->height)
        y = tex->height - 1;
    *out_x = x;
    *out_y = y;
}

vec3 texture_sample_nearest(const Texture *tex, float u, float v)
{
    if (!tex || !tex->pixels || tex->width <= 0 || tex->height <= 0)
        return (vec3){1.0f, 0.0f, 1.0f}; /* 品红:一眼看出贴图加载失败 */

    int x, y;
    nearest_texel_index(tex, u, v, &x, &y);
    return texel(tex, x, y);
}

vec3 texture_sample_rgba_nearest(const Texture *tex, float u, float v, float *out_alpha)
{
    if (!tex || !tex->pixels || tex->width <= 0 || tex->height <= 0)
    {
        if (out_alpha)
            *out_alpha = 1.0f;
        return (vec3){1.0f, 0.0f, 1.0f}; /* 品红:一眼看出贴图加载失败 */
    }

    int x, y;
    nearest_texel_index(tex, u, v, &x, &y);
    if (out_alpha)
        *out_alpha = texel_alpha(tex, x, y);
    return texel(tex, x, y);
}

vec3 texture_sample(const Texture *tex, float u, float v)
{
    if (!tex || !tex->pixels || tex->width <= 0 || tex->height <= 0)
        return (vec3){1.0f, 0.0f, 1.0f}; /* 品红:一眼看出贴图加载失败 */

    u -= floorf(u);
    if (v < 0.0f)
        v = 0.0f;
    if (v > 1.0f)
        v = 1.0f;

    /*
    像素 i 的中心在 u = (i + 0.5) / width,所以先减 0.5 再取整:
    x0 是 UV 左边最近的像素,x1 是右边相邻的像素.
    */
    float xf = u * (float)tex->width - 0.5f;
    float yf = v * (float)tex->height - 0.5f;
    int x0 = (int)floorf(xf);
    int y0 = (int)floorf(yf);
    float fx = xf - (float)x0; /* 插值权重:离 x0 越近,0 越靠近它 */
    float fy = yf - (float)y0;

    if (x0 < 0)
        x0 += tex->width; /* u 方向循环:最左边再往左就是最右边 */
    int x1 = x0 + 1;
    if (x1 >= tex->width)
        x1 = 0;

    int y1;
    if (y0 < 0)
    {
        /* v 方向夹紧:顶行之外没有像素,直接取第一行 */
        y0 = 0;
        y1 = 0;
        fy = 0.0f;
    }
    else
    {
        y1 = y0 + 1;
        if (y1 >= tex->height)
            y1 = tex->height - 1; /* 底行同理 */
    }

    /* 两次水平插值,再垂直插值(双线性) */
    vec3 top = vec3_lerp(texel(tex, x0, y0), texel(tex, x1, y0), fx);
    vec3 bottom = vec3_lerp(texel(tex, x0, y1), texel(tex, x1, y1), fx);
    return vec3_lerp(top, bottom, fy);
}
