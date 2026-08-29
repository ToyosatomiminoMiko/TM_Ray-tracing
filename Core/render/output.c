#include "output.h"

#include <stdio.h>

void output_clear_screen(void)
{
    printf("\x1b[3J\x1b[2J\x1b[H");
    fflush(stdout);
}

/* 把一帧写成 P3 文本 PPM(行优先,每行一个像素的 RGB) */
static int output_ppm_to(FILE *fp, const framebuffer_t *fb)
{
    fprintf(fp, "P3\n%d %d\n255\n", W, H);
    for (int y = 0; y < H; y++)
    {
        for (int x = 0; x < W; x++)
        {
            uint32_t color = fb->pixels[y][x];
            fprintf(fp, "%d %d %d\n",
                    (int)((color >> 16) & 0xFF),
                    (int)((color >> 8) & 0xFF),
                    (int)(color & 0xFF));
        }
    }

    return fflush(fp) == EOF ? -1 : 0;
}

// [debug only]
void output_ansi(const framebuffer_t *fb)
{
    for (int y = 0; y < H; y++)
    {
        for (int x = 0; x < W; x++)
        {
            uint32_t color = fb->pixels[y][x];
            printf(
                "\033[48;2;%d;%d;%dm  \033[0m",
                (int)((color >> 16) & 0xFF),
                (int)((color >> 8) & 0xFF),
                (int)(color & 0xFF));
        }
        printf("\n");
    }
    fflush(stdout);
}

int output_ppm(const framebuffer_t *fb, const char *path)
{
    FILE *fp = fopen(path, "w");
    if (!fp)
    {
        perror(path);
        return -1;
    }

    int ret = output_ppm_to(fp, fb);
    fclose(fp);
    return ret;
}

/* 把一帧 PPM 以二进制 P6 直接写到 stdout,供 shell 管道送给 ffmpeg.
   不用 P3 文本的原因:文本帧的大小随像素内容变化,
   ffmpeg 在长帧流里偶发错位(报 "Invalid maxval: 0");
   P6 每帧固定 头 + W*H*3 字节,帧边界永远正确,管道数据量还小约 3 倍. */
int output_ppm_stream(const framebuffer_t *fb)
{
    fprintf(stdout, "P6\n%d %d\n255\n", W, H);

    unsigned char row[W * 3];
    for (int y = 0; y < H; y++)
    {
        for (int x = 0; x < W; x++)
        {
            uint32_t color = fb->pixels[y][x];
            row[x * 3 + 0] = (unsigned char)((color >> 16) & 0xFF);
            row[x * 3 + 1] = (unsigned char)((color >> 8) & 0xFF);
            row[x * 3 + 2] = (unsigned char)(color & 0xFF);
        }
        if (fwrite(row, 1, sizeof(row), stdout) != sizeof(row))
            return -1;
    }

    return fflush(stdout) == EOF ? -1 : 0;
}
