#include "png.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <zlib.h>

/*
PNG 文件固定以这 8 个字节开头:
0x89 'P' 'N' 'G' 0x0D 0x0A 0x1A 0x0A.
用它们先和 PPM/BMP 等格式区分开,签名不对就没必要继续解析了.
*/
static const uint8_t PNG_SIGNATURE[8] = {0x89, 'P', 'N', 'G', '\r', '\n', 0x1A, '\n'};

/* PNG 规范里的颜色类型取值,其余数字都是坏文件 */
#define PNG_COLOR_TYPE_GRAY 0       /* 灰度 */
#define PNG_COLOR_TYPE_RGB 2        /* 彩色 */
#define PNG_COLOR_TYPE_PALETTE 3    /* 调色板 */
#define PNG_COLOR_TYPE_GRAY_ALPHA 4 /* 灰度 + alpha */
#define PNG_COLOR_TYPE_RGBA 6       /* 彩色 + alpha */

/*
读大端 32 位整数.
PNG 的 chunk 长度和 CRC 都按大端(big-endian)存,和 x86 的内存顺序相反.
*/
static uint32_t read_be32(const uint8_t *p)
{
    return ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) |
           ((uint32_t)p[2] << 8) | (uint32_t)p[3];
}

/* chunk 类型就是 4 个 ASCII 字符(如 "IHDR"),直接按 4 字节比较 */
static int chunk_is(const uint8_t *type, const char *name)
{
    return memcmp(type, name, 4) == 0;
}

/*
一次读入整个文件.
PNG 必须从文件尾定长读取,而且 IDAT 可能分成多块,整读进内存最好处理;
成功返回 0,数据用 malloc 分配,调用方负责 free.
*/
static int read_whole_file(const char *path, uint8_t **out_data, size_t *out_size)
{
    FILE *fp;
    long file_size;
    uint8_t *data;

    *out_data = NULL;
    *out_size = 0;

    fp = fopen(path, "rb");
    if (!fp)
    {
        fprintf(stderr, "png_load: 无法打开文件 %s\n", path);
        return -1;
    }

    if (fseek(fp, 0, SEEK_END) != 0)
    {
        fprintf(stderr, "png_load: 无法定位文件尾 %s\n", path);
        fclose(fp);
        return -1;
    }
    file_size = ftell(fp);
    if (file_size < 0)
    {
        fprintf(stderr, "png_load: 无法获取文件大小 %s\n", path);
        fclose(fp);
        return -1;
    }
    if (file_size < 8)
    {
        fprintf(stderr, "png_load: %s 太短,不像 PNG 文件\n", path);
        fclose(fp);
        return -1;
    }

    rewind(fp);
    data = (uint8_t *)malloc((size_t)file_size);
    if (!data)
    {
        fprintf(stderr, "png_load: 内存不足,无法读入 %s\n", path);
        fclose(fp);
        return -1;
    }
    if (fread(data, 1, (size_t)file_size, fp) != (size_t)file_size)
    {
        fprintf(stderr, "png_load: 读取 %s 失败\n", path);
        free(data);
        fclose(fp);
        return -1;
    }
    fclose(fp);

    *out_data = data;
    *out_size = (size_t)file_size;
    return 0;
}

/*
Paeth 预测器(PNG 规范 9.4).

记 a = 左邻, b = 上邻, c = 左上邻, p = a + b - c;
分别算 p 到 a/b/c 的距离 pa/pb/pc,取距离最小的那个当预测值,
距离相等时按 a -> b -> c 的优先级.
直观理解:当前像素的梯度更可能延续 a-b-c 里最接近的那个方向.
*/
static int paeth_predictor(int a, int b, int c)
{
    int p = a + b - c;
    int pa = abs(p - a);
    int pb = abs(p - b);
    int pc = abs(p - c);

    if (pa <= pb && pa <= pc)
        return a;
    if (pb <= pc)
        return b;
    return c;
}

/*
反滤波一行.

PNG 压缩前会先用"左边/上边/左上"预测当前字节,只把残差交给 zlib,
这样图片压缩率更高;解码时必须把预测值加回去.

row   : 本行数据(不含行首的滤波器字节),原地还原
prev  : 上一行还原后的数据,第一行传 NULL
rowbytes: 本行字节数
bpp   : 每像素字节数.Sub/Paeth 的左邻要往前跳 bpp 字节
filter: 行首记录的滤波器编号 0..4

公式(所有加法都对 256 取模,即只保留低 8 位):
0 None   : x
1 Sub    : x + left
2 Up     : x + up
3 Average: x + floor((left + up) / 2)
4 Paeth  : x + paeth(left, up, upleft)
第一列左边,第一行上方都按 0 处理.
*/
static void unfilter_row(uint8_t *row, const uint8_t *prev, size_t rowbytes,
                         int bpp, int filter)
{
    size_t i;

    switch (filter)
    {
    case 0:
        break;
    case 1:
        /* Sub:每个字节加上同像素左侧通道(相隔 bpp)的值 */
        for (i = (size_t)bpp; i < rowbytes; i++)
            row[i] = (uint8_t)(row[i] + row[i - bpp]);
        break;
    case 2:
        /* Up:加上正上方同一列的值 */
        if (prev)
        {
            for (i = 0; i < rowbytes; i++)
                row[i] = (uint8_t)(row[i] + prev[i]);
        }
        break;
    case 3:
        /* Average:左邻和上邻的平均值,越界的一侧算 0 */
        for (i = 0; i < rowbytes; i++)
        {
            int left = (i >= (size_t)bpp) ? row[i - bpp] : 0;
            int up = prev ? prev[i] : 0;
            row[i] = (uint8_t)(row[i] + ((left + up) / 2));
        }
        break;
    case 4:
        /* Paeth:最复杂也最常用,预测器本身见 paeth_predictor */
        for (i = 0; i < rowbytes; i++)
        {
            int left = (i >= (size_t)bpp) ? row[i - bpp] : 0;
            int up = prev ? prev[i] : 0;
            int upleft = (prev && i >= (size_t)bpp) ? prev[i - bpp] : 0;
            row[i] = (uint8_t)(row[i] + paeth_predictor(left, up, upleft));
        }
        break;
    default:
        /* 调用方已经检查过编号,这里不会走到 */
        break;
    }
}

/*
解析并解码整个 PNG,成功时把 RGBA8 写入 img.
file/file_size 是已经读进内存的完整文件,path 只用于错误信息.
返回 0 成功;返回 -1 失败,所有中间分配的内存都已释放,img 保持清零.
*/
static int decode_png(const char *path, const uint8_t *file, size_t file_size,
                      PngImage *img)
{
    /* IHDR 里的图像参数 */
    uint32_t width = 0;
    uint32_t height = 0;
    int bit_depth = 0;
    int color_type = -1;
    int bpp = 0; /* 文件内每像素字节数:位深 8 时等于通道数 */

    int seen_ihdr = 0;
    int seen_iend = 0;
    int seen_plte = 0;

    /* PLTE:最多 256 种颜色,每种 3 字节 RGB */
    uint8_t palette[256 * 3];
    int palette_count = 0;

    /* 调色板图片的 tRNS:每个调色板下标对应 1 字节 alpha */
    uint8_t trns_alpha[256];
    int trns_alpha_count = 0;

    /* 灰度/RGB 图片的 tRNS:一种"颜色键",样本等于它时完全透明 */
    int has_color_key = 0;
    uint32_t color_key_gray = 0;
    uint32_t color_key_rgb[3] = {0, 0, 0};

    uint8_t *idat = NULL; /* 所有 IDAT 数据拼接后的缓冲区 */
    size_t idat_len = 0;
    uint8_t *raw = NULL;  /* 解压 + 反滤波后的原始样本 */
    uint8_t *rgba = NULL; /* 最终 RGBA8 输出 */
    size_t rowbytes;
    size_t expected;
    size_t pos;
    uint32_t y;
    int ret = -1;

    /* 越过 8 字节签名,逐个 chunk 解析 */
    pos = 8;
    while (pos + 12 <= file_size)
    {
        uint32_t chunk_len = read_be32(file + pos);
        const uint8_t *type = file + pos + 4;
        const uint8_t *data = file + pos + 8;
        uint32_t stored_crc;
        uLong calc_crc;

        /* chunk 布局:长度(4) + 类型(4) + 数据(chunk_len) + CRC(4) */
        if (chunk_len > 0x7FFFFFFF)
        {
            fprintf(stderr, "png_load: %s 中存在非法 chunk 长度 %u\n",
                    path, (unsigned)chunk_len);
            goto cleanup;
        }
        if ((size_t)chunk_len > file_size - pos - 12)
        {
            fprintf(stderr, "png_load: %s 的 chunk 数据越界,文件被截断\n", path);
            goto cleanup;
        }

        /*
        CRC 覆盖"类型 + 数据",不含长度字段.
        用 zlib 的 crc32() 先校验,坏文件在解压前就能被发现;
        stored_crc 位于数据末尾,文件尾端可能被截断,所以上面的越界检查必须在前.
        */
        stored_crc = read_be32(data + chunk_len);
        calc_crc = crc32(0L, Z_NULL, 0);
        calc_crc = crc32(calc_crc, type, 4);
        calc_crc = crc32(calc_crc, data, (uInt)chunk_len);
        if (calc_crc != stored_crc)
        {
            fprintf(stderr, "png_load: %s 的 chunk %.4s CRC 校验失败\n",
                    path, (const char *)type);
            goto cleanup;
        }

        if (chunk_is(type, "IHDR"))
        {
            if (seen_ihdr)
            {
                fprintf(stderr, "png_load: %s 出现了多个 IHDR\n", path);
                goto cleanup;
            }
            if (pos != 8)
            {
                fprintf(stderr, "png_load: %s 的第一个 chunk 不是 IHDR\n", path);
                goto cleanup;
            }
            if (chunk_len != 13)
            {
                fprintf(stderr, "png_load: %s 的 IHDR 长度应为 13,实际 %u\n",
                        path, (unsigned)chunk_len);
                goto cleanup;
            }

            width = read_be32(data);
            height = read_be32(data + 4);
            bit_depth = data[8];
            color_type = data[9];
            seen_ihdr = 1;

            if (width == 0 || height == 0 || width > 0x7FFFFFFFu ||
                height > 0x7FFFFFFFu)
            {
                fprintf(stderr, "png_load: %s 的图像尺寸非法(%ux%u)\n",
                        path, (unsigned)width, (unsigned)height);
                goto cleanup;
            }
            if (bit_depth != 8)
            {
                /* 16 位样本每个通道 2 字节,本教学版不做缩放,直接明确拒绝 */
                fprintf(stderr, "png_load: %s 的位深为 %d,只支持 8 位\n",
                        path, bit_depth);
                goto cleanup;
            }
            if (data[10] != 0)
            {
                fprintf(stderr, "png_load: %s 使用了未知压缩方法 %d\n",
                        path, data[10]);
                goto cleanup;
            }
            if (data[11] != 0)
            {
                fprintf(stderr, "png_load: %s 使用了未知滤波方法 %d\n",
                        path, data[11]);
                goto cleanup;
            }
            if (data[12] != 0)
            {
                /* Adam7 隔行扫描要分 7 趟解码,本教学版不实现 */
                fprintf(stderr, "png_load: %s 是隔行 PNG(interlace=%d),不支持\n",
                        path, data[12]);
                goto cleanup;
            }

            switch (color_type)
            {
            case PNG_COLOR_TYPE_GRAY:
                bpp = 1;
                break;
            case PNG_COLOR_TYPE_RGB:
                bpp = 3;
                break;
            case PNG_COLOR_TYPE_PALETTE:
                bpp = 1; /* 调色板图每个像素存 1 字节下标 */
                break;
            case PNG_COLOR_TYPE_GRAY_ALPHA:
                bpp = 2;
                break;
            case PNG_COLOR_TYPE_RGBA:
                bpp = 4;
                break;
            default:
                fprintf(stderr, "png_load: %s 的颜色类型 %d 不支持\n",
                        path, color_type);
                goto cleanup;
            }
        }
        else if (chunk_is(type, "PLTE"))
        {
            if (chunk_len == 0 || chunk_len % 3 != 0 || chunk_len > 256 * 3)
            {
                fprintf(stderr, "png_load: %s 的 PLTE 长度 %u 非法\n",
                        path, (unsigned)chunk_len);
                goto cleanup;
            }
            palette_count = (int)(chunk_len / 3);
            memcpy(palette, data, chunk_len);
            seen_plte = 1;
        }
        else if (chunk_is(type, "tRNS"))
        {
            if (color_type == PNG_COLOR_TYPE_PALETTE)
            {
                if (chunk_len == 0 || chunk_len > 256)
                {
                    fprintf(stderr, "png_load: %s 的调色板 tRNS 长度 %u 非法\n",
                            path, (unsigned)chunk_len);
                    goto cleanup;
                }
                trns_alpha_count = (int)chunk_len;
                memcpy(trns_alpha, data, chunk_len);
            }
            else if (color_type == PNG_COLOR_TYPE_GRAY)
            {
                if (chunk_len != 2)
                {
                    fprintf(stderr, "png_load: %s 的灰度 tRNS 长度应为 2\n", path);
                    goto cleanup;
                }
                color_key_gray = ((uint32_t)data[0] << 8) | data[1];
                has_color_key = 1;
            }
            else if (color_type == PNG_COLOR_TYPE_RGB)
            {
                if (chunk_len != 6)
                {
                    fprintf(stderr, "png_load: %s 的 RGB tRNS 长度应为 6\n", path);
                    goto cleanup;
                }
                color_key_rgb[0] = ((uint32_t)data[0] << 8) | data[1];
                color_key_rgb[1] = ((uint32_t)data[2] << 8) | data[3];
                color_key_rgb[2] = ((uint32_t)data[4] << 8) | data[5];
                has_color_key = 1;
            }
            else
            {
                /* 灰度+alpha / RGBA 本身就有 alpha 通道,tRNS 不合法 */
                fprintf(stderr, "png_load: %s 的颜色类型 %d 不允许 tRNS\n",
                        path, color_type);
                goto cleanup;
            }
        }
        else if (chunk_is(type, "IDAT"))
        {
            if (!seen_ihdr)
            {
                fprintf(stderr, "png_load: %s 的 IDAT 出现在 IHDR 之前\n", path);
                goto cleanup;
            }
            if (!idat)
            {
                /*
                所有 IDAT 的数据总量一定小于整个文件大小,一次分配够,
                之后各块 IDAT 依次追加即可(必须拼接成一个 zlib 流再解压).
                */
                idat = (uint8_t *)malloc(file_size);
                if (!idat)
                {
                    fprintf(stderr, "png_load: 内存不足,无法保存 IDAT\n");
                    goto cleanup;
                }
            }
            memcpy(idat + idat_len, data, chunk_len);
            idat_len += chunk_len;
        }
        else if (chunk_is(type, "IEND"))
        {
            seen_iend = 1;
            break;
        }
        else
        {
            /* 其他 chunk(文本/时间/伽马等)解析图片本身用不到,直接跳过 */
        }

        pos += 12 + (size_t)chunk_len;
    }

    if (!seen_ihdr)
    {
        fprintf(stderr, "png_load: %s 缺少 IHDR\n", path);
        goto cleanup;
    }
    if (!seen_iend)
    {
        fprintf(stderr, "png_load: %s 缺少 IEND\n", path);
        goto cleanup;
    }
    if (idat_len == 0)
    {
        fprintf(stderr, "png_load: %s 没有 IDAT 数据\n", path);
        goto cleanup;
    }
    if (color_type == PNG_COLOR_TYPE_PALETTE && (!seen_plte || palette_count == 0))
    {
        fprintf(stderr, "png_load: %s 是调色板图但缺少 PLTE\n", path);
        goto cleanup;
    }

    /*
    解压后的原始数据布局:
    每行 = 1 字节滤波器编号 + width*bpp 字节样本,
    总大小 = height * (1 + width*bpp).
    先做溢出检查,避免坏文件里的超大尺寸把 size_t 乘爆.
    */
    rowbytes = (size_t)width * (size_t)bpp;
    if (rowbytes / (size_t)bpp != (size_t)width)
    {
        fprintf(stderr, "png_load: %s 的图像行宽溢出\n", path);
        goto cleanup;
    }
    expected = (rowbytes + 1) * (size_t)height;
    if (expected / (rowbytes + 1) != (size_t)height)
    {
        fprintf(stderr, "png_load: %s 的图像大小溢出\n", path);
        goto cleanup;
    }

    raw = (uint8_t *)malloc(expected);
    if (!raw)
    {
        fprintf(stderr, "png_load: 内存不足,无法解压 %s\n", path);
        goto cleanup;
    }

    /* IDAT 拼好后是一个 zlib 流,用 uncompress() 一次解压 */
    {
        uLongf dest_len = (uLongf)expected;
        int zret = uncompress(raw, &dest_len, idat, (uLong)idat_len);
        if (zret != Z_OK)
        {
            fprintf(stderr, "png_load: %s 的 IDAT 解压失败(zlib 错误码 %d)\n",
                    path, zret);
            goto cleanup;
        }
        if ((size_t)dest_len != expected)
        {
            fprintf(stderr, "png_load: %s 解压后大小不符(期望 %zu,实际 %zu)\n",
                    path, expected, (size_t)dest_len);
            goto cleanup;
        }
    }

    /* 逐扫描线反滤波,每行滤波器编号可以不同(编码器通常逐行自适应选择) */
    for (y = 0; y < height; y++)
    {
        size_t row_off = (size_t)y * (rowbytes + 1);
        int filter = raw[row_off];
        uint8_t *line = raw + row_off + 1;
        const uint8_t *prev = (y == 0)
                                  ? NULL
                                  : raw + (size_t)(y - 1) * (rowbytes + 1) + 1;

        if (filter < 0 || filter > 4)
        {
            fprintf(stderr, "png_load: %s 第 %u 行使用了未知滤波器 %d\n",
                    path, (unsigned)y, filter);
            goto cleanup;
        }
        unfilter_row(line, prev, rowbytes, bpp, filter);
    }

    /* 把反滤波后的样本统一转换成 RGBA8 */
    rgba = (uint8_t *)malloc((size_t)width * (size_t)height * 4);
    if (!rgba)
    {
        fprintf(stderr, "png_load: 内存不足,无法输出 %s\n", path);
        goto cleanup;
    }

    for (y = 0; y < height; y++)
    {
        const uint8_t *line = raw + (size_t)y * (rowbytes + 1) + 1;
        uint8_t *out = rgba + (size_t)y * (size_t)width * 4;
        uint32_t x;

        for (x = 0; x < width; x++)
        {
            uint8_t r;
            uint8_t g;
            uint8_t b;
            uint8_t a = 255;

            switch (color_type)
            {
            case PNG_COLOR_TYPE_GRAY:
                r = g = b = line[x];
                /* tRNS 颜色键:样本值和键相同就当作全透明 */
                if (has_color_key && (uint32_t)r == color_key_gray)
                    a = 0;
                break;
            case PNG_COLOR_TYPE_RGB:
                r = line[x * 3 + 0];
                g = line[x * 3 + 1];
                b = line[x * 3 + 2];
                if (has_color_key && (uint32_t)r == color_key_rgb[0] &&
                    (uint32_t)g == color_key_rgb[1] &&
                    (uint32_t)b == color_key_rgb[2])
                    a = 0;
                break;
            case PNG_COLOR_TYPE_PALETTE:
            {
                uint32_t idx = line[x];
                if ((int)idx >= palette_count)
                {
                    fprintf(stderr,
                            "png_load: %s 第 %u 行第 %u 列的调色板下标 %u 越界\n",
                            path, (unsigned)y, (unsigned)x, (unsigned)idx);
                    goto cleanup;
                }
                r = palette[idx * 3 + 0];
                g = palette[idx * 3 + 1];
                b = palette[idx * 3 + 2];
                if ((int)idx < trns_alpha_count)
                    a = trns_alpha[idx];
                break;
            }
            case PNG_COLOR_TYPE_GRAY_ALPHA:
                r = g = b = line[x * 2 + 0];
                a = line[x * 2 + 1];
                break;
            default: /* PNG_COLOR_TYPE_RGBA */
                r = line[x * 4 + 0];
                g = line[x * 4 + 1];
                b = line[x * 4 + 2];
                a = line[x * 4 + 3];
                break;
            }

            out[x * 4 + 0] = r;
            out[x * 4 + 1] = g;
            out[x * 4 + 2] = b;
            out[x * 4 + 3] = a;
        }
    }

    /* 所有权交给 img,并把局部指针清空,避免 cleanup 重复释放 */
    img->width = (int)width;
    img->height = (int)height;
    img->pixels = rgba;
    rgba = NULL;
    ret = 0;

cleanup:
    free(raw);
    free(idat);
    free(rgba);
    return ret;
}

int png_load(const char *path, PngImage *img)
{
    uint8_t *file_data = NULL;
    size_t file_size = 0;
    int ret;

    if (!img)
        return -1;

    /* 入口先清零:任何失败路径返回后 img 都不持有内存 */
    img->width = 0;
    img->height = 0;
    img->pixels = NULL;

    if (!path)
    {
        fprintf(stderr, "png_load: 文件路径为 NULL\n");
        return -1;
    }

    if (read_whole_file(path, &file_data, &file_size) != 0)
        return -1;

    if (memcmp(file_data, PNG_SIGNATURE, 8) != 0)
    {
        fprintf(stderr, "png_load: %s 不是 PNG 文件(签名不符)\n", path);
        free(file_data);
        return -1;
    }

    ret = decode_png(path, file_data, file_size, img);
    free(file_data);
    return ret;
}

void png_free(PngImage *img)
{
    if (!img)
        return;
    free(img->pixels);
    img->pixels = NULL;
    img->width = 0;
    img->height = 0;
}
